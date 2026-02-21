#include "FSArenaManager.h"

AFSArenaManager::AFSArenaManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFSArenaManager::BeginPlay()
{
	GetWorld()->GetTimerManager().SetTimer(
		SpawnZonesInitTimer,
		this,
		&AFSArenaManager::InitialiseSpawnZones,
		1.f,
		true
	);

	if (bForceActivate)
		StartArena();
}

void AFSArenaManager::StartArena()
{
	if (bIsArenaActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FSArenaManager] Arena already active."));
		return;
	}

	bIsArenaActive = true;
	CurrentMaxAlive = InitialMaxAlive;
	TotalSpawned = 0;
	TotalKills = 0;
	AliveEnemyCount = 0;
	NextEscalationIndex = 0;

	UE_LOG(LogTemp, Log, TEXT("[FSArenaManager] Arena started. TotalToSpawn: %d, InitialMaxAlive: %d"),
		TotalEnemiesToSpawn, InitialMaxAlive);

	OnArenaStarted.Broadcast();
	ScheduleNextSpawn();
}

void AFSArenaManager::TrySpawnEnemy()
{
	if (!bIsArenaActive || TotalSpawned >= TotalEnemiesToSpawn || AliveEnemyCount >= CurrentMaxAlive)
	{
		if (bIsArenaActive && TotalSpawned < TotalEnemiesToSpawn)
			ScheduleNextSpawn();
		return;
	}

	// Pick a random zone
	int32 randIndex{ FMath::RandRange(0, SpawnZones.Num() - 1) };
	if (!SpawnZones.IsValidIndex(randIndex) || !SpawnZones[randIndex])
	{
		ScheduleNextSpawn();
		return;
	}

	AFSEnemy* spawnedEnemy{ SpawnZones[randIndex]->SpawnEnemy() };
	if (spawnedEnemy)
	{
		TotalSpawned++;
		AliveEnemyCount++;
		spawnedEnemy->OnEnemyDeath.AddUObject(this, &AFSArenaManager::HandleOnEnemyDeath);

		UE_LOG(LogTemp, Log, TEXT("[FSArenaManager] Enemy spawned. Alive: %d/%d, Spawned: %d/%d"),
			AliveEnemyCount, CurrentMaxAlive, TotalSpawned, TotalEnemiesToSpawn);
	}

	// Keep spawning if budget remains
	if (TotalSpawned < TotalEnemiesToSpawn)
		ScheduleNextSpawn();
}

void AFSArenaManager::ScheduleNextSpawn()
{
	float cooldown{ FMath::RandRange(MinSpawnCooldown, MaxSpawnCooldown) };
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&AFSArenaManager::TrySpawnEnemy,
		cooldown,
		false
	);
}

void AFSArenaManager::HandleOnEnemyDeath(AFSEnemy* Enemy)
{
	AliveEnemyCount--;
	TotalKills++;

	UE_LOG(LogTemp, Log, TEXT("[FSArenaManager] Enemy killed. Kills: %d, Alive: %d, Remaining to spawn: %d"),
		TotalKills, AliveEnemyCount, TotalEnemiesToSpawn - TotalSpawned);

	CheckCapEscalation();
	CheckArenaCompletion();

	// A slot opened up, restart spawn timer if budget remains and timer is not active
	if (bIsArenaActive && TotalSpawned < TotalEnemiesToSpawn &&
		!GetWorld()->GetTimerManager().IsTimerActive(SpawnTimerHandle))
		ScheduleNextSpawn();
}

void AFSArenaManager::CheckCapEscalation()
{
	while (NextEscalationIndex < EscalationSteps.Num())
	{
		const FCapEscalationStep& step{ EscalationSteps[NextEscalationIndex] };

		if (TotalKills >= step.KillThreshold)
		{
			CurrentMaxAlive = FMath::Min(CurrentMaxAlive + step.MaxAliveIncrease, MaxAliveLimit);
			NextEscalationIndex++;

			UE_LOG(LogTemp, Log, TEXT("[FSArenaManager] MaxAlive escalated to %d at %d kills"),
				CurrentMaxAlive, TotalKills);
		}
		else
			break;
	}
}

void AFSArenaManager::CheckArenaCompletion()
{
	if (TotalSpawned >= TotalEnemiesToSpawn && AliveEnemyCount <= 0)
	{
		bIsArenaActive = false;
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);

		UE_LOG(LogTemp, Log, TEXT("[FSArenaManager] Arena cleared! Total kills: %d"), TotalKills);

		OnArenaCleared.Broadcast();
	}
}

void AFSArenaManager::InitialiseSpawnZones()
{
	TArray<AActor*> foundZones;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAFSSpawnZone::StaticClass(), foundZones);

	if (foundZones.IsEmpty())
	{
		SpawnZoneInitTries++;

		if (SpawnZoneInitTries >= MaxSpawnZoneInitTries)
			GetWorld()->GetTimerManager().ClearTimer(SpawnZonesInitTimer);

		return;
	}

	for (AActor* zone : foundZones)
		SpawnZones.Add(Cast<AAFSSpawnZone>(zone));

	GetWorld()->GetTimerManager().ClearTimer(SpawnZonesInitTimer);
}

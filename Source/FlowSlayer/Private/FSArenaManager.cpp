#include "FSArenaManager.h"

AFSArenaManager::AFSArenaManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFSArenaManager::BeginPlay()
{
	PlayerCharacter = Cast<AFlowSlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

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

void AFSArenaManager::StopArena()
{
	bIsArenaActive = false;
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);

	UE_LOG(LogTemp, Log, TEXT("[FSArenaManager] Arena stopped."));
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
		spawnedEnemy->OnEnemyDeath.AddUniqueDynamic(this, &AFSArenaManager::HandleOnEnemyDeath);
		OnEnemySpawned.Broadcast(spawnedEnemy);

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

	if (PlayerCharacter)
		PlayerCharacter->GetProgressionComponent()->AddXP(Enemy->GetXPReward());

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

		if (ExitPortal)
			ExitPortal->ShowPortal();
		if (RewardChest)
			RewardChest->ShowChest();

		OnArenaCleared.Broadcast();
	}
}

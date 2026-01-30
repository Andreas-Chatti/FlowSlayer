#include "AFSSpawnZone.h"

AAFSSpawnZone::AAFSSpawnZone()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnZoneComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnZoneComp"));
	SpawnZoneComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RootComponent = SpawnZoneComponent;
}

void AAFSSpawnZone::BeginPlay()
{
	Super::BeginPlay();

	if (bDebugLines)
		SpawnZoneComponent->bHiddenInGame = false;

	playerRef = GetWorld()->GetFirstPlayerController()->GetCharacter();

	GetWorld()->GetTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&AAFSSpawnZone::SpawnEnemy,
		SpawnCooldown,
		bIsSpawnEnabled
		);
}

void AAFSSpawnZone::SpawnEnemy()
{
	if (SpawnedEntities.Num() >= MaxEntities && MaxEntities != -1)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AFSSpawnZone] Cannot exceed the number of max entities set from this zone."));
		bIsSpawnEnabled = false;
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	TOptional<FTransform> enemyPosition{ GetRandomTransform() };
	if (!enemyPosition.IsSet())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AFSSpawnZone] Enemy spawn failed."));
		return;
	}

	int32 randIndex{ FMath::RandRange(0, EnemyPoolSpawn.Num() - 1) };

	if (EnemyPoolSpawn.IsValidIndex(randIndex))
	{
		FActorSpawnParameters spawnParams;
		AFSEnemy* spawnedEnemy{ GetWorld()->SpawnActor<AFSEnemy>(EnemyPoolSpawn[randIndex], enemyPosition.GetValue(), spawnParams) };

		if (spawnedEnemy)
		{
			spawnedEnemy->SpawnDefaultController();
			spawnedEnemy->OnEnemyDeath.AddUObject(this, &AAFSSpawnZone::HandleOnEnemyDeath);
			SpawnedEntities.Add(spawnedEnemy);
		}
	}

	SpawnCooldown = FMath::RandRange(MinCooldown, MaxCooldown);

	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);

	GetWorld()->GetTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&AAFSSpawnZone::SpawnEnemy,
		SpawnCooldown,
		bIsSpawnEnabled
	);
}

TOptional<FTransform> AAFSSpawnZone::GetRandomTransform()
{
	if (!SpawnZoneComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[AFSSpawnZone] SpawnZoneComponent is NULL"));
		return NullOpt;
	}

	FVector boxExtent{ SpawnZoneComponent->GetScaledBoxExtent() };
	FVector boxOrigin{ SpawnZoneComponent->GetComponentLocation() };

	constexpr int32 maxTries{ 5 };
	int32 tryCount{ 0 };
	while (tryCount < maxTries)
	{
		tryCount++;

		double randomX{ FMath::FRandRange(-boxExtent.X, boxExtent.X) };
		double randomY{ FMath::FRandRange(-boxExtent.Y, boxExtent.Y) };

		FVector randomWorldPosition{ boxOrigin.X + randomX, boxOrigin.Y + randomY, boxOrigin.Z + boxExtent.Z };

		FHitResult hitResult;
		FVector traceStart{ randomWorldPosition };

		constexpr double groundTraceMargin{ 100.0 };
		double targetZ{ boxOrigin.Z - boxExtent.Z - groundTraceMargin };
		FVector traceEnd{ randomWorldPosition.X, randomWorldPosition.Y, targetZ };

		FCollisionQueryParams queryParams;
		queryParams.bTraceComplex = false;

		if (!GetWorld()->LineTraceSingleByChannel(hitResult, traceStart, traceEnd, ECC_WorldStatic, queryParams))
		{
			UE_LOG(LogTemp, Warning, TEXT("[AFSSpawnZone] No ground detected"));
			continue;
		}

		randomWorldPosition.Z = hitResult.ImpactPoint.Z;

		if (bDebugLines)
		{
			DrawDebugSphere(GetWorld(), hitResult.ImpactPoint, 10.0f, 12, FColor::Red, false, 2.0f);
			DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor::Green, false, 2.0f);
		}

		if (!playerRef)
			playerRef = GetWorld()->GetFirstPlayerController()->GetCharacter();

		if (playerRef)
		{
			FVector playerLoc{ playerRef->GetActorLocation() };
			double distSquared{ FVector::DistSquared(playerLoc, randomWorldPosition) };
			if (distSquared < FMath::Square(MinSpawnDistance))
				continue;
		}

		return FTransform{ randomWorldPosition };
	}

	return NullOpt;
}

void AAFSSpawnZone::HandleOnEnemyDeath(AFSEnemy* enemy)
{
	SpawnedEntities.Remove(enemy);

	if (SpawnedEntities.Num() < MaxEntities && MaxEntities != -1 && !bIsSpawnEnabled && 
		!GetWorld()->GetTimerManager().IsTimerActive(SpawnTimerHandle))
	{

		bIsSpawnEnabled = true;

		GetWorld()->GetTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&AAFSSpawnZone::SpawnEnemy,
			SpawnCooldown,
			bIsSpawnEnabled
		);
	}
}

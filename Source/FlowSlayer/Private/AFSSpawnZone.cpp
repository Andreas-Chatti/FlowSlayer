#include "AFSSpawnZone.h"

AAFSSpawnZone::AAFSSpawnZone()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnZoneComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SpawnZoneComp"));
	SpawnZoneComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = SpawnZoneComponent;
}

void AAFSSpawnZone::BeginPlay()
{
	Super::BeginPlay();

	SpawnZoneComponent->SetHiddenInGame(!bDebugLines);

	playerRef = GetWorld()->GetFirstPlayerController()->GetCharacter();
	verifyf(playerRef, TEXT("[SpawnZone] WARNING: PlayerRef is NULL or INVALID !"));

	navSystem = FNavigationSystem::GetCurrent<const UNavigationSystemV1>(GetWorld());
	checkf(navSystem, TEXT("[SpawnZone] FATAL: Navigation system is NULL or INVALID !"));
}

AFSEnemy* AAFSSpawnZone::SpawnEnemy()
{
	if (EnemyPoolSpawn.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[SpawnZone] EnemyPoolSpawn is empty — assign enemy classes in the editor."));
		return nullptr;
	}

	TOptional<FTransform> enemyPosition{ GetRandomTransform() };
	if (!enemyPosition.IsSet())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnZone] Enemy spawn failed — no valid NavMesh position found."));
		return nullptr;
	}

	int32 randIndex{ FMath::RandRange(0, EnemyPoolSpawn.Num() - 1) };
	AFSEnemy* spawnedEnemy{ GetWorld()->SpawnActor<AFSEnemy>(EnemyPoolSpawn[randIndex], enemyPosition.GetValue(), {}) };

	if (spawnedEnemy)
		spawnedEnemy->SpawnDefaultController();

	return spawnedEnemy;
}

TOptional<FTransform> AAFSSpawnZone::GetRandomTransform()
{
	FVector origin{ SpawnZoneComponent->GetComponentLocation() };
	float searchRadius{ SpawnZoneComponent->GetScaledSphereRadius() };

	constexpr int32 maxTries{ 50 };
	for (int32 tryCount{ 0 }; tryCount < maxTries; ++tryCount)
	{
		// Returns any navigable NavMesh point within searchRadius, regardless of pathfinding connectivity from origin
		FNavLocation navLocation;
		if (!navSystem->GetRandomPointInNavigableRadius(origin, searchRadius, navLocation))
		{
			UE_LOG(LogTemp, Warning, TEXT("[SpawnZone] No navigable NavMesh point found"));
			continue;
		}

		double distSquared{ FVector::DistSquared(playerRef->GetActorLocation(), navLocation.Location) };
		if (distSquared < FMath::Square(MinSpawnDistance))
		{
			if (bDebugLines)
				DrawDebugSphere(GetWorld(), navLocation.Location, 10.0f, 12, FColor::Red, false, 2.0f);

			continue;
		}

		if (bDebugLines)
			DrawDebugSphere(GetWorld(), navLocation.Location, 10.0f, 12, FColor::Green, false, 2.0f);

		return FTransform{ navLocation.Location };
	}

	return NullOpt;
}

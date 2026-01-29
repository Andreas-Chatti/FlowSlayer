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
			spawnedEnemy->SpawnDefaultController();
	}

	SpawnCooldown = FMath::RandRange(MIN_COOLDOWN, MAX_COOLDOWN);

	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);

	GetWorld()->GetTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&AAFSSpawnZone::SpawnEnemy,
		SpawnCooldown,
		bIsSpawnEnabled
	);
}

TOptional<FTransform> AAFSSpawnZone::GetRandomTransform() const
{
	if (!SpawnZoneComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[AFSSpawnZone] SpawnZoneComponent is NULL"));
		return NullOpt;
	}

	FVector boxExtent{ SpawnZoneComponent->GetScaledBoxExtent() };
	FVector boxOrigin{ SpawnZoneComponent->GetComponentLocation() };

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
		return NullOpt;
	}

	randomWorldPosition.Z = hitResult.ImpactPoint.Z;

	if (bDebugLines)
	{
		DrawDebugSphere(GetWorld(), hitResult.ImpactPoint, 10.0f, 12, FColor::Red, false, 2.0f);
		DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor::Green, false, 2.0f);
	}

	return FTransform(randomWorldPosition);
}
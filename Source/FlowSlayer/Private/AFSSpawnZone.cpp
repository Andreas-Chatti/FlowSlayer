#include "AFSSpawnZone.h"

AAFSSpawnZone::AAFSSpawnZone()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnZoneComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnZoneComp"));
	SpawnZoneComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RootComponent = SpawnZoneComponent;
}

void AAFSSpawnZone::BeginPlay()
{
	Super::BeginPlay();

	// Probème : Le timer de cette méthode reste toujours fixe tant qu'on ne désactive pas le timer pour en remettre un autre
	// Solution : A la fin de SpawnEnemy, désactiver le timer et le relancer directement avec le nouveau SpawnCooldown
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
	FTransform enemyPosition{ GetRandomTransform() };
	FActorSpawnParameters spawnParams;

	// TODO : Faire une sélection aléatoire du pool d'ennemis dans l'array membre de AFSSpawnZone
	// Problème : Chaque ennemi ici ne semble pas avoir de AIController qui marche. L'ennemi spawn mais est inactif.
	AFSEnemy* spawnedEnemy{ GetWorld()->SpawnActor<AFSEnemy_Grunt>(EnemyToSpawn, enemyPosition, spawnParams) };

	SpawnCooldown = FMath::RandRange(MIN_COOLDOWN, MAX_COOLDOWN);
}

FTransform AAFSSpawnZone::GetRandomTransform() const
{
	if (!SpawnZoneComponent)
		return FTransform();

	FVector boxExtent{ SpawnZoneComponent->GetScaledBoxExtent() };
	FVector boxOrigin{ SpawnZoneComponent->GetComponentLocation() };

	// Position aléatoire en X/Y seulement
	double randomX{ FMath::FRandRange(-boxExtent.X, boxExtent.X) };
	double randomY{ FMath::FRandRange(-boxExtent.Y, boxExtent.Y) };

	FVector randomWorldPosition{ boxOrigin.X + randomX, boxOrigin.Y + randomY, boxOrigin.Z + boxExtent.Z };

	// Line trace vers le bas pour trouver le sol
	FHitResult hitResult;
	FVector traceStart{ randomWorldPosition };
	// C'est quoi 100.f ?
	// C'est quoi boxOrigin.Z - boxExtend.Z ?
	FVector traceEnd{ randomWorldPosition.X, randomWorldPosition.Y, boxOrigin.Z - boxExtent.Z - 100.f };

	FCollisionQueryParams queryParams;
	queryParams.bTraceComplex = false;

	if (GetWorld()->LineTraceSingleByChannel(hitResult, traceStart, traceEnd, ECC_WorldStatic, queryParams))
	{
		randomWorldPosition.Z = hitResult.ImpactPoint.Z;
		DrawDebugSphere(GetWorld(), hitResult.ImpactPoint, 10.0f, 12, FColor::Red, false, 2.0f);
	}

	DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor::Green, false, 2.0f);

	return FTransform(randomWorldPosition);
}

void AAFSSpawnZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

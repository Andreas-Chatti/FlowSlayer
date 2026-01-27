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

	AFSEnemy* spawnedEnemy{ GetWorld()->SpawnActor<AFSEnemy_Grunt>(AFSEnemy_Grunt::StaticClass(), enemyPosition, spawnParams) };

	SetNewRandomSpawnCooldown();
}

FTransform AAFSSpawnZone::GetRandomTransform() const
{
	if (!SpawnZoneComponent)
		return FTransform();

	// Récupérer les dimensions de la box
	FVector boxExtent = SpawnZoneComponent->GetScaledBoxExtent();
	FVector boxOrigin = SpawnZoneComponent->GetComponentLocation();

	// Générer position aléatoire dans la box
	float randomX = FMath::FRandRange(-boxExtent.X, boxExtent.X);
	float randomY = FMath::FRandRange(-boxExtent.Y, boxExtent.Y);
	float randomZ = FMath::FRandRange(-boxExtent.Z, boxExtent.Z);

	FVector randomLocalPosition{ randomX, randomY, randomZ };
	FVector randomWorldPosition = boxOrigin + randomLocalPosition;

	return FTransform(randomWorldPosition);
}

void AAFSSpawnZone::SetNewRandomSpawnCooldown()
{
	float newRandCooldown{ FMath::RandRange(MIN_COOLDOWN, MAX_COOLDOWN) };

	SpawnCooldown = std::move(newRandCooldown);
}

void AAFSSpawnZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

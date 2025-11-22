#include "FSEnemy_Grunt.h"

AFSEnemy_Grunt::AFSEnemy_Grunt()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AFSEnemy_Grunt::BeginPlay()
{
	Super::BeginPlay();
}

void AFSEnemy_Grunt::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFSEnemy_Grunt::Attack_Implementation()
{
	Super::Attack_Implementation();

	UE_LOG(LogTemp, Warning, TEXT("[GRUNT SPECIFIC] Slow heavy swing!"));

	// TODO: Comportement unique au Grunt
	// - Animation spéciale du Grunt
	// - Son spécifique
	// - Effets visuels uniques
	// - Logique d'attaque différente (par ex: AOE, knockback, etc.)
}

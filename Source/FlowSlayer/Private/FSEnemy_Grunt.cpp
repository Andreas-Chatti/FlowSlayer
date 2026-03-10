#include "FSEnemy_Grunt.h"

AFSEnemy_Grunt::AFSEnemy_Grunt()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFSEnemy_Grunt::Attack_Implementation()
{
    Super::Attack_Implementation();

	// TODO: Comportement unique au Grunt
	// - Animation spéciale du Grunt
	// - Son spécifique
	// - Effets visuels uniques
	// - Logique d'attaque différente (par ex: AOE, knockback, etc.)
}
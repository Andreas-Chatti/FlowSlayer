#include "FSEnemy_Runner.h"

AFSEnemy_Runner::AFSEnemy_Runner()
{
}

void AFSEnemy_Runner::BeginPlay()
{
	Super::BeginPlay();
}

void AFSEnemy_Runner::Attack_Implementation()
{
	// 1️⃣ D'ABORD : Appelle le comportement générique de la classe parente
	Super::Attack_Implementation();

	// 2️⃣ ENSUITE : Ajoute le comportement spécifique au Runner
	UE_LOG(LogTemp, Warning, TEXT("[RUNNER SPECIFIC] Quick dash attack!"));

	// TODO: Comportement unique au Runner
	// - Animation rapide de dash
	// - Son de dash/rush
	// - Effets visuels de vitesse (motion blur, trail)
	// - Peut-être 2-3 attaques rapides consécutives
	// - Frappe en passant (hit-and-run)
}


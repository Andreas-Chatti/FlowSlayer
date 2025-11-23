#include "FSEnemy_Runner.h"

AFSEnemy_Runner::AFSEnemy_Runner()
{
	PrimaryActorTick.bCanEverTick = false;

	ProjectileShootSocket = "AttackProjectile";
}

void AFSEnemy_Runner::BeginPlay()
{
	Super::BeginPlay();

	OnProjectileSpawned.AddUObject(this, &AFSEnemy_Runner::ShootProjectileAtPlayer);
}

void AFSEnemy_Runner::Attack_Implementation()
{
	// 1️⃣ D'ABORD : Appelle le comportement générique de la classe parente
	Super::Attack_Implementation();

	// 2️⃣ ENSUITE : Ajoute le comportement spécifique au Runner
	UE_LOG(LogTemp, Warning, TEXT("[RUNNER SPECIFIC] Magic Projectile throwed !"));

	// TODO: Comportement unique au Runner
	// - Animation rapide de dash
	// - Son de dash/rush
	// - Effets visuels de vitesse (motion blur, trail)
	// - Peut-être 2-3 attaques rapides consécutives
	// - Frappe en passant (hit-and-run)
}

void AFSEnemy_Runner::ShootProjectileAtPlayer()
{
	// Determine trajectory to enemy
	// Spawn projectile
	// OnHitEvent (in projectile class)

	if (!ProjectileClass)
		return;

	FVector spawnLocation{ GetMesh()->GetSocketLocation(ProjectileShootSocket) };
	AFSProjectile* projectile{ AFSProjectile::SpawnProjectile(GetWorld(), this, ProjectileClass, spawnLocation, GetActorRotation()) };
	if (projectile)
	{
		projectile->SetDamage(Damage);
		FVector PlayerLocation{ Player->GetActorLocation() };
		FVector ShootDirection{ (PlayerLocation - spawnLocation).GetSafeNormal() };
		projectile->FireInDirection(ShootDirection);
	}
}


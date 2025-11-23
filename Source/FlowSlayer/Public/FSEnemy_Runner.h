#pragma once
#include "CoreMinimal.h"
#include "Sockets.h"
#include "FSEnemy.h"
#include "FSProjectile.h"
#include "FSEnemy_Runner.generated.h"

UCLASS()
class FLOWSLAYER_API AFSEnemy_Runner : public AFSEnemy
{
	GENERATED_BODY()
	
public:

	AFSEnemy_Runner();

protected:

	virtual void BeginPlay() override;

	virtual void Attack_Implementation() override;

	void ShootProjectileAtPlayer();

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<AFSProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	FName ProjectileShootSocket;
};


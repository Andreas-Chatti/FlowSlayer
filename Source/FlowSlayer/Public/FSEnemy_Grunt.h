#pragma once
#include "CoreMinimal.h"
#include "FSEnemy.h"
#include "FSEnemy_Grunt.generated.h"

UCLASS()
class FLOWSLAYER_API AFSEnemy_Grunt : public AFSEnemy
{
	GENERATED_BODY()
	
public:

	AFSEnemy_Grunt();

protected:

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	void Attack_Implementation() override;

	void UpdateDamageHitbox();
	void ActivateDamageHitbox();
	void DeactivateDamageHitbox();

	/** Wether Damage Hitbox is activated */
	bool bIsDamageHitboxActive{ false };

	/** Unique list of actors hit during an attack */
	UPROPERTY()
	TSet<AActor*> ActorsHitThisAttack;

	/** Used for sweep during an attack */
	FVector PreviousHitboxLocation;
};

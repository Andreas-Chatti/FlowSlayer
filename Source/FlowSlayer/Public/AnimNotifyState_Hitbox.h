#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "FSCombatComponent.h"
#include "FSEnemy.h"
#include "AnimNotifyState_Hitbox.generated.h"

/**
 * Activates attack hitbox detection between NotifyBegin and NotifyEnd.
 * Used for player weapon hitboxes and enemy attack hitboxes.
 */
UCLASS(meta = (DisplayName = "Weapon active frame"))
class FLOWSLAYER_API UAnimNotifyState_Hitbox : public UAnimNotifyState
{
	GENERATED_BODY()

	UAnimNotifyState_Hitbox();
	
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** Hitbox profile of the attack occuring during this instance 
	* Set up directly in the anim montage
	*/
	UPROPERTY(EditAnywhere, Category = "Hitbox")
	FHitboxProfile HitboxProfile;

	/** Player character combat component 
	* Contains the equipped weapon actor to activate/deactivate the hitbox 
	* nullptr if instigator is an enemy
	*/
	UPROPERTY()
	UFSCombatComponent* CombatComp{ nullptr };

	/** Player's weapon reference 
	* nullptr if instigator is an enemy
	*/
	UPROPERTY()
	const AFSWeapon* PlayerWeapon{ nullptr };

	/* EnemyInstigator (AFSEnemy) 
	* Used if the instigator is an AFSEnemy
	* nullptr otherwise
	*/
	UPROPERTY()
	AFSEnemy* EnemyInstigator{ nullptr };
};

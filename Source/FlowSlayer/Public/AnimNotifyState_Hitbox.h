#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "HitboxComponent.h"
#include "FSEnemy.h"
#include "AnimNotifyState_Hitbox.generated.h"

/**
 * Activates attack hitbox detection between NotifyBegin and NotifyEnd.
 * Used for player and enemies hitboxes (UHitboxComponent)
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

	/** Player's weapon reference 
	* nullptr if instigator is an enemy
	*/
	UPROPERTY()
	const UHitboxComponent* HitboxComp{ nullptr };
};

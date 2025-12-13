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
UCLASS(meta = (DisplayName = "Weapon Hitbox"))
class FLOWSLAYER_API UAnimNotifyState_Hitbox : public UAnimNotifyState
{
	GENERATED_BODY()

	UAnimNotifyState_Hitbox();
	
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** Player character combat component 
	* Contains the equipped weapon actor to activate/deactivate the hitbox 
	*/
	UPROPERTY()
	UFSCombatComponent* CombatComp{ nullptr };

	/* EnemyInstigator (AFSEnemy) 
	* Used if the instigator is an AFSEnemy
	*/
	UPROPERTY()
	AFSEnemy* EnemyInstigator{ nullptr };
};

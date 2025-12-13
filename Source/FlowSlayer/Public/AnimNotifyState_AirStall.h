#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "FSCombatComponent.h"
#include "AnimNotifyState_AirStall.generated.h"

/* This notify state is mainly used to set an Air Stall during an air attack
* When notify begin it takes the default GravityScale from the player's Character Movement Component
* and then call the callback from CombatComponent to activate and change current GravityScale with the AirStallGravity value (See FSCombatComponent.h)
* Then on NotifyEnd, set back the saved GravityScale value to remove air stall and set gravity back to normal
*/
UCLASS(meta = (DisplayName = "Air stall window"))
class FLOWSLAYER_API UAnimNotifyState_AirStall : public UAnimNotifyState
{
	GENERATED_BODY()

	UAnimNotifyState_AirStall();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY()
	UFSCombatComponent* CombatComp{ nullptr };

	/** Player's default gravity scale before air stall */
	float GravityScale;
};

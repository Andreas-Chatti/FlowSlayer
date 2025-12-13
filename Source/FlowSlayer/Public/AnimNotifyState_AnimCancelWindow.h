#pragma once
#include "CoreMinimal.h"
#include "../FlowSlayerCharacter.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_AnimCancelWindow.generated.h"

/**
 * Defines a time window during an animation where the player can cancel into another action.
 * Used for combo transitions and skill-based animation canceling.
 */
UCLASS(meta = (DisplayName = "Animation Cancel Window"))
class FLOWSLAYER_API UAnimNotifyState_AnimCancelWindow : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:

	UAnimNotifyState_AnimCancelWindow();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:

	/** Cached reference to the FlowSlayer character */
	UPROPERTY()
	const AFlowSlayerCharacter* FSCharacter { nullptr };

	/** Cached reference to the character's combat component */
	UPROPERTY()
	const UFSCombatComponent* CombatComp { nullptr };
};

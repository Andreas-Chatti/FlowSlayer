#pragma once
#include "CoreMinimal.h"
#include "../FlowSlayerCharacter.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_AnimCancelWindow.generated.h"

UENUM(BlueprintType)
enum class EAnimCancelWindowActionType : uint8
{
	Dash,
	Move,
	Any
};

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


protected:

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:

	/** Cached reference to the FlowSlayer character */
	UPROPERTY()
	const AFlowSlayerCharacter* FSCharacter { nullptr };

	/** Type of action that will cancel the current animation during this window */
	UPROPERTY(EditAnywhere)
	EAnimCancelWindowActionType CancelActionTrigger{ EAnimCancelWindowActionType::Any };

	/** TRUE once OnAnimationCanceled has been broadcast — prevents firing multiple times per window */
	bool bAnimCancelTrigger{ false };
};

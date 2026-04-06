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
	Guard,
	Custom,
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
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:

	/** Cached reference to the FlowSlayer character */
	UPROPERTY()
	const AFlowSlayerCharacter* FSCharacter { nullptr };

	/** Cached reference of the Player's EIC */
	UPROPERTY()
	UEnhancedInputComponent* EIC{ nullptr };

	/** Type of action that will cancel the current animation during this window */
	UPROPERTY(EditAnywhere, Category = "AnimationCancel")
	EAnimCancelWindowActionType CancelActionTrigger{ EAnimCancelWindowActionType::Any };

	UPROPERTY(EditAnywhere, Category = "AnimationCancel", meta = (EditCondition = "CancelActionTrigger == EAnimCancelWindowActionType::Custom"))
	TSet<EAnimCancelWindowActionType> CancelActionTriggerCustom{};

	/** BlendOutTime of the current animation when canceling */
	UPROPERTY(EditAnywhere, Category = "AnimationCancel")
	float CancelBlendOutTime{ 0.f };

	/** TRUE once OnAnimationCanceled has been broadcast — prevents firing multiple times per window */
	bool bAnimCancelTrigger{ false };

	///// DASH CANCEL /////

	/** Dash input action that will cancel this animation during this window */
	UPROPERTY(EditAnywhere, Category = "AnimationCancel", meta = (EditCondition = "CancelActionTrigger == EAnimCancelWindowActionType::Dash || CancelActionTrigger == EAnimCancelWindowActionType::Any || CancelActionTrigger == EAnimCancelWindowActionType::Custom"))
	const UInputAction* DashAction {nullptr};

	/** Cached binding of dash's action to remove it in NotifyEnd() */
	uint32 DashBindingHandle{ 0 };

	UFUNCTION()
	void HandleDashCancelInput();

	///// Move CANCEL /////

	UPROPERTY(EditAnywhere, Category = "AnimationCancel", meta = (EditCondition = "CancelActionTrigger == EAnimCancelWindowActionType::Move || CancelActionTrigger == EAnimCancelWindowActionType::Any || CancelActionTrigger == EAnimCancelWindowActionType::Custom"))
	const UInputAction* MoveAction {nullptr};

	/** Cached binding of move's action to remove it in NotifyEnd() */
	uint32 MoveBindingHandle{ 0 };

	UFUNCTION()
	void HandleMoveCancelInput();

	///// GUARD CANCEL /////

	UPROPERTY(EditAnywhere, Category = "AnimationCancel", meta = (EditCondition = "CancelActionTrigger == EAnimCancelWindowActionType::Guard || CancelActionTrigger == EAnimCancelWindowActionType::Any || CancelActionTrigger == EAnimCancelWindowActionType::Custom"))
	const UInputAction* GuardAction { nullptr };

	/** Cached binding of move's action to remove it in NotifyEnd() */
	uint32 GuardBindingHandle{ 0 };

	UFUNCTION()
	void HandleGuardCancelInput();

	/** Returns true if the given action type should be bound for this cancel window,
	 *  based on CancelActionTrigger and CancelActionTriggerCustom */
	bool ShouldBind(EAnimCancelWindowActionType Type) const;
};

#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "../FlowSlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AnimNotifyState_MovementSpeed.generated.h"

/**
 * Overrides MaxWalkSpeed during an animation window.
 * On NotifyEnd, restores the correct speed based on the current lock-on state
 * (run speed if locked on, sprint speed otherwise).
 * Supports either instant snap or smooth interpolation toward the target speed.
 */
UCLASS(meta = (DisplayName = "MaxWalkSpeedModifier"))
class FLOWSLAYER_API UAnimNotifyState_MovementSpeed : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	UAnimNotifyState_MovementSpeed();

protected:

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** Target MaxWalkSpeed applied during this notify window */
	UPROPERTY(EditAnywhere, Category = "Speed")
	float TargetSpeed{ 300.f };

	/** If true, snaps directly to TargetSpeed on the first frame instead of interpolating */
	UPROPERTY(EditAnywhere, Category = "Speed")
	bool bSnapSpeed{ false };

	/** Interpolation speed toward the target speed (used only when bSnapSpeed is false) */
	UPROPERTY(EditAnywhere, Category = "Speed", meta = (ClampMin = "0.1", EditCondition = "!bSnapSpeed"))
	float InterpolationSpeed{ 10.f };

private:

	/** Cached reference to the owning FlowSlayer character */
	UPROPERTY()
	AFlowSlayerCharacter* OwnerRef{ nullptr };
};

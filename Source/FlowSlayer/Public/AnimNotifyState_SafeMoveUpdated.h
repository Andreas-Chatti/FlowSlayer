#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DashComponent.h"
#include "AnimNotifyState_SafeMoveUpdated.generated.h"

/**
 * AnimNotifyState that drives the dash movement via SafeMoveUpdatedComponent.
 *
 * Reads the dash direction and distance from UDashComponent on the owning character.
 * Movement is curve-driven over the notify's duration. Calls UDashComponent::EndDash()
 * when the notify ends to reset dash state and start the cooldown.
 */
UCLASS(meta = (DisplayName = "Dash"))
class FLOWSLAYER_API UAnimNotifyState_SafeMoveUpdated : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:

	UAnimNotifyState_SafeMoveUpdated();

protected:

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** Drives the dash movement profile (acceleration, deceleration, etc.)
	*  X axis = normalized time [0, 1]  Y axis = normalized displacement [0, 1] */
	UPROPERTY(EditAnywhere, meta = (DisplayName = "DashCurve"))
	UCurveFloat* MoveCurve{ nullptr };

private:

	/** Owning character, cached in NotifyBegin */
	UPROPERTY()
	ACharacter* OwningCharacter{ nullptr };

	/** DashComponent on the owning character — provides direction, distance, and EndDash() */
	UPROPERTY()
	UDashComponent* DashComp{ nullptr };

	/** World-space start position of the dash, captured in NotifyBegin */
	FVector Start{ FVector::ZeroVector };

	/** World-space target position of the dash — Start + direction * Distance */
	FVector End{ FVector::ZeroVector };

	/** Time elapsed since move started, used to compute normalized alpha [0, 1] */
	float TimeElapsed{ 0.f };

	/** Curve output from the previous frame� used to compute per-frame delta displacement */
	float LastCurveValue{ 0.f };

	/** True once alpha reaches 1.0 — stops further movement in NotifyTick */
	bool bDashFinished{ false };

	/** Total duration of the notify bar, captured from TotalDuration in NotifyBegin */
	float NotifyDuration{ 0.f };
};

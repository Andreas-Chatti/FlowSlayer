#include "AnimNotifyState_MovementSpeed.h"

UAnimNotifyState_MovementSpeed::UAnimNotifyState_MovementSpeed()
{
	NotifyColor = FColor::Silver;
	bShouldFireInEditor = false;
}

void UAnimNotifyState_MovementSpeed::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	OwnerRef = Cast<AFlowSlayerCharacter>(MeshComp->GetOwner());
	if (!OwnerRef)
		return;

	if (bSnapSpeed)
		OwnerRef->GetCharacterMovement()->MaxWalkSpeed = TargetSpeed;
}

void UAnimNotifyState_MovementSpeed::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (bSnapSpeed || !OwnerRef)
		return;

	OwnerRef->GetCharacterMovement()->MaxWalkSpeed = FMath::FInterpTo(OwnerRef->GetCharacterMovement()->MaxWalkSpeed, TargetSpeed, FrameDeltaTime, InterpolationSpeed);
}

void UAnimNotifyState_MovementSpeed::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!OwnerRef)
		return;

	OwnerRef->GetCharacterMovement()->MaxWalkSpeed = OwnerRef->GetLockOnComponent()->IsLockedOnTarget() ? OwnerRef->GetRunSpeedThreshold() : OwnerRef->GetSprintSpeedThreshold();
	OwnerRef = nullptr;
}

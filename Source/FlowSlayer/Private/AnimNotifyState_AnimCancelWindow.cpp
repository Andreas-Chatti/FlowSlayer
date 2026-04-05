#include "AnimNotifyState_AnimCancelWindow.h"

UAnimNotifyState_AnimCancelWindow::UAnimNotifyState_AnimCancelWindow()
{
	NotifyColor = FColor::Green;
}

void UAnimNotifyState_AnimCancelWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
		return;

	AActor* Owner{ MeshComp->GetOwner() };
	FSCharacter = Cast<AFlowSlayerCharacter>(Owner);
	if (!FSCharacter)
		return;

	if (CancelActionTrigger == EAnimCancelWindowActionType::Move)
		return;
}

void UAnimNotifyState_AnimCancelWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!FSCharacter || bAnimCancelTrigger)
		return;

	bool bDashCancel{ FSCharacter->GetInputManagerComponent()->GetInputKeyState(EKeys::LeftShift) && FSCharacter->GetInputManagerComponent()->HasMovementInput() && CancelActionTrigger == EAnimCancelWindowActionType::Dash};
	bool bMoveCancel{ FSCharacter->GetInputManagerComponent()->HasMovementInput() && CancelActionTrigger == EAnimCancelWindowActionType::Move };
	bool bAnyCancel{ CancelActionTrigger == EAnimCancelWindowActionType::Any && (FSCharacter->GetInputManagerComponent()->GetInputKeyState(EKeys::LeftShift) || FSCharacter->GetInputManagerComponent()->HasMovementInput()) };

	if (bDashCancel || bMoveCancel || bAnyCancel)
	{
		bAnimCancelTrigger = true;
		FSCharacter->OnAnimationCanceled.Broadcast(CancelBlendOutTime);
	}
}

void UAnimNotifyState_AnimCancelWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	bAnimCancelTrigger = false;
}
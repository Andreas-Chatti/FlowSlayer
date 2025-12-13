#include "AnimNotifyState_AnimCancelWindow.h"

UAnimNotifyState_AnimCancelWindow::UAnimNotifyState_AnimCancelWindow()
{
	NotifyColor = FColor::Green;
}

void UAnimNotifyState_AnimCancelWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
		return;

	AActor* Owner{ MeshComp->GetOwner() };
	FSCharacter = Cast<AFlowSlayerCharacter>(Owner);
	if (!FSCharacter)
		return;

	CombatComp = FSCharacter->GetCombatComponent();
}

void UAnimNotifyState_AnimCancelWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (!FSCharacter)
		return;

	else if (FSCharacter->WantsToJump() || FSCharacter->WantsToDash())
	{
		FlowSlayerInput::EActionType actionType{ FlowSlayerInput::EActionType::NONE };

		if (FSCharacter->WantsToJump())
			actionType = FlowSlayerInput::EActionType::Jump;
		else if (FSCharacter->WantsToDash())
			actionType = FlowSlayerInput::EActionType::Dash;

		FSCharacter->OnAnimationCanceled.Broadcast(actionType);
	}
}

void UAnimNotifyState_AnimCancelWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
}
#include "AnimationCancelWindow.h"

void UAnimationCancelWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
		return;

	AActor* Owner{ MeshComp->GetOwner() };
	FSCharacter = Cast<AFlowSlayerCharacter>(Owner);
	if (!FSCharacter)
		return;

	CombatComp = FSCharacter->GetCombatComponent();
}

void UAnimationCancelWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (!FSCharacter)
		return;


	else if (FSCharacter->bWasJumping || FSCharacter->WantsToDash())
	{
		FlowSlayerInput::EActionType actionType{ FlowSlayerInput::EActionType::NONE };

		if (FSCharacter->bWasJumping)
			actionType = FlowSlayerInput::EActionType::Jump;
		else if (FSCharacter->WantsToDash())
			actionType = FlowSlayerInput::EActionType::Dash;

		FSCharacter->OnAnimationCanceled.Broadcast(actionType);
	}
}

void UAnimationCancelWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
}
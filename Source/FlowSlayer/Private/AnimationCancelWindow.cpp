#include "AnimationCancelWindow.h"

void UAnimationCancelWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
		return;

	AActor* Owner{ MeshComp->GetOwner() };
	if (!Owner)
		return;

	CombatComp = Owner->FindComponentByClass<UFSCombatComponent>();
	if (!CombatComp)
		return;

	CombatComp->OnFullComboWindowOpened.Broadcast();
}

void UAnimationCancelWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
}

void UAnimationCancelWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!CombatComp)
		return;

	CombatComp->OnFullComboWindowClosed.Broadcast();
}

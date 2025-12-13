#include "AnimNotifyState_ComboWindow.h"

UAnimNotifyState_ComboWindow::UAnimNotifyState_ComboWindow()
{
	NotifyColor = FColor::Blue;
}

void UAnimNotifyState_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
		return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
		return;

	CombatComp = Owner->FindComponentByClass<UFSCombatComponent>();
	if (!CombatComp)
		return;

	CombatComp->OnComboWindowOpened.Broadcast();
}

void UAnimNotifyState_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!CombatComp)
		return;

	CombatComp->OnComboWindowClosed.Broadcast();
}

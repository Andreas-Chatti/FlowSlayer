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

	bHasClosedEarly = false;
	CombatComp->OnComboInputWindowOpened.ExecuteIfBound();
}

void UAnimNotifyState_ComboWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (!CombatComp || bHasClosedEarly)
		return;

	// Immediatly do the transition to next attack if player wants to chain to a new combo
	if (CombatComp->GetChainingToNewCombo())
	{
		// Closing input window early to trigger ContinueCombo()
		CombatComp->OnComboInputWindowClosed.ExecuteIfBound();
		bHasClosedEarly = true;
	}
}

void UAnimNotifyState_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!CombatComp || bHasClosedEarly)
		return;

	CombatComp->OnComboInputWindowClosed.ExecuteIfBound();
}

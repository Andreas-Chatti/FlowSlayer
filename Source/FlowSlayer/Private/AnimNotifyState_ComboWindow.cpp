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
	CombatComp->OnComboWindowOpened.Broadcast();
}

void UAnimNotifyState_ComboWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (!CombatComp || bHasClosedEarly)
		return;

	// Si le joueur a demandé de chain vers un nouveau combo, effectuer la transition immédiatement
	if (CombatComp->GetChainingToNewCombo())
	{
		// Broadcaster la fermeture de la combo window pour déclencher ContinueCombo()
		CombatComp->OnComboWindowClosed.Broadcast();
		bHasClosedEarly = true;

		// Le nouveau montage remplacera automatiquement l'ancien via PlayAnimMontage()
	}
}

void UAnimNotifyState_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!CombatComp || bHasClosedEarly)
		return;

	CombatComp->OnComboWindowClosed.Broadcast();
}

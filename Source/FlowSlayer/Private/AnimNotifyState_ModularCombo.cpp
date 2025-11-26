#include "AnimNotifyState_ModularCombo.h"

void UAnimNotifyState_ModularCombo::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
		return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
		return;

	CombatComp = Owner->FindComponentByClass<UFSCombatComponent>();
	if (!CombatComp)
		return;

	CombatComp->OnModularComboWindowOpened.Broadcast();
}

void UAnimNotifyState_ModularCombo::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!CombatComp)
		return;

	CombatComp->OnModularComboWindowClosed.Broadcast();
}

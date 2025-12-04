#include "AirStallNotify.h"

void UAirStallNotify::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
		return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
		return;

	CombatComp = Owner->FindComponentByClass<UFSCombatComponent>();
	if (!CombatComp)
		return;

	UCharacterMovementComponent* movementComp{ Owner->FindComponentByClass<UCharacterMovementComponent>() };
	if (!movementComp)
		return;

	GravityScale = movementComp->GravityScale;

	CombatComp->OnAirStallStarted.Broadcast();
}

void UAirStallNotify::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!CombatComp)
		return;

	CombatComp->OnAirStallFinished.Broadcast(GravityScale);
}

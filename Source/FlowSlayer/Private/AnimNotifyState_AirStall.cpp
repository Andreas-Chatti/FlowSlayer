#include "AnimNotifyState_AirStall.h"

UAnimNotifyState_AirStall::UAnimNotifyState_AirStall()
{
	NotifyColor = FColor::Cyan;
}

void UAnimNotifyState_AirStall::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
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

void UAnimNotifyState_AirStall::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!CombatComp)
		return;

	CombatComp->OnAirStallFinished.Broadcast(GravityScale);
}

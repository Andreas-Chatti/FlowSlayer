#include "AnimNotifyState_AirStall.h"

UAnimNotifyState_AirStall::UAnimNotifyState_AirStall()
{
	NotifyColor = FColor::Cyan;
}

void UAnimNotifyState_AirStall::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
		return;

	AActor* Owner{ MeshComp->GetOwner() };
	if (!Owner)
		return;

	movementCompRef = Owner->FindComponentByClass<UCharacterMovementComponent>();
	if (!movementCompRef)
		return;

	movementCompRef->SetMovementMode(EMovementMode::MOVE_Flying);
}

void UAnimNotifyState_AirStall::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!movementCompRef)
		return;

	movementCompRef->SetMovementMode(EMovementMode::MOVE_Falling);
}

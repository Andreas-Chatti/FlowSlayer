#include "AnimNotifyState_RotateToTarget.h"
void UAnimNotifyState_RotateToTarget::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (!bSnapRotation)
		return;

	ACharacter* owningCharacter{ Cast<ACharacter>(MeshComp->GetOwner()) };
	if (!owningCharacter)
		return;

	if (bIsEnemy)
	{
		APawn* player{ UGameplayStatics::GetPlayerPawn(MeshComp->GetWorld(), 0) };
		if (!player)
			return;

		FVector directionToPlayer{ (player->GetActorLocation() - owningCharacter->GetActorLocation()).GetSafeNormal() };
		FRotator targetRotation{ 0.f, FRotationMatrix::MakeFromX(directionToPlayer).Rotator().Yaw, 0.f };
		owningCharacter->SetActorRotation(targetRotation, ETeleportType::TeleportPhysics);
	}

	else
		owningCharacter->SetActorRotation(FRotator{ 0.f, owningCharacter->GetControlRotation().Yaw, 0.f }, ETeleportType::TeleportPhysics);
}

void UAnimNotifyState_RotateToTarget::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (bSnapRotation)
		return;

	ACharacter* owningCharacter{ Cast<ACharacter>(MeshComp->GetOwner()) };
	if (!owningCharacter)
		return;

	FRotator newRotation{ FRotator::ZeroRotator };
	if (bIsEnemy)
	{
		APawn* player{ UGameplayStatics::GetPlayerPawn(MeshComp->GetWorld(), 0) };
		if (!player)
			return;

		FVector directionToPlayer{ (player->GetActorLocation() - owningCharacter->GetActorLocation()).GetSafeNormal() };
		FRotator targetRotation{ 0.f, FRotationMatrix::MakeFromX(directionToPlayer).Rotator().Yaw, 0.f };
		newRotation = FMath::RInterpTo(owningCharacter->GetActorRotation(), targetRotation, FrameDeltaTime, RotationSpeed);
	}

	else
		newRotation = FMath::RInterpTo(owningCharacter->GetActorRotation(), FRotator{ 0.f, owningCharacter->GetControlRotation().Yaw, 0.f }, FrameDeltaTime, RotationSpeed);

	owningCharacter->SetActorRotation(newRotation);
}

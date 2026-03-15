#include "AnimNotify_Launch.h"

UAnimNotify_Launch::UAnimNotify_Launch()
{
	NotifyColor = FColor::Yellow;
}

void UAnimNotify_Launch::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	ACharacter* owningCharacter{ Cast<ACharacter>(MeshComp->GetOwner()) };
	if (!owningCharacter)
		return;

	FVector velocity{ VelocityX, VelocityY, VelocityZ };
	owningCharacter->LaunchCharacter(std::move(velocity), bXYOverride, bZOverride);
}

#include "AnimNotifyState_Hitbox.h"

UAnimNotifyState_Hitbox::UAnimNotifyState_Hitbox()
{
    NotifyColor = FColor::Orange;
}

void UAnimNotifyState_Hitbox::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    if (!MeshComp)
        return;

    AActor* Owner{ MeshComp->GetOwner() };
    if (!Owner)
        return;

    HitboxComp = Owner->FindComponentByClass<UHitboxComponent>();
}

void UAnimNotifyState_Hitbox::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    if (HitboxComp)
        HitboxComp->OnActiveFrameStarted.Execute(&HitboxProfile);
}

void UAnimNotifyState_Hitbox::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    if (HitboxComp)
        HitboxComp->OnActiveFrameStopped.Execute();
}

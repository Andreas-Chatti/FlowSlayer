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

    CombatComp = Owner->FindComponentByClass<UFSCombatComponent>();
    if (CombatComp)
        CombatComp->GetEquippedWeapon()->OnHitboxActivated.Broadcast();

    EnemyInstigator = Cast<AFSEnemy>(Owner);
    if (EnemyInstigator)
        EnemyInstigator->OnHitboxActivated.Broadcast();
}

void UAnimNotifyState_Hitbox::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    if(CombatComp)
        CombatComp->GetEquippedWeapon()->OnHitboxDeactivated.Broadcast();

    else if(EnemyInstigator)
        EnemyInstigator->OnHitboxDeactivated.Broadcast();
}

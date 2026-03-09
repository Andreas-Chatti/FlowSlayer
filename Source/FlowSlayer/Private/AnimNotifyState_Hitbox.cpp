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
    {
        PlayerWeapon = CombatComp->GetEquippedWeapon();
        AttackData = CombatComp->GetOngoingAttack();
    }

    bool bIsPlayer{ CombatComp && PlayerWeapon && AttackData };
    if (!bIsPlayer)
    {
        EnemyInstigator = Cast<AFSEnemy>(Owner);
        if (EnemyInstigator)
            EnemyInstigator->OnHitboxActivated.Broadcast();
    }
}

void UAnimNotifyState_Hitbox::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    if (PlayerWeapon && AttackData)
        PlayerWeapon->OnActiveFrameStarted.Execute(AttackData->ActiveFrameRadius);
}

void UAnimNotifyState_Hitbox::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    if (PlayerWeapon)
        PlayerWeapon->OnActiveFrameStopped.Execute();

    else if (EnemyInstigator)
        EnemyInstigator->OnHitboxDeactivated.Broadcast();
}

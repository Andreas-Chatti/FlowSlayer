#include "ActivateHitboxNotify.h"

void UActivateHitboxNotify::Notify(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& EventReference)
{
    if (!meshComp || !meshComp->GetOwner())
        return;

    if (AFlowSlayerCharacter * character{ Cast<AFlowSlayerCharacter>(meshComp->GetOwner()) })
        character->GetCombatComponent()->OnHitboxActivated.Broadcast();

    else if (AFSEnemy * enemy{ Cast<AFSEnemy>(meshComp->GetOwner()) })
        enemy->OnHitboxActivated.Broadcast();
}

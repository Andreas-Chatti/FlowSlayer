#include "DeactivateHitboxNotify.h"

void UDeactivateHitboxNotify::Notify(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& EventReference)
{
    if (!meshComp || !meshComp->GetOwner())
        return;

    if (AFlowSlayerCharacter* character{ Cast<AFlowSlayerCharacter>(meshComp->GetOwner()) })
        character->GetCombatComponent()->OnHitboxDeactivated.Broadcast();

    else if (AFSEnemy * enemy{ Cast<AFSEnemy>(meshComp->GetOwner()) })
        enemy->OnHitboxDeactivated.Broadcast();
}

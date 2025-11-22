#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "../FlowSlayerCharacter.h"
#include "FSEnemy.h"
#include "ActivateHitboxNotify.generated.h"

UCLASS()
class FLOWSLAYER_API UActivateHitboxNotify : public UAnimNotify
{
    GENERATED_BODY()

public:

    virtual void Notify(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& EventReference) override;
};
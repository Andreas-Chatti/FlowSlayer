#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "../FlowSlayerCharacter.h"
#include "FSEnemy.h"
#include "DeactivateHitboxNotify.generated.h"

UCLASS()
class FLOWSLAYER_API UDeactivateHitboxNotify : public UAnimNotify
{
    GENERATED_BODY()

public:
    virtual void Notify(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& EventReference) override;
};
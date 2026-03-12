#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HealthComponent.h"
#include "FSDamageable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UFSDamageable : public UInterface
{
    GENERATED_BODY()
};

class FLOWSLAYER_API IFSDamageable
{
    GENERATED_BODY()

public:

    virtual UHealthComponent* GetHealthComponent() = 0;

    UFUNCTION()
    virtual void NotifyHitReceived(AActor* instigatorActor, const FAttackData& usedAttack) = 0;
};

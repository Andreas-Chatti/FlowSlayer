#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
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

    virtual void ReceiveDamage(float DamageAmount, AActor* DamageDealer) = 0;

    virtual bool IsDead() const = 0;
};

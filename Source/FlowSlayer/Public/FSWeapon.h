#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "FSDamageable.h"
#include "CombatData.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "FSWeapon.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class FLOWSLAYER_API AFSWeapon : public AActor
{
    GENERATED_BODY()

public:

    AFSWeapon();

    /** Trail component attached at weapon base socket */
    UNiagaraComponent* GetTrailNiagaraBaseComponent() const { return TrailNiagaraBaseComponent; }

    /** Trail component attached at weapon tip socket */
    UNiagaraComponent* GetTrailNiagaraTipComponent() const { return TrailNiagaraTipComponent; }

    FName GetBaseSocketName() const { return BaseSocket; }
    FName GetTipSocketName() const { return TipSocket; }

    FVector GetBaseSocketLocation() const { return WeaponMesh->GetSocketLocation(BaseSocket); }
    FVector GetTipSocketLocation() const { return WeaponMesh->GetSocketLocation(TipSocket); }

protected:

    /** Root component for the weapon actor */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootComp;

    /** Weapon mesh component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* WeaponMesh;

    /** Sword trail Niagara VFX component attached at weapon base socket */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UNiagaraComponent* TrailNiagaraBaseComponent{ nullptr };

    /** Sword trail Niagara VFX component attached at weapon tip socket */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UNiagaraComponent* TrailNiagaraTipComponent{ nullptr };

    /** Socket name at the base of the weapon where the hitbox starts */
    UPROPERTY(EditDefaultsOnly, Category = "Sockets")
    FName BaseSocket{ "S_WeaponBase" };

    /** Socket name at the tip of the weapon where the hitbox ends */
    UPROPERTY(EditDefaultsOnly, Category = "Sockets")
    FName TipSocket{ "S_WeaponTip" };

private:

    /** Initialize all weapon components in constructor */
    void InitializeComponents();
};

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

    void ActivateTrail() const { SwordTrailComponent->Activate(); }
    void DeactivateTrail() const { SwordTrailComponent->Deactivate(); }

    FVector GetBaseSocketLocation() const { return WeaponMesh->GetSocketLocation(BaseSocket); }
    FVector GetTipSocketLocation() const { return WeaponMesh->GetSocketLocation(TipSocket); }

protected:

    virtual void BeginPlay() override;

    /** Root component for the weapon actor */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootComp;

    /** Weapon mesh component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* WeaponMesh;

    /** Sword trail VFX component
    * Activated/deactivated when hitbox is enabled/disabled
    */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UNiagaraComponent* SwordTrailComponent{ nullptr };

    /** Sword trail VFX system asset */
    UPROPERTY(EditDefaultsOnly, Category = "VFX")
    UNiagaraSystem* SwordTrailSystem;

    /* Socket name of the base weapon where the hitbox starts 
    * Can be empty if there's no socket
    */
    UPROPERTY(EditDefaultsOnly, Category = "Sockets")
    FName BaseSocket{ "S_WeaponBase" };

    /* Socket name of the tip of the weapon where the hitbox ends
    * Can be empty if there's no socket
    */
    UPROPERTY(EditDefaultsOnly, Category = "Sockets")
    FName TipSocket{ "S_WeaponTip" };

private:

    /** Initialize all weapon components in constructor */
    void InitializeComponents();
};
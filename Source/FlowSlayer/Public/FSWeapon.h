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
#include "WeaponPartData.h"
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

    /**
     * Equips a weapon part and applies its stat to the weapon's cached multipliers.
     * If this slot already has a part, the previous tier's effect is first undone,
     * then the new part's effect is applied.
     */
    void EquipPart(EWeaponPartType PartType, const FWeaponPartData& PartData);

    /** Returns the currently equipped tier for a given slot (0 = not equipped, 1-3 = tier) */
    UFUNCTION(BlueprintPure, Category = "WeaponParts")
    int32 GetCurrentTier(EWeaponPartType PartType) const;

    /** Returns the cumulative damage multiplier contributed by all equipped weapon parts */
    UFUNCTION(BlueprintPure, Category = "WeaponParts")
    float GetDamageMultiplier() const { return WeaponDamageMultiplier; }

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

    /**
     * Maps each weapon slot to the tier currently equipped (0 = empty, 1 = T1, 2 = T2, 3 = T3).
     * Persists for the duration of the run; reset implicitly on OpenLevel (actor destroyed).
     */
    UPROPERTY(VisibleAnywhere, Category = "WeaponParts|State")
    TMap<EWeaponPartType, int32> EquippedPartTiers;

    /**
     * Maps each weapon slot to the FWeaponPartData of the currently equipped tier.
     * Retained so EquipPart() can reverse the previous tier's stat before applying the new one.
     * Must be UPROPERTY — FWeaponPartData contains a UTexture2D* that GC must track.
     */
    UPROPERTY()
    TMap<EWeaponPartType, FWeaponPartData> EquippedPartDataCache;

    /** Cumulative damage multiplier from all equipped weapon parts — starts at 1.0 (no contribution) */
    float WeaponDamageMultiplier{ 1.f };

    /** Applies a weapon part's stat to the appropriate multiplier — also used to reverse a previous tier */
    void ApplyPartStat(const FWeaponPartData& PartData);

    /** Builds a reversed copy of PartData (negates additive, inverts multiplicative) for the undo step */
    FWeaponPartData BuildReversePart(const FWeaponPartData& PartData) const;
};

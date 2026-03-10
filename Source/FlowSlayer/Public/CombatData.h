#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CombatData.generated.h"

UENUM(BlueprintType)
enum class EAttackType : uint8
{
    None,

    // Light combos
    StandingLight, // LMB
    RunningLight, // Z + LMB

    // Heavy combos
    StandingHeavy, // RMB
    RunningHeavy, // Z + RMB

    // Dash attacks
    DashPierce,           // SHIFT + Z + LMB
    DashSpinningSlash,    // SHIFT + Q/D + LMB
    DashDoubleSlash,      // SHIFT + Z + RMB
    DashBackSlash,        // SHIFT + S + RMB

    // Jump attacks
    JumpSlam,             // S + LMB (airborne)
    JumpForwardSlam,      // Z + LMB (airborne)
    JumpUpperSlam,        // Z + RMB (airborne)

    // Launcher attacks
    Launcher,             // A + LMB
    PowerLauncher,        // A + RMB

    // Spin attacks
    SpinAttack,           // E + LMB
    HorizontalSweep,      // E + RMB

    // Forward power attacks
    PowerSlash,           // F + S
    PierceThrust,         // F + Z

    // Slam attacks
    GroundSlam,           // S + RMB
    DiagonalRetourne,     // S + LMB

    // Air attacks
    AirCombo,             // LMB (airborne)
    AerialSlam            // RMB (airborne after launcher)
};

UENUM(BlueprintType)
enum class EHitboxShape : uint8
{
    WeaponSweep,
    Sphere,
    Cone,
    Box
};

/** Single attack data within a combo */
USTRUCT(BlueprintType)
struct FAttackData : public FTableRowBase
{
    GENERATED_BODY()

    DECLARE_DELEGATE(FOnBeforeAttack);
    DECLARE_DELEGATE(FOnAttackExecuted);
    DECLARE_DELEGATE_OneParam(FOnAttackHit, AActor* hitActor);

    /** Name of the attack
    * Used mostly for debugging
    */
    UPROPERTY(EditDefaultsOnly)
    FName Name{ "" };

    /** Animation montage for this attack */
    UPROPERTY(EditDefaultsOnly)
    UAnimMontage* Montage{ nullptr };

    /** Damage dealt by this attack
     * Configured via InitializeComboAttackData()
     */
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float Damage{ 50.f };

    /** Knockback force applied to enemy (0 = no knockback)
     * Configured via InitializeComboAttackData()
     */
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float KnockbackForce{ 0.f };

    /** Vertical knockback component (adds upward velocity)
     * Configured via InitializeComboAttackData()
     */
    UPROPERTY(EditAnywhere)
    float KnockbackUpForce{ 0.f };

    /** Flow reward on attack successfully hit target */
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float FlowReward{ 5.f };

    /** How long the player has to launch the next attack after a hit to successfully continue a true combo */
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
    float ComboWindowDuration{ 0.1f };

    UPROPERTY(EditAnywhere, Category = "Hitbox")
    EHitboxShape HitboxShape{ EHitboxShape::WeaponSweep };

    /** Zone offset from the player (local space) 
    * Can be ZeroVector (center being the player)
    */
    UPROPERTY(EditAnywhere, Category = "Hitbox", meta = (EditCondition = "HitboxShape != EHitboxShape::WeaponSweep"))
    FVector HitboxOffset{ FVector::ZeroVector };

    /** Radius of each active frame during an sweep attack */
    UPROPERTY(EditAnywhere, Category = "Hitbox", meta = (ClampMin = "0.0", EditCondition = "HitboxShape == EHitboxShape::WeaponSweep"))
    float SweepSphereRadius{ 10.f };

    /** Zone radius (Sphere, Cone) */
    UPROPERTY(EditAnywhere, Category = "Hitbox", meta = (ClampMin = "0.0", EditCondition = "HitboxShape != EHitboxShape::WeaponSweep"))
    float HitboxRange{ 200.f };

    /** Demi angle of the cone in degree (only for Cone) */
    UPROPERTY(EditAnywhere, Category = "Hitbox", meta = (ClampMin = "0.0", ClampMax = "180.0", EditCondition = "HitboxShape == EHitboxShape::Cone"))
    float ConeHalfAngle{ 45.f };

    /** Box extent (only for box) */
    UPROPERTY(EditAnywhere, Category = "Hitbox", meta = (EditCondition = "HitboxShape == EHitboxShape::Box"))
    FVector BoxExtent{ 100.f, 100.f, 50.f };

    /** Attack type needed to launch this attack
     * This data is based from the inputs actions in AFlowSlayerCharacter class
     * Configured via InitializeComboAttackData()
     */
    UPROPERTY(EditAnywhere)
    EAttackType AttackType{ EAttackType::None };

    /** Chainable attacks from this attack
     * Configured via InitializeComboAttackData()
     */
    UPROPERTY(EditAnywhere)
    TSet<EAttackType> ChainableAttacks;

    /* Called just before the attack animation plays
    * Use for setup logic: rotation, VFX prep, state changes, etc.
    * Can be empty
    */
    FOnBeforeAttack OnBeforeAttack;

    /* Attack side effect
    * An attack can have a specific effect
    * Called when the attack is executed
    * Can be empty (no side effect)
    */
    FOnAttackExecuted OnAttackExecuted;

    /* Called when attack hit an enemy
    * Can be empty (no side effect)
    */
    FOnAttackHit OnAttackHit;

    // === LATER (Phase 2+) ===
    // USoundBase* HitSound;
    // UNiagaraSystem* HitVFX;
    // float CustomHitstop;
    // etc.
};

/**
 * Combo data structure
 * Encapsulates all data related to a single combo chain.
 * Can contain one or more attacks (FAttackData)
 */
USTRUCT(BlueprintType)
struct FCombo
{
    GENERATED_BODY()

    /** Array of attack animations for this combo
     * Multiple montages (e.g., [Attack1, Attack2, Attack3])
     */
    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    TArray<FAttackData> Attacks;

    /** Returns the maximum combo index (last attack index in the array) */
    int32 GetMaxComboIndex() const { return FMath::Max(0, Attacks.Num() - 1); }

    /** Checks if this combo data is valid and ready to use */
    bool IsValid() const { return !Attacks.IsEmpty() && Attacks[0].Montage != nullptr; }

    /** Gets an attack montage at the specified index (returns nullptr if invalid) */
    const FAttackData* GetAttackAt(int32 Index) const
    {
        return Attacks.IsValidIndex(Index) ? &Attacks[Index] : nullptr;
    }

    /** Gets the last attack montage in the combo */
    const FAttackData* GetLastAttack() const
    {
        return !Attacks.IsEmpty() ? &Attacks.Last() : nullptr;
    }
};
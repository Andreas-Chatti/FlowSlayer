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
    DashDoubleSlash,      // SHIFT + F (ground)
    DashBackSlash,        // SHIFT + E (ground)

    // Jump attacks
    JumpSlam,             // E + Z (ground + air)
    JumpForwardSlam,      // SHIFT + A (ground + air)
    JumpUpperSlam,        // RMB + Z (ground + air) TODO: revoir les touches

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
enum class EAttackDataContext : uint8
{
    /** Attack can only be performed on the ground */
    Ground,

    /** Attack can only be performed while airborne */
    Air,

    /** Attack can be performed on ground or airborne */
    Any
};

/** Single attack data within a combo */
USTRUCT(BlueprintType)
struct FAttackData : public FTableRowBase
{
    GENERATED_BODY()

    DECLARE_DELEGATE(FOnAttackExecuted);
    DECLARE_DELEGATE_OneParam(FOnAttackHit, AActor* hitActor);

    /** Name of the attack
    * Used mostly for debugging
    */
    UPROPERTY(EditDefaultsOnly)
    FName Name{ "" };

    /** Animation montage for this attack */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
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

    /** Whether this attack is a ground or air attack
     * Used to validate attack execution against the player's current movement state
     */
    UPROPERTY(EditAnywhere)
    EAttackDataContext AttackContext{ EAttackDataContext::Ground };

    /* Attack side effect
    * An attack can have a specific effect
    * Called when the attack is executed
    * Can be empty (no side effect)
    */
    FOnAttackExecuted OnAttackExecuted;

    /** Delay before an attack can be used again */
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.1"))
    float CooldownDelay{ 5.f };

    /** State to check if that attack can be used or not */
    bool bOnCooldown{ false };

    /** TimerHandle of the cooldown */
    FTimerHandle CooldownTimer;

    void StartCooldown(UWorld* world)
    {
        if (!world)
            return;

        bOnCooldown = true;

        world->GetTimerManager().SetTimer(
            CooldownTimer,
            [this]() { bOnCooldown = false; },
            CooldownDelay,
            false
        );
    }

    // === LATER (Phase 2+) ===
    // USoundBase* HitSound;
    // UNiagaraSystem* HitVFX;
    // float CustomHitstop;
    // etc.
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHitReceived, AActor*, instigatorActor, const FAttackData&, usedAttack);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHitLanded, AActor*, hitActor, const FVector&, hitLocation, const FAttackData&, usedAttack);

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

    /** Returns the maximum combo index (last attack VALID index in the array) */
    int32 GetMaxComboIndex() const { return FMath::Max(0, Attacks.Num() - 1); }

    /** Checks if this combo data is valid and ready to use */
    bool IsValid() const { return !Attacks.IsEmpty() && Attacks[0].Montage != nullptr; }

    /** Gets an attack montage at the specified index (returns nullptr if invalid) */
    const FAttackData* GetAttackAt(int32 Index) const
    {
        return Attacks.IsValidIndex(Index) ? &Attacks[Index] : nullptr;
    }

    const FAttackData* GetFirstAttack() const
    {
        return Attacks.IsValidIndex(0) ? &Attacks[0] : nullptr;
    }

    /** Gets the last attack montage in the combo */
    const FAttackData* GetLastAttack() const
    {
        return !Attacks.IsEmpty() ? &Attacks.Last() : nullptr;
    }
};
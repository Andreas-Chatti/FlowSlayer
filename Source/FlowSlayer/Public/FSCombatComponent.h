#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/ActorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "Camera/CameraShakeBase.h"
#include "Logging/LogMacros.h"
#include "FSWeapon.h"
#include "FSDamageable.h"
#include "MotionWarpingComponent.h"
#include "EnhancedInputLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Camera/CameraComponent.h"
#include "FSCombatComponent.generated.h"

class USoundBase;
class UParticleSystem;
class UCameraShakeBase;

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

/** Delegates for combo window management - broadcasted by AnimNotifyState_ComboWindow */
DECLARE_MULTICAST_DELEGATE(FOnComboWindowOpened);
DECLARE_MULTICAST_DELEGATE(FOnComboWindowClosed);

/** Delegates for AirCombo air stall - broadcasted by AirStallNotify */
DECLARE_MULTICAST_DELEGATE(FOnAirStallStarted);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAirStallFinished, float gravityScale);

/** Delegate broadcasted when a successfull hit landed on an enemy target */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHitLanded, AActor*, actorHit, const FVector&, hitLocation, float, damageAmount, float, flowReward);


/** Single attack data within a combo */
USTRUCT(BlueprintType)
struct FAttackData
{
    GENERATED_BODY()

    DECLARE_DELEGATE(FOnAttackExecuted);
    DECLARE_DELEGATE_OneParam(FOnAttackHit, AActor* hitActor);

    /** Animation montage for this attack */
    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    UAnimMontage* Montage{ nullptr };

    /** Damage dealt by this attack
     * Configured via InitializeComboAttackData()
     */
    float Damage{ 50.f };

    /** Knockback force applied to enemy (0 = no knockback)
     * Configured via InitializeComboAttackData()
     */
    float KnockbackForce{ 0.f };

    /** Vertical knockback component (adds upward velocity)
     * Configured via InitializeComboAttackData()
     */
    float KnockbackUpForce{ 0.f };

    /** Flow reward on attack successfully hit target */
    float FlowReward{ 5.f };

    /** Attack type needed to launch this attack
     * This data is based from the inputs actions in AFlowSlayerCharacter class
     * Configured via InitializeComboAttackData()
     */
    EAttackType AttackType{ EAttackType::None };

    /** Chainable attacks from this attack
     * Configured via InitializeComboAttackData()
     */
    TSet<EAttackType> ChainableAttacks;

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComboCounterStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboCountChanged, int32, HitCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComboCounterEnded);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FLOWSLAYER_API UFSCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UFSCombatComponent();

    /** Combo window delegates
    * Broadcasted by ComboWindow notify state class to notify combo window open/close events
    */
    FOnComboWindowClosed OnComboWindowClosed;
    FOnComboWindowOpened OnComboWindowOpened;

    /** Air stall for air combos
    * Broadcasted by AirStallNotify to start an air stall for an air combo
    * Stop the air stall at the end
    */
    FOnAirStallStarted OnAirStallStarted;
    FOnAirStallFinished OnAirStallFinished;

    /** Hit landed delegate 
    * Broadcasted by OnHitLanded() when an sucessfull hit has been landed on an enemy target
    */
    UPROPERTY(BlueprintAssignable)
    FOnHitLanded OnHitLandedNotify;

    /** Broadcasted when a new hit streak begins (first hit) */
    UPROPERTY(BlueprintAssignable)
    FOnComboCounterStarted OnComboCounterStarted;

    /** Broadcasted each time a hit is added to the streak */
    UPROPERTY(BlueprintAssignable)
    FOnComboCountChanged OnComboCountChanged;

    /** Broadcasted when the streak timer expires (combo ended) */
    UPROPERTY(BlueprintAssignable)
    FOnComboCounterEnded OnComboCounterEnded;

    /** Returns the timer ratio (1.0 = full, 0.0 = expired) for the UI bar */
    UFUNCTION(BlueprintCallable, Category = "Combat|Combo Counter")
    float GetComboTimeRatio() const;

private:

    /** Duration of the combo streak timer — resets on each hit */
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Combo Counter")
    float ComboCounterTimerDuration{ 1.5f };

    /** Number of hits in the current streak */
    int32 ComboHitCount{ 0 };

    /** Remaining time before the streak ends */
    float ComboTimeRemaining{ 0.f };

    /** Whether a streak is currently active */
    bool bComboCounterActive{ false };

    /** Resets all combo counter state and broadcasts OnComboCounterEnded */
    void ResetComboCounter();

public:

    /** Called for attacking input */
    void Attack(EAttackType attackType, bool isMoving, bool isFalling);

    bool isAttacking() const { return bIsAttacking; }

    /** Enable or disable bIsAttacking flag */
    void SetIsAttacking(bool isAttacking) { bIsAttacking = isAttacking; }

    /** Returns whether we're currently chaining to a new combo */
    bool GetChainingToNewCombo() const { return bChainingToNewCombo; }

    /** Set all combos and combat states back to default and stops the current attack animation */
    void CancelAttack();

protected:

    virtual void BeginPlay() override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    TSubclassOf<AFSWeapon> weaponClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat VFX")
    float hitFlashDuration{ 0.15f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat VFX")
    UMaterialInterface* HitFlashMaterial;
    void ApplyHitFlash(AActor* hitActor);

public:

    AActor* GetNearestEnemyFromPlayer(float distanceRadius, bool debugLines = false) const;

    // === HIT REACTION ===

    /** Called by equippedWeapon (Player's weapon actor) 
    * Called when equippedWeapon's hitbox touches at least one AFSEnemy
    * Applies damage, hitstop, vfx, sfx and cameraShake
    */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void OnHitLanded(AActor* hitActor, const FVector& hitLocation);

    // === DAMAGE ===

    void ApplyDamage(AActor* target, AActor* instigator, float damageAmount);

    // === KNOCKBACK ===

    void ApplyKnockback(AActor* target, float KnockbackForce = 0.f, float UpKnockbackForce = 0.f);

    // === HITSTOP ===

    /** Freeze duration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Hitstop")
    float hitstopDuration{ 0.08f };

    /** Slowness (0 = total freeze) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Hitstop")
    float hitstopTimeDilation{ 0.05f }; // Ralentissement (0 = freeze total)

    void ApplyHitstop();

    // === VFX ===

    /** Hit Particules VFX */
    UPROPERTY(EditDefaultsOnly, Category = "VFX")
    TArray<UNiagaraSystem*> hitParticlesSystemArray;

    void SpawnHitVFX(const FVector& location);

    // === SFX ===

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat SFX")
    USoundBase* hitSound{ nullptr };

    void PlayHitSound(const FVector& location);

    // === CAMERA SHAKE ===

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Camera")
    TSubclassOf<UCameraShakeBase> hitCameraShake;

    void ApplyCameraShake();

    const AFSWeapon* GetEquippedWeapon() const { return equippedWeapon; }

    void SetLockedOnTargetRef(AActor* lockedOnTarget) { LockedOnTarget = lockedOnTarget; }

private:

    /** Player's Weapon reference */
    UPROPERTY()
    AFSWeapon* equippedWeapon;

    /** Player owner reference 
    * Owner of this Component 
    */
    UPROPERTY()
    ACharacter* PlayerOwner;

    /** Cached AnimInstance reference */
    UPROPERTY()
    UAnimInstance* AnimInstance;

    /** Cached MotionWarpingComponent reference */
    UPROPERTY()
    UMotionWarpingComponent* MotionWarpingComponent;

    /** Current locked-on target reference
    * nullptr if there's no target locked-on
    */
    UPROPERTY()
    AActor* LockedOnTarget;

    UPROPERTY()
    FTimerHandle hitstopTimerHandle;

    UFUNCTION()
    void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    void ResetTimeDilation();

    bool InitializeAndAttachWeapon();

    // === COMBAT - COMBO SYSTEM ===

    /** Standing light attack combo */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo StandingLightCombo;

    /** Standing heavy attack combo */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo StandingHeavyCombo;

    /** Running light attack combo */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo RunningLightCombo;

    /** Running heavy attack combo */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo RunningHeavyCombo;

    /** Dash Pierce attack */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo DashPierceAttack;

    /** Dash Spinning slash attack */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo DashSpinningSlashAttack;

    /** Dash Double slash attack */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo DashDoubleSlashAttack;

    /** Dash back slash attack */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo DashBackSlashAttack;

    /** LMB Air attack combo */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo AirCombo;

    /** Called when an air attack hits an enemy
    * Sets both player and enemy to Flying mode for air combo window
    */
    void OnAirAttackHit(AActor* hitEnemy);

    /** Freezes an enemy in mid-air by disabling gravity and stopping movement
    * @param enemyMovement The enemy's movement component to freeze
    */
    void FreezeEnemyInAir(TWeakObjectPtr<UCharacterMovementComponent> enemyMovement);

    /** Schedules an enemy to unfreeze after a delay, restoring normal falling physics
    * @param enemyMovement The enemy's movement component to unfreeze
    * @param delay Time in seconds before unfreezing the enemy
    */
    void ScheduleEnemyUnfreeze(TWeakObjectPtr<UCharacterMovementComponent> enemyMovement, float delay);

    /** Detects when a launched enemy reaches trajectory peak and freezes them for air combo window
    * Uses a looping timer to check velocity and height until peak is detected
    * @param enemy The enemy actor to track
    * @param enemyMovement The enemy's movement component
    * @param maxHeight Maximum allowed height gain before forcing freeze (safety limit)
    * @param freezeDuration How long to keep the enemy frozen for combo window
    */
    void FreezeEnemyAtTrajectoryPeak(AActor* enemy, UCharacterMovementComponent* enemyMovement, float maxHeight, float freezeDuration);

    /** SPACE + LMB air attack */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo JumpSlamAttack;

    /** SPACE + Z + LMB jump forward slam */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo JumpForwardSlamAttack;

    /** SPACE + RMB jump upper + slam combo */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo JumpUpperSlamComboAttack;

    /** A + LMB clean launcher */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo LauncherAttack;

    /** Called when launcher attack hits an enemy
    * Detects trajectory peak and freezes enemy in air for combo window
    * @param hitEnemy The enemy actor that was hit by the launcher
    */
    void OnLauncherAttackHit(AActor* hitEnemy);

    /** A + RMB power launcher */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo PowerLauncherAttack;

    /** E multi-hit spin attack */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo SpinAttack;

    /** E + RMB horizontal sweep */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo HorizontalSweepAttack;

    /** Z + Hold RMB power slash */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo PowerSlashAttack;

    /** Z + LMB (double tap) pierce thrust */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo PierceThrustAttack;

    /** S + RMB ground slam */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo GroundSlamAttack;

    /** S + LMB diagonal retourné */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo DiagonalRetourneAttack;

    /** RMB (airborne) aerial slam follow-up */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo AerialSlamAttack;

    /** Currently active combo (pointer to one of the above combos) */
    FCombo* OngoingCombo{ nullptr };

    /** Lookup table for fast combo selection based on attack type
     * Populated in InitializeComboLookupTable() during BeginPlay
     */
    TMap<EAttackType, FCombo*> ComboLookupTable;

    /** Initialize the combo lookup table with all available combos */
    void InitializeComboLookupTable();

    /** Initialize all combo attack data (damage, knockback, attack types, chainable attacks)
     * Called in PostInitProperties() after Blueprint montages are loaded
     */
    void InitializeComboAttackData();

    /** Is the character currently performing an attack? */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
    bool bIsAttacking{ false };

    /** Can the player input continue the combo? Set by AnimNotify_ComboWindow */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
    bool bComboWindowOpened{ false };

    /** Can the player input continue the combo? Set by AnimNotify_ComboWindow */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
    bool bContinueCombo{ false };

    /** Are we chaining to a new combo? */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
    bool bChainingToNewCombo{ false };

    /** Current attack index in the combo chain (0 = first attack, 1 = second, etc.) */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
    int32 ComboIndex{ 0 };

    /** Next combo to chain into */
    FCombo* PendingCombo{ nullptr };

    // === FUNCTIONS ===

    /** Select the correct Combo based on the current player's state (moving, falling, attack type) */
    FCombo* SelectComboBasedOnState(EAttackType attackType, bool isMoving, bool isFalling);

    /** Get the next attack montage in the combo sequence */
    UAnimMontage* GetComboNextAttack(const FCombo& combo);

    /** Advances to the next attack in the combo sequence */
    void ContinueCombo();

    /** Resets all combo state variables to their default values */
    void ResetComboState();

    // === COMBO WINDOW HANDLERS (Bound to delegates in BeginPlay) ===

    /** Called when MODULAR combo window opens via delegate broadcast */
    void HandleComboWindowOpened();

    /** Called when MODULAR combo window closes via delegate broadcast */
    void HandleComboWindowClosed();

    /** Called to start an air stall
    * Used for air combos
    */
    void HandleAirStallStarted();

    /** Called to stop an air stall
    * Used for air combos
    */
    void HandleAirStallFinished(float gravityScale);

    /** GravityScale during air stall
    * Lower value means less gravity = player staying in the air longer
    */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    float AirStallGravity{ 0.3f };

    /** Helper to get the target for motion warping
    * Prioritizes locked-on target if within radius, otherwise finds nearest enemy
    * @param searchRadius Maximum detection radius
    * @param debugLines Whether to show debug lines
    * @return Target actor or nullptr if none found
    */
    AActor* GetTargetForMotionWarp(float searchRadius, bool debugLines = false);

    /** Setup motion warp for air-based attacks (launcher, air combos, aerial slams)
    *
    * Creates a motion warp target toward the nearest enemy and synchronizes Flying movement mode
    * with the animation's MotionWarp notify state to allow smooth aerial tracking.
    *
    * @param notifyStartTime When the MotionWarp notify state STARTS in the animation montage (seconds).
    *                        MOVE_Flying is activated at this exact moment to disable gravity.
    *
    * @param notifyEndTime When the MotionWarp notify state ENDS in the animation montage (seconds).
    *                      MOVE_Falling is restored at this exact moment to re-enable gravity.
    *
    * @param zOffset Vertical offset added to enemy position (positive = higher, negative = lower).
    *                Use this to aim above the enemy (e.g., 150.f for launcher attacks).
    *
    * @param forwardOffset Forward distance added in front of the player's facing direction.
    *                      Use this to land slightly in front of the enemy instead of directly on them.
    *
    * @param searchRadius Maximum detection radius from player position to find the nearest enemy (cm).
    *                     Defaults to 250.f. Motion warp is skipped if no enemy is found within radius.
    */
    void SetupAirAttackMotionWarp(FName motionWarpingTargetName, float notifyStartTime, float notifyEndTime, float searchRadius, bool debugLines = false, float zOffset = 0.f, float forwardOffset = 0.f);

    /** Setup motion warp for ground-based attacks (dash attacks, launcher ground phase)
    *
    * Creates a motion warp target toward the nearest enemy for ground tracking.
    * Unlike air attacks, this does NOT change movement mode (stays in MOVE_Walking).
    *
    * @param notifyStartTime When the MotionWarp notify state STARTS in the animation montage (seconds).
    * @param notifyEndTime When the MotionWarp notify state ENDS in the animation montage (seconds).
    * @param forwardOffset Forward distance added in front of the player's facing direction.
    * @param searchRadius Maximum detection radius from player position to find the nearest enemy (cm).
    * @param motionWarpingTargetName Name of the warp target to create (must match notify state).
    */
    void SetupGroundAttackMotionWarp(FName motionWarpingTargetName, float notifyStartTime, float notifyEndTime, float searchRadius, bool debugLines = false, float forwardOffset = 0.f);

    /** Transitions from current combo to a different pending combo
    * Replaces the ongoing combo with the pending combo and starts it from the first attack
    * Used when chaining between different combo types (e.g., StandingLight -> RunningHeavy)
    */
    void ChainingToNextCombo();

};
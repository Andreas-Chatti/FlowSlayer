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
#include "FSFocusable.h"
#include "FSDamageable.h"
#include "EnhancedInputLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Camera/CameraComponent.h"
#include "FSCombatComponent.generated.h"

class USoundBase;
class UParticleSystem;
class UCameraShakeBase;

/** Delegates Used to activate or deactivate equippedWeapon hitbox */
DECLARE_MULTICAST_DELEGATE(FOnHitboxActivated);
DECLARE_MULTICAST_DELEGATE(FOnHitboxDeactivated);

/** Delegates for Modular combo window management - broadcasted by AnimNotifyState_ModularCombo */
DECLARE_MULTICAST_DELEGATE(FOnModularComboWindowOpened);
DECLARE_MULTICAST_DELEGATE(FOnModularComboWindowClosed);

/** Delegates for Full combo window management - broadcasted by AnimNotifyState_FullCombo */
DECLARE_MULTICAST_DELEGATE(FOnFullComboWindowOpened);
DECLARE_MULTICAST_DELEGATE(FOnFullComboWindowClosed);

/** Delegates for AirCombo air stall - broadcasted by AirStallNotify */
DECLARE_MULTICAST_DELEGATE(FOnAirStallStarted);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAirStallFinished, float gravityScale);

/** Delegate when lock-on is stopped */
DECLARE_MULTICAST_DELEGATE(FOnLockOnStopped);


/** Single attack data within a combo */
USTRUCT(BlueprintType)
struct FAttackData
{
    GENERATED_BODY()

    /** Animation montage for this attack */
    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    UAnimMontage* Montage{ nullptr };

    /** Damage dealt by this attack */
    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float Damage{ 50.f };

    /** Knockback force applied to enemy (0 = no knockback) */
    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float KnockbackForce{ 0.f };

    /** Vertical knockback component (adds upward velocity) */
    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float KnockbackUpForce{ 0.f };

    /** Attack name for debugging */
    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    FName AttackName{ NAME_None };

    /** Input list to trigger this attack */
    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    UInputAction* RequiredInput;

    // === LATER (Phase 2+) ===
    // USoundBase* HitSound;
    // UNiagaraSystem* HitVFX;
    // float CustomHitstop;
    // etc.
};


/**
 * Combo data structure
 *
 * Encapsulates all data related to a single combo chain.
 * Supports both MODULAR combos (multiple montages) and FULL combos (single montage).
 */
USTRUCT(BlueprintType)
struct FCombo
{
    GENERATED_BODY()

    /** Array of attack animations for this combo
     * - MODULAR combo: Multiple montages (e.g., [Attack1, Attack2, Attack3])
     * - FULL combo: Single montage containing entire combo sequence
     */
    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    TArray<FAttackData> Attacks;

    /** Combo name for debugging purposes */
    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    FName ComboName{ "NAME_None" };

    /** Returns the maximum combo index (last attack index in the array) */
    int32 GetMaxComboIndex() const { return FMath::Max(0, Attacks.Num() - 1); }

    /** Returns true if this is a FULL combo (single montage with entire sequence) */
    bool IsFullCombo() const { return Attacks.Num() == 1; }

    /** Returns true if this is a MODULAR combo (multiple separate montages) */
    bool IsModularCombo() const { return Attacks.Num() > 1; }

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

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FLOWSLAYER_API UFSCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    enum class EAttackType
    {
        Light,
        Heavy,
        HeavySpecial,
    };

    UFSCombatComponent();

    /** Event delegates notify
    * Notified during a MELEE attack Animation
    */
    FOnHitboxActivated OnHitboxActivated;
    FOnHitboxDeactivated OnHitboxDeactivated;

    /** MODULAR Combo window delegates
    * Broadcasted by AnimNotifyState_ModularCombo to notify modular combo window open/close events
    */
    FOnModularComboWindowClosed OnModularComboWindowClosed;
    FOnModularComboWindowOpened OnModularComboWindowOpened;

    /** FULL Combo window delegates
    * Broadcasted by AnimNotifyState_FullCombo to notify full (1 full animation) combo window open/close events
    */
    FOnFullComboWindowOpened OnFullComboWindowOpened;
    FOnFullComboWindowClosed OnFullComboWindowClosed;

    /** Air stall for air combos 
    * Broadcasted by AirStallNotify to start an air stall for an air combo
    * Stop the air stall at the end
    */
    FOnAirStallStarted OnAirStallStarted;
    FOnAirStallFinished OnAirStallFinished;

    /** Broadcasted when the lock-on is stopped or interrupted by distance or target's death */
    FOnLockOnStopped OnLockOnStopped;

    /** Called for attacking input */
    void Attack(UInputAction* inputAction, bool isMoving, bool isFalling);

    bool isAttacking() const { return bIsAttacking; }

    /** Enable or disable bIsAttacking flag */
    void SetIsAttacking(bool isAttacking) { bIsAttacking = isAttacking; }

    /** Set all combos and combat states back to default and stops the current attack animation */
    void CancelAttack();

protected:

    virtual void BeginPlay() override;

    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    TSubclassOf<AFSWeapon> weaponClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat VFX")
    float hitFlashDuration{ 0.15f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat VFX")
    UMaterialInterface* HitFlashMaterial;
    void ApplyHitFlash(AActor* hitActor);

public:

    // === HIT REACTION ===

    // Appeler quand on HIT quelque chose
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void OnHitLanded(AActor* hitActor, const FVector& hitLocation);

    // === DAMAGE ===

    void ApplyDamage(AActor* target, AActor* instigator, float damageAmount);

    // === KNOCKBACK ===

    void ApplyKnockback(AActor* target, float KnockbackForce = 0.f, float UpKnockbackForce = 0.f);

    // === HITSTOP ===

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Hitstop")
    float hitstopDuration{ 0.1f }; // Dur�e du freeze

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

private:

    /** Player's Weapon */
    UPROPERTY()
    AFSWeapon* equippedWeapon;

    /** Owner of this Component */
    UPROPERTY()
    ACharacter* PlayerOwner;

    /** Cached AnimInstance reference */
    UPROPERTY()
    UAnimInstance* AnimInstance;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerStats", meta = (AllowPrivateAccess = "true"))
    float Damage{ 50.f };

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

    /** Running heavy attack combo */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo AirCombo;

    /** Currently active combo (pointer to one of the above combos) */
    FCombo* OngoingCombo{ nullptr };

    /** Is the character currently performing an attack? */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
    bool bIsAttacking{ false };

    /** Can the player input continue the combo? Set by AnimNotify_ComboWindow */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
    bool bComboWindowOpened{ false };

    /** Can the player input continue the combo? Set by AnimNotify_ComboWindow */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
    bool bContinueCombo{ false };

    /** Current attack index in the combo chain (0 = first attack, 1 = second, etc.) */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
    int32 ComboIndex{ 0 };

    /** List of valid targets currently in lock-on detection radius 
    * Only the nearest target is locked-on
    */
    UPROPERTY()
    TSet<AActor*> TargetsInLockOnRadius{};

    /** Current locked-on target nearest to the player */
    UPROPERTY()
    AActor* CurrentLockedOnTarget{ nullptr };

    /** IFSDamageable version of the current locked-on target
    * Mainly to access IsDead() method to disengage lock-on when target dies
    */
    IFSDamageable* CachedDamageableLockOnTarget{ nullptr };

    /** Radius where focusable targets can be detected and locked-on */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
    float LockOnDetectionRadius{ 2000.f };

    /** TRUE if player is locked-on to a target */
    bool bIsLockedOnEngaged{ false };

    /** Disable the lock-on if the target is outside the detection radius 
    * Is call every LockOnDistanceCheckDelay in EngageLockOn()
    * If distance between player and locked-on target is >= LockOnDetectionRadius
    * OR target is dead
    * Calls DisengageLockOn() which deactivate the LockOnValidCheckTimer to stop this method running every 
    */
    void LockOnValidCheck();

    /** Update Pitch and Yaw rotation of the camera every frame (called in Tick) based on the locked-on target distance from the player 
    * Yaw and pitch values will be interpolated based on LockOnDetectionRadius / 2
    * At minimum distance yaw and pitch offset will be equal to CloseCameraPitchOffset and CloseCameraYawOffset
    * At maximum distance (LockOnDetectionRadius / 2), yaw and pitch offset will be equal to FarCameraPitchOffset and FarCameraYawOffset
    * Check whether the target is on the right or left side of the screen and adjust dynamically the yaw (negative or positive) based on the target's position
    */
    void UpdateLockOnCamera(float deltaTime);

    /** Min pitch offset when locked-on FAR from target */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
    float FarCameraPitchOffset{ -10.0f };

    /** Max pitch offset when locked-on CLOSE to target */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
    float CloseCameraPitchOffset{ -5.0f };

    /** Max yaw offset when locked-on CLOSE an enemy */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
    float CloseCameraYawOffset{ 30.0f };

    /** Min yaw offset when locked-on FAR an enemy */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
    float FarCameraYawOffset{ 0.0f };

    /** Camera rotation interp speed */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
    float CameraRotationInterpSpeed{ 8.0f };

public:
    /** Delay in which the player can switch lock-on in-between targets */
    const float targetSwitchDelay{ 0.65f };

    /** Sensibility on the X axis LEFT and RIGHT when the Player moves the mouse to switch lock-on target */
    const double XAxisSwitchSensibility{ 1.0 };

private:
    /** Timer responsible for the lock-on delay in-between targets */
    UPROPERTY()
    FTimerHandle delaySwitchLockOnTimer;

    UPROPERTY()
    FTimerHandle LockOnValidCheckTimer;

    /** Delay in-between each distance checks */
    float LockOnDistanceCheckDelay{ 0.5f };

public:
    /** Whether the player is currently locked on a valid target */
    UFUNCTION(BlueprintCallable)
    bool IsLockedOnTarget() const { return bIsLockedOnEngaged; }

    /** @return Current locked-on target or nullptr if there's none */
    const AActor* GetCurrentLockedOnTarget() const { return CurrentLockedOnTarget; }

    /** Lock-on the nearest target in a specific sphere radius the character being the center
    * @return FALSE if no valid target is found
    * @return TRUE if a valid target is found
    */
    bool EngageLockOn();

    /** Switch lock-on target based on camera look direction
    * @param followCamera Camera component used to determine direction
    * @param axisValueX Mouse/controller X axis input (negative = left, positive = right)
    * @return TRUE if successfully switched to new target, FALSE otherwise
    */
    UFUNCTION(BlueprintCallable)
    bool SwitchLockOnTarget(float axisValueX);

    /** Stop the lock-on */
    void DisengageLockOn();

private:

    // === FUNCTIONS ===

    /** Select the correct Combo based on the current player's state (moving, falling, attack type) */
    FCombo* SelectComboBasedOnState(UInputAction* inputAction, bool isMoving, bool isFalling);

    /** Get the next attack montage in the combo sequence */
    UAnimMontage* GetComboNextAttack(const FCombo& combo);

    /** Advances to the next attack in the combo sequence */
    void ContinueCombo();

    /** Resets all combo state variables to their default values */
    void StopCombo();

    // === COMBO WINDOW HANDLERS (Bound to delegates in BeginPlay) ===

    /** Called when MODULAR combo window opens via delegate broadcast */
    void HandleModularComboWindowOpened();

    /** Called when MODULAR combo window closes via delegate broadcast */
    void HandleModularComboWindowClosed();

    /** Called when FULL combo window opens via delegate broadcast */
    void HandleFullComboWindowOpened();

    /** Called when FULL combo window closes via delegate broadcast */
    void HandleFullComboWindowClosed();

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
};
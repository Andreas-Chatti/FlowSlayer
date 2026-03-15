#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/ActorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"
#include "FSWeapon.h"
#include "CombatData.h"
#include "FSDamageable.h"
#include "HitboxComponent.h"
#include "HitFeedbackComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "FSCombatComponent.generated.h"

class USoundBase;
class UParticleSystem;
class UCameraShakeBase;

/** Delegates for combo window management - broadcasted by AnimNotifyState_ComboWindow */
DECLARE_DELEGATE(FOnComboInputWindowOpened);
DECLARE_DELEGATE(FOnComboInputWindowClosed);

/** Delegate broadcasted when any attack starts or ends */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackingStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackingEnded);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComboCounterStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboCountChanged, int32, HitCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComboCounterEnded);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FLOWSLAYER_API UFSCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UFSCombatComponent();

    /* Attacking started delegate
    * Broadcasted by UFSCombatComponent
    * On any attack started
    */
    UPROPERTY(BlueprintAssignable)
    FOnAttackingStarted OnAttackingStarted;

    /* Attacking ended delegate
    * Broadcasted by UFSCombatComponent
    * On any attack ended
    */
    UPROPERTY(BlueprintAssignable)
    FOnAttackingEnded OnAttackingEnded;

    /** Combo window delegates
    * Broadcasted by ComboWindow notify state class to notify combo window open/close events
    */
    FOnComboInputWindowOpened OnComboInputWindowOpened;
    FOnComboInputWindowClosed OnComboInputWindowClosed;

    /** Hit landed delegate 
    * Broadcasted by OnHitLanded() when an sucessfull hit has been landed on an enemy target
    */
    UPROPERTY(BlueprintAssignable)
    FOnHitLanded OnHitLanded;

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

    /** Whether the play can do an Air attack while in the air
    * 
    * Used mainly to track if the player already did an air attack
    * 
    * @return true if the player can do an air attack, false otherwise
    */
    UFUNCTION(BlueprintPure, Category = "Combat|Combo Counter")
    bool CanAirAttack() const { return bCanAirAttack; }

    /** @return Current used combo 
    * None if the player isn't attacking
    */
    const FCombo* GetOngoingCombo() const { return OngoingCombo; }

    /** @return Current used attack in the current combo */
    const FAttackData* GetOngoingAttack() const 
    { 
        if (OngoingCombo)
            return OngoingCombo->GetAttackAt(ComboIndex);
        else
            return nullptr;
    }

private:

    /** Duration of the combo streak timer — resets on each hit */
    float OngoingAttackComboWindowDuration{ 0.f };

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
    void OnAttackInputReceived(EAttackType attackType);

    bool IsAttacking() const { return bIsAttacking; }

    /** Returns whether we're currently chaining to a new combo */
    bool GetChainingToNewCombo() const { return bChainingToNewCombo; }

    /** Set all combos and combat states back to default and stops the current attack animation */
    void CancelAttack(float blendOutTime = 0.2f);

    /** Return all datas of a specific attack */
    FAttackData* GetAttackData(FName rowName) const;

    const UHitboxComponent* GetHitboxComponent() const { return HitboxComponent; }

    UHitFeedbackComponent* GetHitFeedbackComponent() const { return HitFeedBackComponent; }

protected:

    virtual void BeginPlay() override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    TSubclassOf<AFSWeapon> weaponClass;

    UPROPERTY(EditDefaultsOnly)
    UDataTable* AttackDataTable;

    UPROPERTY(VisibleAnywhere)
    UHitboxComponent* HitboxComponent;

    UPROPERTY(VisibleAnywhere)
    UHitFeedbackComponent* HitFeedBackComponent;

public:

    // === HIT REACTION ===

    /** Called by HitboxComponent (UHitboxComponent) 
    * Called upon target hit
    * Applies damage, hitstop, vfx, sfx and cameraShake
    */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void HandleOnHitLanded(AActor* hitActor, const FVector& hitLocation);

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

    /** Current locked-on target reference
    * nullptr if there's no target locked-on
    */
    UPROPERTY()
    AActor* LockedOnTarget;

    bool InitializeAndAttachWeapon();

    // === COMBAT - COMBO SYSTEM ===

    /** Standing light attack combo */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo StandingLightCombo;

    /** Standing heavy attack combo */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo StandingHeavyCombo;

    /** Running light attack combo */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo RunningLightCombo;

    /** Running heavy attack combo */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo RunningHeavyCombo;

    /** Dash Pierce attack */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo DashPierceAttack;

    /** Dash Spinning slash attack */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo DashSpinningSlashAttack;

    /** Dash Double slash attack */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo DashDoubleSlashAttack;

    /** Dash back slash attack */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo DashBackSlashAttack;

    /** LMB Air attack combo */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo AirCombo;

    /** SPACE + LMB air attack */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo JumpSlamAttack;

    /** SPACE + Z + LMB jump forward slam */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo JumpForwardSlamAttack;

    /** SPACE + RMB jump upper + slam combo */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo JumpUpperSlamComboAttack;

    /** A + LMB clean launcher */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo LauncherAttack;

    /** A + RMB power launcher */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo PowerLauncherAttack;

    /** E multi-hit spin attack */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo SpinAttack;

    /** E + RMB horizontal sweep */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo HorizontalSweepAttack;

    /** Z + Hold RMB power slash */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo PowerSlashAttack;

    /** Z + LMB (double tap) pierce thrust */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo PierceThrustAttack;

    /** S + RMB ground slam */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo GroundSlamAttack;

    /** S + LMB diagonal retourné */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
    FCombo DiagonalRetourneAttack;

    /** RMB (airborne) aerial slam follow-up */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combos", meta = (AllowPrivateAccess = "true"))
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
    bool bComboInputWindowOpen{ false };

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
    FCombo* GetComboFromContext(EAttackType attackType);

    /** Advances to the next attack in the combo sequence */
    void ContinueCombo();

    /* Execute the anim montage attack and the ongoingAttack delegates */
    void ExecuteAttack(UAnimMontage* attackMontage);

    /** Called when a valid attack input is received during an opened combo input window */
    void OnComboWindowInputReceived(EAttackType attackType);

    /** Resets all combo state variables to their default values */
    void ResetComboState();

    // === COMBO WINDOW HANDLERS (Bound to delegates in BeginPlay) ===

    /** Called when combo input window opens via delegate broadcast */
    void HandleComboInputWindowOpened();

    /** Called when combo input window closes via delegate broadcast */
    void HandleComboInputWindowClosed();

    /** Called when the character has landed on the ground */
    UFUNCTION()
    void HandleOnLanded(const FHitResult& Hit);

    /** Whether the play can do an Air attack while in the air
    * Used mainly to track if the player already did an air attack
    * Prevent from spamming multiple air attacks
    */
    bool bCanAirAttack{ true };

    /** Transitions from current combo to a different pending combo
    * Replaces the ongoing combo with the pending combo and starts it from the first attack
    * Used when chaining between different combo types (e.g., StandingLight -> RunningHeavy)
    */
    void ChainingToNextCombo();
};
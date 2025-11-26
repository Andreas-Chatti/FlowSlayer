#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/ActorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "Camera/CameraShakeBase.h"
#include "Logging/LogMacros.h"
#include "FSWeapon.h"
#include "EnhancedInputLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "FSCombatComponent.generated.h"

class USoundBase;
class UParticleSystem;
class UCameraShakeBase;

/** Delegates Used to activate or deactivate equippedWeapon hitbox */
DECLARE_MULTICAST_DELEGATE(FOnHitboxActivated);
DECLARE_MULTICAST_DELEGATE(FOnHitboxDeactivated);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FLOWSLAYER_API UFSCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    enum class EAttackType
    {
        Light,
        Heavy
    };

    UFSCombatComponent();

    /** Event delegates notify
    * Notified during a MELEE attack Animation
    */
    FOnHitboxActivated OnHitboxActivated;
    FOnHitboxDeactivated OnHitboxDeactivated;

    /** Called for attacking input */
    void Attack(EAttackType attackTypeInput, bool isMoving, bool isFalling);

    bool isAttacking() const { return bIsAttacking; }

protected:

    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    TSubclassOf<AFSWeapon> weaponClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat VFX")
    float hitFlashDuration{ 0.15f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat VFX")
    UMaterialInterface* HitFlashMaterial;
    void ApplyHitFlash(AActor* hitActor);

public:

    // === HIT REACTION ===

    // Appel�e quand on HIT quelque chose
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void OnHitLanded(AActor* hitActor, const FVector& hitLocation);

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

    /** Animation montage containing all combo attack sections */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
    TArray<UAnimMontage*> StandingLightCombo{ nullptr };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
    TArray<UAnimMontage*> StandingHeavyCombo{ nullptr };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
    TArray<UAnimMontage*> RunningLightCombo{ nullptr };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
    TArray<UAnimMontage*> RunningHeavyCombo{ nullptr };

    /** Character ongoing combo */
    TArray<UAnimMontage*>* OngoingCombo{ nullptr };

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

    /** Maximum number of attacks in this combo chain */
    UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo", meta = (AllowPrivateAccess = "true"))
    int32 MaxComboIndex{ 0 };

    // === FUNCTIONS ===

    /** Select the correct Combo Array based on the current player's state */
    TArray<UAnimMontage*>* SelectComboBasedOnState(EAttackType attackTypeInput, bool isMoving, bool isFalling);

    /** Start a combo */
    UAnimMontage* GetComboNextAttack(TArray<UAnimMontage*> combo);

    /** Advances to the next attack in the combo sequence */
    void ContinueCombo();

    /** Resets all combo state variables to their default values */
    void ResetCombo();

    // === ANIM NOTIFIES ===

    /** Called by animation notify to open the combo input window */
    UFUNCTION()
    void AnimNotify_ComboWindow(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);

    /** Called by animation notify when the attack animation ends */
    UFUNCTION()
    void AnimNotify_AttackEnd(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);
};
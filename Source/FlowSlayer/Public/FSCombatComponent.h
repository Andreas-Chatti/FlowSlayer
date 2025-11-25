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

    UFSCombatComponent();

    /** Event delegates notify
    * Notified during a MELEE attack Animation
    */
    FOnHitboxActivated OnHitboxActivated;
    FOnHitboxDeactivated OnHitboxDeactivated;

    /** Called for attacking input */
    void Attack(bool isMoving, bool isFalling);

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

    // Appelée quand on HIT quelque chose
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void OnHitLanded(AActor* hitActor, const FVector& hitLocation);

    // === HITSTOP ===

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Hitstop")
    float hitstopDuration{ 0.1f }; // Durée du freeze

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

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerStats", meta = (AllowPrivateAccess = "true"))
    float Damage{ 50.f };

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* RunningAttackMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations", meta = (AllowPrivateAccess = "true"))
    UAnimMontage* IdleAttackMontage;

    UPROPERTY()
    FTimerHandle hitstopTimerHandle;

    UFUNCTION()
    void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    void ResetTimeDilation();

    bool InitializeAndAttachWeapon();

    bool bIsAttacking{ false };
};
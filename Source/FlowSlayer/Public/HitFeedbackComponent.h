#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HitFeedbackComponent.generated.h"

/**
 * Handles all hit feedback effects for its owner (hitstop, hit shake, hit flash, knockback, VFX, SFX, camera shake).
 * Used by both player and enemies. Each instance only applies effects to the actor that owns it.
 *
 * - Call OnLandHit() on the attacker's component when a hit is confirmed
 * - Call OnReceiveHit() on the victim's component when they receive a hit
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FLOWSLAYER_API UHitFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UHitFeedbackComponent();

    /** Called on the ATTACKER's component when a hit is confirmed. Applies owner-side effects (mesh shake, VFX, SFX, camera shake, hitstop) */
    void OnLandHit(const FVector& hitLocation);

    /** Called on the VICTIM's component when they receive a hit. Applies owner-side effects (knockback, hitstop, hit shake, hit flash) */
    void OnReceiveHit(const FVector& attackerLocation, float knockbackForce = 0.f, float upKnockbackForce = 0.f);

protected:

	virtual void BeginPlay() override;

private:

    /** Cached character reference of the owner */
    ACharacter* OwnerCharacter{ nullptr };

private:

    // === KNOCKBACK ===

    /** Launches the owner away from the attacker's location */
    void ApplyKnockback(const FVector& attackerLocation, float knockbackForce, float upKnockbackForce);

    // === HITSTOP ===

    /** Applies time dilation to the owner for hitstopDuration seconds */
    void ApplyHitstop();

    /** Time dilation applied to the owner during hitstop (< 1 = slow motion) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitstop", meta = (AllowPrivateAccess = "true"))
    float HitstopTimeDilation{ 0.35f };

    /** Duration of the hitstop freeze in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hitstop", meta = (AllowPrivateAccess = "true"))
    float HitstopDuration{ 0.25f };

    // === HITSHAKE ===

    /** Shakes the owner's skeletal mesh along the camera right axis */
    void ApplyHitShake(float amplitude);

    /** Speed of the shake oscillation */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitShake", meta = (AllowPrivateAccess = "true"))
    float ShakeSpeed{ 30.f };

    /** Amplitude of the shake when taking any damage
    * Only applied to self
    */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitShake", meta = (AllowPrivateAccess = "true"))
    float ReceiveShakeAmplitude{ 20.f };

    /** Amplitude of the shake when landing damage 
    * Only applied to self
    */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HitShake", meta = (AllowPrivateAccess = "true"))
    float LandedShakeAmplitude{ 1.f };

    /** Active hit shake timer handle */
    UPROPERTY()
    FTimerHandle HitShakeTimer;

    // === HITFLASH ===

    /** Overlay material applied to the owner's mesh on hit */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitFlash", meta = (AllowPrivateAccess = "true"))
    UMaterialInterface* HitFlashMaterial{ nullptr };

    /** Speed parameter passed to the hit flash material */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitFlash", meta = (AllowPrivateAccess = "true"))
    float HitFlashSpeed{ 20.f };

    /** Applies the hit flash overlay material to the owner's mesh */
    void ApplyHitFlash();

    // === HIT VFX ===

    /** Pool of Niagara systems to pick from randomly on hit */
    UPROPERTY(EditDefaultsOnly, Category = "HitVFX", meta = (AllowPrivateAccess = "true"))
    TArray<UNiagaraSystem*> HitParticlesSystemArray;

    /** Spawns a random hit VFX at the given world location */
    void SpawnHitVFX(const FVector& location);

    // === SFX ===

    /** Sound played at the hit location */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SFX", meta = (AllowPrivateAccess = "true"))
    USoundBase* HitSound{ nullptr };

    /** Plays the hit sound at the given world location */
    void PlayHitSound(const FVector& location);

    // === CAMERA SHAKE ===

    /** Camera shake class applied on hit */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UCameraShakeBase> HitCameraShake;

    /** Triggers the camera shake on the local player controller */
    void ApplyCameraShake();
};

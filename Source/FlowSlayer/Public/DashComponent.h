#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "DashComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDashStarted, float flowCost);
DECLARE_MULTICAST_DELEGATE(FOnDashEnded);

DECLARE_DELEGATE_RetVal_OneParam(bool, FCanAffordDash, float flowCost);

/**
 * Handles the player's dash movement.
 *
 * Moves the character along a curve-driven trajectory over a fixed duration.
 * Input direction is snapped to 8 directions and converted to world space
 * relative to the camera. Dash is blocked during attacks, cooldown, and while airborne.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UDashComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UDashComponent();

    /** Initiates a dash in the given 2D input direction (raw axis values) */
    void StartDash(const FVector2D& inputDirection);

    virtual void BeginPlay() override;

    virtual void TickComponent(float deltaTime, ELevelTick tickType,
        FActorComponentTickFunction* thisTickFunction) override;

    /** Broadcast when the dash starts — used to trigger animations, VFX, etc. */
    FOnDashStarted OnDashStarted;

    /** Broadcast when the dash ends — used to restore movement state */
    FOnDashEnded OnDashEnded;

    /** Broadcast to check whether the player has enough flow to perform a dash */
    FCanAffordDash CanAffordDash;

    UFUNCTION(BlueprintPure)
    bool IsDashing() const { return bIsDashing; }

    /** Returns the world-space normalized direction of the current or last dash */
    UFUNCTION(BlueprintPure)
    FVector GetDashDirectionWorld() const { return DashDirectionWorld; }

    UFUNCTION(BlueprintPure)
    bool CanDash() const;

    UFUNCTION(BlueprintPure)
    bool IsOnCooldown() const { return bIsOnCooldown; }

    /** Internal method called by delegate OnAttackingStarted in UFSCombatComponent */
    UFUNCTION()
    void OnAttackingStarted();

    /** Internal method called by delegate OnAttackingEnded in UFSCombatComponent */
    UFUNCTION()
    void OnAttackingEnded();

protected:

    /** Drives the dash movement profile (acceleration, deceleration, etc.)
     *  X axis = normalized time [0, 1] — Y axis = normalized displacement [0, 1] */
    UPROPERTY(EditDefaultsOnly, Category = "Dash")
    UCurveFloat* DashCurve{ nullptr };

    /** Total distance covered by the dash in Unreal units */
    UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0.0"))
    float DashDistance{ 600.f };

    /** Total duration of the dash in seconds */
    UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0.01"))
    float DashDuration{ 0.2f };

    /** Time in seconds before the player can dash again after landing */
    UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0.0"))
    float DashCooldown{ 0.5f };

    /** Flow cost per dash usage */
    UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float FlowCost{ 10.f };

    UPROPERTY(EditDefaultsOnly, Category = "Dash")
    UAnimMontage* DashMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
    USoundBase* DashSound{ nullptr };

    /** Niagara system asset used for the after-image trail effect during the dash */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
    UNiagaraSystem* DashVFX;

    /** Runtime Niagara component attached to the mesh — activated during dash, deactivated on end */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UNiagaraComponent* AfterImagesVFXComp{ nullptr };

private:

    /** Reference to the owning character, cached on BeginPlay */
    ACharacter* OwningPlayer{ nullptr };

    /** World-space start and end positions computed at dash launch */
    FVector dashStart{ FVector::ZeroVector };
    FVector dashEnd{ FVector::ZeroVector };

    /** Time elapsed since dash started, used to compute normalized alpha [0, 1] */
    float dashElapsed{ 0.f };

    /** Curve output from the previous frame — used to compute per-frame delta displacement */
    float lastCurveValue{ 0.f };

    /** World-space normalized direction of the current or last dash */
    FVector DashDirectionWorld{FVector::ZeroVector};

    bool bIsDashing{ false };
    bool bIsOnCooldown{ false };

    /** Set by UFSCombatComponent via delegates — prevents dashing during an attack */
    bool bIsAttacking{ false };

    FTimerHandle cooldownTimer;

    /** Spawns and attaches the Niagara after-image component to the character's mesh */
    void InitDashVFXComp();

    void endDash();
};

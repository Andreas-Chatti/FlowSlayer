#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "UpgradeData.h"
#include "DashComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDashStarted, float, flowCost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDashEnded);

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

    /** Reset dash state and starts cooldown */
    UFUNCTION(BlueprintCallable)
    void EndDash();

    virtual void BeginPlay() override;

    /** Broadcast when the dash starts — used to trigger animations, VFX, etc. */
    UPROPERTY(BlueprintAssignable)
    FOnDashStarted OnDashStarted;

    /** Broadcast when the dash ends — used to restore movement state */
    UPROPERTY(BlueprintAssignable)
    FOnDashEnded OnDashEnded;

    /** Broadcast to check whether the player has enough flow to perform a dash */
    FCanAffordDash CanAffordDash;

    UFUNCTION(BlueprintPure)
    bool IsDashing() const { return bIsDashing; }

    /** Returns the snapped 2D input direction set at StartDash — used by AnimNotifyState to recompute world direction at movement start */
    FVector2D GetSnappedInput2D() const { return SnappedInput2D; }

    /** Forward blend weight for the 8D dash blend space — dot(dashDir, actorForward), captured once at StartDash */
    UFUNCTION(BlueprintPure)
    float GetDashForward() const { return DashForward; }

    /** Lateral blend weight for the 8D dash blend space — dot(dashDir, actorRight), captured once at StartDash */
    UFUNCTION(BlueprintPure)
    float GetDashLateral() const { return DashLateral; }

    /** Returns the distance travelled by the player during a completed dash */
    UFUNCTION(BlueprintPure)
    float GetDashDistance() const { return Distance; }

    UFUNCTION(BlueprintPure)
    bool CanDash() const;

    /** Applies upgrade effects that concern the dash system (DashFlowCost stat) */
    UFUNCTION()
    void HandleOnUpgradeSelected(const FUpgradeData& Upgrade);

    /** Internal method called by delegate OnAttackingStarted in UFSCombatComponent */
    UFUNCTION()
    void OnAttackingStarted();

    /** Internal method called by delegate OnAttackingEnded in UFSCombatComponent */
    UFUNCTION()
    void OnAttackingEnded();

protected:

    /** Distance (in meter) the player travels when doing a complete dash */
    UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "1.0"))
    float Distance{ 300.f };

    /** Maximum allowed dash duration — EndDash() is force-called if NotifyEnd hasn't fired by then */
    UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0.1"))
    float MaxDashDuration{ 1.8f };

    /** Flow cost per dash usage */
    UPROPERTY(EditDefaultsOnly, Category = "Dash", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float FlowCost{ 10.f };

private:

    /** Reference to the owning character, cached on BeginPlay */
    ACharacter* OwningPlayer{ nullptr };

    /** Snapped 2D input direction, set at StartDash — AnimNotifyState recomputes the world direction from this at NotifyBegin */
    FVector2D SnappedInput2D{ FVector2D::ZeroVector };

    /** Forward blend weight — dot(dashDir, actorForward), captured once at StartDash to prevent ABP blend space drift during animation */
    float DashForward{ 0.f };

    /** Lateral blend weight — dot(dashDir, actorRight), captured once at StartDash to prevent ABP blend space drift during animation */
    float DashLateral{ 0.f };

    /** True while the dash movement is in progress */
    bool bIsDashing{ false };

    /** Set by UFSCombatComponent via delegates — prevents dashing during an attack */
    bool bIsAttacking{ false };

    /** Safety fallback — fires EndDash() if NotifyEnd never triggers (e.g. montage interrupted before NotifyBegin) */
    FTimerHandle SafetyTimer;
};
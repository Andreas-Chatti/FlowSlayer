#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FSFlowComponent.generated.h"

/**
 * Flow tiers — unlocked progressively as CurrentFlow increases.
 * Each tier grants additional gameplay bonuses (speed, attack power, special moves).
 */
UENUM(BlueprintType)
enum class EFlowTier : uint8
{
	None,   // 0  - 24% : No bonus
	Low,    // 25 - 49% : Movement speed boost
	Medium, // 50 - 74% : Attack speed boost
	High,   // 75 - 99% : Damage boost
	Max     // 100%     : Special attacks available
};

/** Broadcast whenever CurrentFlow changes. Listened to by UI and other gameplay systems. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFlowChanged, float, current, float, max);

/** Broadcast when the player crosses a tier threshold (up or down). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFlowTierChanged, EFlowTier, newTier, EFlowTier, oldTier);

/**
 * Manages the player's Flow resource.
 * Flow is gained by landing attacks and decays passively over time when the player stops fighting.
 * Higher flow tiers grant speed, damage bonuses and unlock special attacks.
 * Taking damage reduces flow, except at Max tier where an immunity window protects it briefly.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FLOWSLAYER_API UFSFlowComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFSFlowComponent();

	/** Adds flow when the player lands a hit. Amount depends on the attack performed. */
	void AddFlow(float Amount);

	/** Removes flow either when the player gets hit
	* or when the player stops attacking during a specific amount of time
	*/
	void RemoveFlow(float amount);

	/** Voluntarily consumes flow to trigger a special attack. */
	void ConsumeFlow(float Amount);

	/** Called by the damage system when the player takes a hit.
	 * At Max tier, flow loss is deferred behind an immunity window instead of applied immediately.
	 */
	UFUNCTION()
	void OnPlayerHit(float damageAmount, AActor* damageDealer);

	/** Returns the current flow tier based on the flow ratio. */
	EFlowTier GetFlowTier() const;

	/** Returns the current flow as a normalized ratio between 0.0 and 1.0. */
	UFUNCTION(BlueprintCallable)
	float GetFlowRatio() const;

	/** Broadcast whenever CurrentFlow changes. */
	UPROPERTY(BlueprintAssignable)
	FFlowChanged FlowChanged;

	/** Broadcast when the player crosses a flow tier threshold. */
	UPROPERTY(BlueprintAssignable)
	FFlowTierChanged FlowTierChanged;

	/** Bound to CombatComponent's OnHitLandedNotify delegate.
	 * Extracts the flow reward from the hit and calls AddFlow().
	 */
	UFUNCTION()
	void OnHitLanded(AActor* actorHit, const FVector& hitLocation, float damageAmount, float flowReward);

protected:

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	/** Maximum flow value. Defines the 100% cap. */
	float MaxFlow{ 100.f };

	/** Current flow value. Clamped between 0 and MaxFlow at all times. */
	float CurrentFlow{ 0.f };

	/** Cached tier for change detection. Updated by OnFlowTierChanged. */
	EFlowTier CurrentTier{ EFlowTier::None };

	/** Flow lost per second during passive decay. */
	float DecayRate{ 8.f };

	/** Delay in seconds before passive decay starts after the last successful hit. */
	float DecayGracePeriod{ 5.f };

	/** Duration of the immunity window at Max tier when the player is hit.
	 * During this window, flow loss is deferred. Resets if the player lands a hit.
	 */
	float ImmunityDuration{ 5.f };

	/** Whether passive decay is currently active. Set to true when DecayGraceTimer expires. */
	bool bIsDecaying{ false };

	/** Started after each successful hit. Triggers passive decay on expiry. */
	FTimerHandle DecayGraceTimer;

	/** Active only at Max tier when the player takes a hit.
	 * Resets if player lands a hit during the window. Applies deferred flow loss on expiry.
	 */
	FTimerHandle ImmunityTimer;

	/** Internal callback bound to FlowChanged. Detects tier transitions and broadcasts FlowTierChanged. */
	UFUNCTION()
	void OnFlowChanged(float currentFlow, float maxFlow);

	/** Internal callback bound to FlowTierChanged. Updates the cached CurrentTier. */
	UFUNCTION()
	void OnFlowTierChanged(EFlowTier newTier, EFlowTier oldTier);
};

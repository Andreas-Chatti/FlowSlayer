#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AFSSpawnZone.h"
#include "ArenaPortal.h"
#include "RewardChest.h"
#include "../FlowSlayerCharacter.h"
#include "ProgressionComponent.h"
#include "FSArenaManager.generated.h"

/** Broadcasted when the arena encounter starts */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnArenaStarted);

/** Broadcasted when the arena is fully cleared (all enemies spawned and killed) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnArenaCleared);

/** Broadcasted each time an enemy is spawned — passes the enemy so external systems can bind to it */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemySpawned, AFSEnemy*, Enemy);

/**
 * Defines a cap escalation threshold.
 * When TotalKills reaches KillThreshold, the max alive enemy count increases by CapIncrease.
 */
USTRUCT(BlueprintType)
struct FCapEscalationStep
{
	GENERATED_BODY()

	/** Number of total kills required to trigger this escalation */
	UPROPERTY(EditAnywhere, Category = "Escalation")
	int32 KillThreshold{ 0 };

	/** How much the max alive count increases at this threshold */
	UPROPERTY(EditAnywhere, Category = "Escalation")
	int32 MaxAliveIncrease{ 1 };
};

/**
 * Orchestrates an arena encounter across multiple spawn zones.
 * Manages total enemies to spawn, max alive limit, escalation,
 * and arena lifecycle (start/clear).
 *
 * Place this actor in the level, assign SpawnZones manually in the editor.
 * RunManager calls StartArena() when the player enters this arena.
 */
UCLASS()
class FLOWSLAYER_API AFSArenaManager : public AActor
{
	GENERATED_BODY()

public:

	AFSArenaManager();

	virtual void BeginPlay() override;

	// ==================== EVENTS ====================

	/** Broadcasted when the arena starts */
	UPROPERTY(BlueprintAssignable, Category = "Arena|Events")
	FOnArenaStarted OnArenaStarted;

	/** Broadcasted when the arena is cleared */
	UPROPERTY(BlueprintAssignable, Category = "Arena|Events")
	FOnArenaCleared OnArenaCleared;

	/** Broadcasted each time an enemy is spawned — RunManager binds here to track score */
	UPROPERTY(BlueprintAssignable, Category = "Arena|Events")
	FOnEnemySpawned OnEnemySpawned;

	// ==================== PUBLIC API ====================

	/** Starts the arena encounter. Called by RunManager when the player enters this arena. */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	void StartArena();

	/** Stops the arena and clears all timers. Called by RunManager on run reset. */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	void StopArena();

	/** Returns true if the arena is currently active */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	bool IsArenaActive() const { return bIsArenaActive; }

	/** Returns current number of alive enemies across all zones */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	int32 GetAliveEnemyCount() const { return AliveEnemyCount; }

	/** Returns total kills so far in this arena */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	int32 GetTotalKills() const { return TotalKills; }

	/** Returns how many enemies can still be spawned */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	int32 GetRemainingToSpawn() const { return TotalEnemiesToSpawn - TotalSpawned; }

	/** Returns current max alive enemies allowed simultaneously */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	int32 GetCurrentMaxAlive() const { return CurrentMaxAlive; }

	/** Returns the exit portal assigned to this arena — used by RunManager to bind OnPlayerTeleported */
	AArenaPortal* GetExitPortal() const { return ExitPortal; }

private:

	// ==================== CONFIGURATION ====================

	/** Spawn zones owned by this arena — assign manually in the editor */
	UPROPERTY(EditAnywhere, Category = "Arena|Spawning")
	TArray<AAFSSpawnZone*> SpawnZones;

	/** Total number of enemies to spawn in this arena */
	UPROPERTY(EditAnywhere, Category = "Arena|Spawning")
	int32 TotalEnemiesToSpawn{ 30 };

	/** Max enemies alive at the same time when the arena starts */
	UPROPERTY(EditAnywhere, Category = "Arena|Spawning")
	int32 InitialMaxAlive{ 5 };

	/** Absolute maximum that MaxAlive can reach through escalation */
	UPROPERTY(EditAnywhere, Category = "Arena|Spawning")
	int32 MaxAliveLimit{ 15 };

	/**
	 * Ordered escalation steps. Each step defines a kill threshold
	 * and how much MaxAlive increases when that threshold is reached.
	 * Must be sorted by KillThreshold ascending in the editor.
	 */
	UPROPERTY(EditAnywhere, Category = "Arena|Escalation")
	TArray<FCapEscalationStep> EscalationSteps;

	/** Minimum spawn cooldown between each spawn attempt */
	UPROPERTY(EditAnywhere, Category = "Arena|SpawnTiming")
	float MinSpawnCooldown{ 1.f };

	/** Maximum spawn cooldown between each spawn attempt */
	UPROPERTY(EditAnywhere, Category = "Arena|SpawnTiming")
	float MaxSpawnCooldown{ 3.f };

	/** Exit portal revealed when this arena is cleared — null for the last arena */
	UPROPERTY(EditAnywhere, Category = "Arena|Navigation")
	AArenaPortal* ExitPortal{nullptr};

	UPROPERTY(EditAnywhere, Category = "Arena|Reward")
	ARewardChest* RewardChest{ nullptr };

	UPROPERTY(EditAnywhere, Category = "Arena|Debug")
	bool bForceActivate{ false };

	// ==================== RUNTIME STATE ====================

	/** Whether the arena encounter is currently running */
	bool bIsArenaActive{false};

	/** Current max alive enemies (starts at InitialMaxAlive, grows via escalation) */
	int32 CurrentMaxAlive{ 0 };

	/** Total enemies spawned so far */
	int32 TotalSpawned{ 0 };

	/** Total enemies killed so far */
	int32 TotalKills{ 0 };

	/** Currently alive enemy count */
	int32 AliveEnemyCount{ 0 };

	/** Index into EscalationSteps, tracks the next threshold to check */
	int32 NextEscalationIndex{ 0 };

	/** Timer handle for the spawn loop */
	FTimerHandle SpawnTimerHandle;

	// ==================== INTERNAL METHODS ====================

	/** Timer callback: attempts to spawn an enemy from a random zone */
	void TrySpawnEnemy();

	/** Schedules the next spawn attempt with a random cooldown */
	void ScheduleNextSpawn();

	/** Callback when a managed enemy dies */
	UFUNCTION()
	void HandleOnEnemyDeath(AFSEnemy* Enemy);

	/** Checks if cap should escalate based on current kill count */
	void CheckCapEscalation();

	/** Checks if the arena is completed (all enemies spawned and killed) */
	void CheckArenaCompletion();

	/** Cached player reference — used to award XP on enemy death */
	UPROPERTY()
	AFlowSlayerCharacter* PlayerCharacter{ nullptr };
};

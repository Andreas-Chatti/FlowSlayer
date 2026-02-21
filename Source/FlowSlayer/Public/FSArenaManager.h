#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AFSSpawnZone.h"
#include "FSArenaManager.generated.h"

/** Broadcasted when the arena encounter starts */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnArenaStarted);

/** Broadcasted when the arena is fully cleared (all enemies spawned and killed) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnArenaCleared);

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
	int32 KillThreshold{0};

	/** How much the max alive count increases at this threshold */
	UPROPERTY(EditAnywhere, Category = "Escalation")
	int32 MaxAliveIncrease{1};
};

/**
 * Orchestrates an arena encounter across multiple spawn zones.
 * Manages total enemies to spawn, max alive limit, escalation,
 * and arena lifecycle (start/clear).
 *
 * Place this actor in the level and assign SpawnZones in the editor.
 * Call StartArena() to begin the encounter (e.g. from a trigger volume overlap).
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

	// ==================== PUBLIC API ====================

	/** Starts the arena encounter. Begins spawning enemies. */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	void StartArena();

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

private:

	// ==================== CONFIGURATION ====================

	/** Spawn zones managed by this arena */
	UPROPERTY()
	TArray<AAFSSpawnZone*> SpawnZones;

	/** Total number of enemies to spawn in this arena */
	UPROPERTY(EditAnywhere, Category = "Arena|Spawning")
	int32 TotalEnemiesToSpawn{30};

	/** Max enemies alive at the same time when the arena starts */
	UPROPERTY(EditAnywhere, Category = "Arena|Spawning")
	int32 InitialMaxAlive{5};

	/** Absolute maximum that MaxAlive can reach through escalation */
	UPROPERTY(EditAnywhere, Category = "Arena|Spawning")
	int32 MaxAliveLimit{15};

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

	// ==================== RUNTIME STATE ====================

	/** Whether the arena encounter is currently running */
	bool bIsArenaActive{false};

	/** Current max alive enemies (starts at InitialMaxAlive, grows via escalation) */
	int32 CurrentMaxAlive{0};

	/** Total enemies spawned so far */
	int32 TotalSpawned{0};

	/** Total enemies killed so far */
	int32 TotalKills{0};

	/** Currently alive enemy count */
	int32 AliveEnemyCount{0};

	/** Index into EscalationSteps, tracks the next threshold to check */
	int32 NextEscalationIndex{0};

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

	/** Finds all AAFSSpawnZone actors in the level and populates the SpawnZones array.
	* Retries on a timer if no zones are found yet (actors may not be loaded).
	*/
	void InitialiseSpawnZones();

	/** Timer handle for retrying SpawnZone discovery */
	FTimerHandle SpawnZonesInitTimer;

	/** Current number of init retry attempts */
	int32 SpawnZoneInitTries{ 0 };

	/** Max retry attempts before giving up on SpawnZone discovery */
	static constexpr int32 MaxSpawnZoneInitTries{ 10 };
};

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FSArenaManager.h"
#include "FSEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "RunManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunArenaCleared);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreChanged, int32, NewScore);

/**
 * Singleton actor placed in the level.
 * Owns the ordered list of arenas and orchestrates run progression:
 * arena activation, portal reveal, and run completion events.
 */
UCLASS()
class FLOWSLAYER_API ARunManager : public AActor
{
	GENERATED_BODY()

public:

	ARunManager();

	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

	// ==================== EVENTS ====================

	/** Broadcasted when the current arena is cleared (chest and other systems react here) */
	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnRunArenaCleared OnRunArenaCleared;

	/** Broadcasted when the last arena is cleared — triggers run completion flow */
	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnRunCompleted OnRunCompleted;

	/** Broadcasted each time the score increases */
	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnScoreChanged OnScoreChanged;

	// ==================== PUBLIC API ====================

	/** Starts the run from the first arena. Called from BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartRun();

	/** Advances to the next arena. Called by the portal via OnPlayerTeleported. */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartNextArena();

	/** Returns the index of the arena currently active (0-based) */
	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetCurrentArenaIndex() const { return CurrentArenaIndex; }

	/** Returns the total number of arenas in this run */
	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetTotalArenas() const { return Arenas.Num(); }

	/** Returns true if the current arena is the last one */
	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsLastArena() const { return CurrentArenaIndex >= Arenas.Num() - 1; }

	/**
	 * Returns the elapsed run time in seconds.
	 * Live during the run, frozen after OnRunCompleted is broadcast.
	 */
	UFUNCTION(BlueprintPure, Category = "Run")
	float GetElapsedRunTime() const;

	/** Returns the current score — valid at any point during or after the run */
	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetScore() const { return CurrentScore; }

protected:

	// ==================== CONFIGURATION ====================

	/** Ordered list of arenas for this run — assign in the editor */
	UPROPERTY(EditAnywhere, Category = "Run")
	TArray<AFSArenaManager*> Arenas;

	// ==================== RUNTIME STATE ====================

	/** Index of the currently active arena */
	int32 CurrentArenaIndex{0};

	/** World time at which StartRun() was called */
	float RunStartTime{0.f};

	/** Total elapsed run time in seconds — frozen when the run completes */
	float ElapsedRunTime{0.f};

	/** Whether the run has completed */
	UPROPERTY(BlueprintReadOnly)
	bool bRunCompleted{ false };

	/** Accumulated score for this run */
	int32 CurrentScore{0};

	// ==================== INTERNAL ====================

	/** Binds OnArenaCleared on the given arena and starts it */
	void ActivateArena(AFSArenaManager* Arena);

	/** Called when the current arena broadcasts OnArenaCleared */
	UFUNCTION()
	void HandleOnArenaCleared();

	/** Called when an arena enemy is spawned — used to bind OnEnemyDeath for score tracking */
	UFUNCTION()
	void HandleOnEnemySpawned(AFSEnemy* spawnedEnemy);

	/** Called when a managed enemy dies — increments score and checks if arena is cleared */
	UFUNCTION()
	void HandleOnEnemyDeath(AFSEnemy* deadEnemy);
};

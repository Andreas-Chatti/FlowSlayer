#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FSArenaManager.h"
#include "Kismet/GameplayStatics.h"
#include "RunManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunArenaCleared);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunCompleted);

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

private:

	// ==================== CONFIGURATION ====================

	/** Ordered list of arenas for this run — assign in the editor */
	UPROPERTY(EditAnywhere, Category = "Run")
	TArray<AFSArenaManager*> Arenas;

	// ==================== RUNTIME STATE ====================

	/** Index of the currently active arena */
	int32 CurrentArenaIndex{0};

	// ==================== INTERNAL ====================

	/** Binds OnArenaCleared on the given arena and starts it */
	void ActivateArena(AFSArenaManager* Arena);

	/** Called when the current arena broadcasts OnArenaCleared */
	UFUNCTION()
	void HandleOnArenaCleared();
};

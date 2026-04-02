#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FSEnemy_Grunt.h"
#include "FSEnemy_Runner.h"
#include "FSEnemyAIController.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem.h"
#include "AFSSpawnZone.generated.h"

/**
 * Actor defining a circular zone where enemies spawn at random NavMesh positions.
 * Spawning is triggered externally by an AFSArenaManager.
 *
 * Setup requirements:
 * - Place a NavMeshBoundsVolume in the level that covers the arena floor.
 * - Enemies will spawn on any green (navigable) NavMesh surface that falls within
 *   the sphere radius of this actor. The sphere visualizes the exact search area.
 * - Size the sphere so it stays within the arena walls to avoid spawning outside.
 */
UCLASS()
class FLOWSLAYER_API AAFSSpawnZone : public AActor
{
	GENERATED_BODY()

public:

	AAFSSpawnZone();

	/**
	 * Spawns a single enemy at a random valid position within the zone.
	 * @return The spawned enemy, or nullptr if spawn failed.
	 */
	AFSEnemy* SpawnEnemy();

protected:

	virtual void BeginPlay() override;

	/** Defines the spawn zone center and search radius.
	 *  Enemies spawn on any navigable NavMesh surface (green area) within this sphere.
	 *  The sphere must overlap a NavMeshBoundsVolume — without it no spawn points will be found.
	 *  Keep the sphere inside the arena walls to prevent spawning outside. */
	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	USphereComponent* SpawnZoneComponent;

	/** Pool table of enemy type to spawn from that zone */
	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	TArray<TSubclassOf<AFSEnemy>> EnemyPoolSpawn;

	/** When true, shows the spawn zone sphere in game and draws debug spheres at each NavMesh point found:
	 *  green = accepted spawn position, red = rejected (player too close) */
	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	bool bDebugLines{ false };

	/** Gives a random location within the limits of the spawn zone
	* @return Nullopt if invalid location, otherwise returns the random location
	*/
	TOptional<FTransform> GetRandomTransform();

	/** Minimum distance an enemy will spawn from the player */
	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	double MinSpawnDistance{ 1000.0 };

	/** Player reference */
	UPROPERTY(BlueprintReadOnly)
	ACharacter* playerRef{ nullptr };

	/** Navigation system reference */
	UPROPERTY(BlueprintReadOnly)
	const UNavigationSystemV1* navSystem{ nullptr };

	/** Number of tries of spawn before aborting and return nullptr */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SpawnSettings")
	int32 MaxSpawnTries{ 50 };

	/** Number of current tries to successfully spawn an enemy 
	* This includes trying to find a valid transform (GetRandomTransform)
	* And try to successfully spawn an AFSEnemy right after (SpawnEnemy() -> GetWorld()->SpawnActor(...))
	*/
	int16 CurrentSpawnTries{ 0 };
};

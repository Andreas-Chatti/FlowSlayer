#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FSEnemy_Grunt.h"
#include "FSEnemy_Runner.h"
#include "FSEnemyAIController.h"
#include <Components/BoxComponent.h>
#include "AFSSpawnZone.generated.h"

/**
 * Actor defining a zone where enemies can spawn at random positions.
 * Uses a BoxComponent to define spawn boundaries and performs ground detection
 * via line traces to ensure enemies spawn on valid surfaces.
 * Spawning is triggered externally by an AFSArenaManager.
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

private:

	/* Main spawn zone BoxComponent
	* Has to be adjusted in the editor and to be within a NavMeshBoundVolume in order for enemies to properly move and find the player
	*/
	UPROPERTY(EditAnywhere, Category = "SpawnZone")
	UBoxComponent* SpawnZoneComponent;

	/** Pool table of enemy type to spawn from that zone */
	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	TArray<TSubclassOf<AFSEnemy>> EnemyPoolSpawn;

	/** Whether to show debug lines :
	* Line trace from the assigned ramdom spawn location to the bottom of the box (+ 100.f)
	* Impact point if a proper ground is found
	*/
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
	ACharacter* playerRef{ nullptr };
};

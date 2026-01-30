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
 */
UCLASS()
class FLOWSLAYER_API AAFSSpawnZone : public AActor
{
	GENERATED_BODY()
	
public:	

	AAFSSpawnZone();

protected:

	virtual void BeginPlay() override;

private:

	/* Main spawn zone BoxComponent 
	* Has to be adjusted in the editor and to be within a NavMeshBoundVolume in order for enemies to properly move and find the player
	*/
	UPROPERTY(EditAnywhere, Category = "SpawnZone")
	UBoxComponent* SpawnZoneComponent;

	FTimerHandle SpawnTimerHandle;
	
	/** Initial spawn cooldown when the game starts 
	* Will be reassigned randomly between MIN_COOLDOWN and MAX_COOLDOWN on each spawn
	*/
	UPROPERTY(EditDefaultsOnly, Category = "SpawnSettings")
	float SpawnCooldown{ 3.f };

	/** Pool table of enemy type to spawn from that zone */
	UPROPERTY(EditDefaultsOnly, Category = "SpawnSettings")
	TArray<TSubclassOf<AFSEnemy>> EnemyPoolSpawn;

	/** Whether to show debug lines :
	* Line trace from the assigned ramdom spawn location to the bottom of the box (+ 100.f)
	* Impact point if a proper ground is found
	*/
	UPROPERTY(EditDefaultsOnly, Category = "SpawnSettings")
	bool bDebugLines{ false };

	/** If false, this zone will be deactivated and no enemy will not spawn from this zone */
	bool bIsSpawnEnabled{ true };

	/** Main method that is called :
	* If bIsSpawnEnabled is TRUE
	* Everytime and randomly between MIN_COOLDOWN and MAX_COOLDOWN
	* Enemy spawn will fail if no ground is detected within 100 unity (1 meter) under the spawn zone
	*/
	void SpawnEnemy();

	/** Gives a random location within the limits of the spawn zone 
	* @return Nullopt if invalid location, otherwise returns the random location
	*/
	TOptional<FTransform> GetRandomTransform();

	/** Minimum spawn cooldown (included) */
	static constexpr float MIN_COOLDOWN{ 3.f };

	/** Maximum spawn cooldown (included) */
	static constexpr float MAX_COOLDOWN{ 6.f };

	/** Minimum distance an enemy will spawn from the player */
	static constexpr double MIN_SPAWN_DIST{ 1000.0 };

	/** Player reference */
	ACharacter* playerRef{ nullptr };
};

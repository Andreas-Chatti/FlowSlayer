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
	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	float SpawnCooldown{ 3.f };

	/** Pool table of enemy type to spawn from that zone */
	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	TArray<TSubclassOf<AFSEnemy>> EnemyPoolSpawn;

	/** Whether to show debug lines :
	* Line trace from the assigned ramdom spawn location to the bottom of the box (+ 100.f)
	* Impact point if a proper ground is found
	*/
	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
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
	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	float MinCooldown{ 3.f };

	/** Maximum spawn cooldown (included) */
	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	float MaxCooldown{ 6.f };

	/** Minimum distance an enemy will spawn from the player */
	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	double MinSpawnDistance{ 1000.0 };

	/** Maximum alive spawned entities this spawn zone can have simultaniously
	* If this value is -1, that means this specific zone can spawn infinite alive mobs
	* Example : If equal 10, 10 enemies can spawn from this zone and if 10 enemies have spawned and all
	* of them are still alive, mobs will stop spawning unless one or more mobs dies.
	*/
	UPROPERTY(EditAnywhere, Category = "SpawnSettings")
	int32 MaxEntities{ 10 };

	/** List of current spawned entities */
	TArray<AFSEnemy*> SpawnedEntities;

	/** Player reference */
	ACharacter* playerRef{ nullptr };

	UFUNCTION()
	void HandleOnEnemyDeath(AFSEnemy* enemy);
};

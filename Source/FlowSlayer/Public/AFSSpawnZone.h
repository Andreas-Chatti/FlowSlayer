#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FSEnemy_Grunt.h"
#include "FSEnemy_Runner.h"
#include <Components/BoxComponent.h>
#include "AFSSpawnZone.generated.h"

UCLASS()
class FLOWSLAYER_API AAFSSpawnZone : public AActor
{
	GENERATED_BODY()
	
public:	

	AAFSSpawnZone();

	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY(EditAnywhere, Category = "SpawnZone")
	UBoxComponent* SpawnZoneComponent;

	FTimerHandle SpawnTimerHandle;
	
	UPROPERTY(EditDefaultsOnly, Category = "SpawnSettings")
	float SpawnCooldown{ 3.f };

	// TODO : Faire un pool  d'ennemis à spawn (TArray de AFSEnemy)
	UPROPERTY(EditDefaultsOnly, Category = "SpawnSettings")
	TSubclassOf<AFSEnemy_Grunt> EnemyToSpawn;

	bool bIsSpawnEnabled{ true };

	void SpawnEnemy();

	FTransform GetRandomTransform() const;

	// TODO : Ajuster les cooldown
	// Problème : Le cooldown semble toujours être très court et jamais à plus de 10 secondes alors que
	// MAX_COOLDOWN est à 120.f
	static constexpr float MIN_COOLDOWN{ 3.f };

	static constexpr float MAX_COOLDOWN{ 120.f };
};

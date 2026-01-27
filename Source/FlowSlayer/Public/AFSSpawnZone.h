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

	bool bIsSpawnEnabled{ true };

	void SpawnEnemy();

	FTransform GetRandomTransform() const;

	void SetNewRandomSpawnCooldown();

	static constexpr float MIN_COOLDOWN{ 3.f };

	static constexpr float MAX_COOLDOWN{ 120.f };
};

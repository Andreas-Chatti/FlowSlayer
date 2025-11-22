#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Character.h"
#include "FSEnemy.h"
#include "FSEnemyAIController.generated.h"

UCLASS()
class FLOWSLAYER_API AFSEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:

	AFSEnemyAIController();

	UFUNCTION(BlueprintCallable)
	void JumpToDestination(FVector Destination);

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:

	UPROPERTY(EditDefaultsOnly, Category = "AI")

	AFSEnemy* OwnedEnemyPawn;

	void FollowPlayer();

	void RotateToPlayer();

	void OnMoveToTargetCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);
};

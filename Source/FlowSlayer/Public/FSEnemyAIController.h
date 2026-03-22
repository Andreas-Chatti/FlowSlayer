#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Character.h"
#include "FSEnemy.h"
#include "FSEnemyAIController.generated.h"

/**
 * AI Controller for FSEnemy characters.
 * Handles movement toward the player, attack triggering, and rotation.
 * Enemies follow the player until within attack range, then rotate and attack.
 */
UCLASS()
class FLOWSLAYER_API AFSEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:

	AFSEnemyAIController();

	/** Launches the possessed character toward a destination using projectile arc physics */
	UFUNCTION(BlueprintCallable)
	void JumpToDestination(FVector Destination);

protected:

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	/** Triggers FollowPlayer every tick if the enemy is alive, not attacking, and can attack */
	virtual void Tick(float DeltaTime) override;

private:

	/** Cached reference to the possessed enemy */
	UPROPERTY()
	AFSEnemy* OwnedEnemy{ nullptr };

	/** Cached reference to the player pawn */
	UPROPERTY()
	APawn* PlayerRef{ nullptr };

	/** Moves the enemy toward the player using NavMesh pathfinding */
	void FollowPlayer();

	/** Called when the enemy reaches the player's acceptance radius — triggers rotation and attack */
	void OnMoveToTargetCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);
};

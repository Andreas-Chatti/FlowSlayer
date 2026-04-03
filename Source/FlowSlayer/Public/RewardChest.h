#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "EnhancedInputComponent.h"
#include "RewardChest.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChestOpened);

UCLASS()
class FLOWSLAYER_API ARewardChest : public AActor
{
	GENERATED_BODY()
	
public:	

	ARewardChest();

	/** Event triggered when the chest is opened */
	UPROPERTY(BlueprintAssignable, Category = "Chest | Events")
	FOnChestOpened OnChestOpened;

	/** Open this chest instance (Broadcast OnChestOpened to listeners)*/
	UFUNCTION(BlueprintCallable)
	void OpenChest();

	/** Set this chest mesh visible if hidden 
	* and activates collision on OverlapZone
	*/
	UFUNCTION(BlueprintCallable)
	void ShowChest();

protected:

	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly)
	const APawn* PlayerRef{ nullptr };

	UPROPERTY(BlueprintReadOnly)
	APlayerController* PlayerController{ nullptr };

	UPROPERTY(EditDefaultsOnly, Category = "Chest | Input")
	UInputAction* InteractAction{ nullptr };

	/** Mesh for the chest */
	UPROPERTY(EditAnywhere, Category = "Chest | Components")
	UStaticMeshComponent* ChestMesh;

	/** Overlap zone for detecting player interaction */
	UPROPERTY(EditAnywhere, Category = "Chest | Components")
	UBoxComponent* OverlapZone;

	/** Widget component for displaying the key UI prompt */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest | UI")
	UWidgetComponent* KeyUIWidget{ nullptr };

	/** Handle for when the overlap begins */
	UFUNCTION()
	void HandleOnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Handle for when the overlap ends */
	UFUNCTION()
	void HandleOnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};

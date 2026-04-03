#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "FlowSlayerCharacter.h"
#include "Public/RunManager.h"
#include "FlowSlayerGameMode.generated.h"

UCLASS(minimalapi)
class AFlowSlayerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AFlowSlayerGameMode();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsScreenActive(UUserWidget* WidgetInstance) const;

protected:

	// ==================== CONFIGURATION ====================

	/** Upgrade screen widget class displayed on level up milestone */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> UpgradeScreenClass;

	/** Death screen widget class */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> DeathScreenClass;

	/** Win screen widget class */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> WinScreenClass;

	/** Pause screen widget class */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> PauseScreenClass;

	// ==================== RUNTIME STATE ====================

	/** Cached player character reference */
	UPROPERTY(BlueprintReadOnly)
	AFlowSlayerCharacter* PlayerCharacter{ nullptr };

	/** Current upgrade screen widget instance */
	UPROPERTY(BlueprintReadOnly)
	UUserWidget* UpgradeScreenInstance{ nullptr };

	/** Current death screen widget instance */
	UPROPERTY(BlueprintReadOnly)
	UUserWidget* DeathScreenInstance{ nullptr };

	/** Current win screen widget instance */
	UPROPERTY(BlueprintReadOnly)
	UUserWidget* WinScreenInstance{ nullptr };

	/** Current pause screen widget instance */
	UPROPERTY(BlueprintReadOnly)
	UUserWidget* PauseScreenInstance{ nullptr };

	// ==================== HANDLERS ====================

	/** Called when the player levels up and reaches milestone - shows upgrade screen */
	UFUNCTION()
	void HandleOnMilestoneLevelUp(int32 newLevel);

	/** Called when the player dies — shows the death screen */
	UFUNCTION()
	void HandleOnPlayerDeath(AFlowSlayerCharacter* player);

	/** Called when the last arena is cleared — shows the win screen */
	UFUNCTION()
	void HandleOnRunCompleted();

	/** Called when player presses pause button */
	UFUNCTION()
	void HandleOnPlayerPausePressed();

	/** Called when the reward chest is opened — shows the reward screen */
	UFUNCTION()
	void HandleOnChestOpened();

	/** Shows the given widget and switches the player controller to UI-only input */
	void ShowScreen(TSubclassOf<UUserWidget> WidgetClass, UUserWidget*& WidgetInstance, bool bPauseWorld = false);

	/** Hide the given widget instance */
	UFUNCTION(BlueprintCallable)
	void HideScreen(UUserWidget* WidgetInstance);
};

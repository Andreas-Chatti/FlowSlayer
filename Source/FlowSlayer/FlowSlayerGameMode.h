#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
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

	// ==================== HANDLERS ====================

	/** Called when the player levels up and reaches milestone - shows upgrade screen */
	UFUNCTION(BlueprintNativeEvent)
	void HandleOnMilestoneLevelUp(int32 newLevel);

	/** BlueprintNativeEvent implementation for milestone level up */
	virtual void HandleOnMilestoneLevelUp_Implementation(int32 NewLevel);

	/** Called when the player dies — shows the death screen */
	UFUNCTION()
	void HandleOnPlayerDeath(AFlowSlayerCharacter* player);

	/** Called when the last arena is cleared — shows the win screen */
	UFUNCTION()
	void HandleOnRunCompleted();

	/** Shows the given widget and switches the player controller to UI-only input */
	void ShowScreen(TSubclassOf<UUserWidget> WidgetClass, UUserWidget*& WidgetInstance, bool bPauseWorld = false);

	/** Hide the given widget instance */
	UFUNCTION(BlueprintCallable)
	void HideScreen(UUserWidget* WidgetInstance);
};

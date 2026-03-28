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

private:

	// ==================== CONFIGURATION ====================

	/** Death screen widget class — assign WBP_DeathScreen in the Blueprint */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> DeathScreenClass;

	/** Win screen widget class — assign WBP_WinScreen in the Blueprint */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> WinScreenClass;

	// ==================== RUNTIME STATE ====================

	/** Cached player character reference */
	UPROPERTY()
	AFlowSlayerCharacter* PlayerCharacter{ nullptr };

	/** Current death screen widget instance */
	UPROPERTY()
	UUserWidget* DeathScreenInstance{ nullptr };

	/** Current win screen widget instance */
	UPROPERTY()
	UUserWidget* WinScreenInstance{ nullptr };

	// ==================== HANDLERS ====================

	/** Called when the player dies — shows the death screen */
	UFUNCTION()
	void HandleOnPlayerDeath(AFlowSlayerCharacter* player);

	/** Called when the last arena is cleared — shows the win screen */
	UFUNCTION()
	void HandleOnRunCompleted();

	/** Shows the given widget and switches the player controller to UI-only input */
	void ShowEndScreen(TSubclassOf<UUserWidget> WidgetClass, UUserWidget*& WidgetInstance);
};

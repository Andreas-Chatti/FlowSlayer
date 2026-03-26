#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProgressionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnXPGained, int32, Amount, int32, NewTotal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelUp, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMilestoneLevelUp, int32, NewLevel);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FLOWSLAYER_API UProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UProgressionComponent();

	/** Awards XP to the player — triggers level up(s) if threshold is reached */
	void AddXP(int32 Amount);

	UFUNCTION(BlueprintPure)
	int32 GetCurrentLevel() const { return CurrentLevel; }

	UFUNCTION(BlueprintPure)
	int32 GetCurrentXP() const { return CurrentXP; }

	UFUNCTION(BlueprintPure)
	int32 GetXPToNextLevel() const { return XPToNextLevel; }

	/** Returns XP progress in [0, 1] for the current level — used by XP bar UI */
	UFUNCTION(BlueprintPure)
	float GetXPRatio() const;

	/** Returns true if the given level triggers a milestone reward */
	UFUNCTION(BlueprintPure)
	bool IsMilestoneLevel(int32 Level) const;

	/** Broadcasted when XP is gained */
	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnXPGained OnXPGained;

	/** Broadcasted on every level up — future entry point for the upgrade choice screen */
	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnLevelUp OnLevelUp;

	/** Broadcasted on milestone levels — entry point for special rewards (weapon upgrades, new skills) */
	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnMilestoneLevelUp OnMilestoneLevelUp;

private:

	/** Current player level */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression", meta = (AllowPrivateAccess = "true"))
	int32 CurrentLevel{ 1 };

	/** Accumulated XP within the current level */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression", meta = (AllowPrivateAccess = "true"))
	int32 CurrentXP{ 0 };

	/** XP required to reach the next level */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression", meta = (AllowPrivateAccess = "true"))
	int32 XPToNextLevel{ 60 };

	/** Number of levels between milestone rewards */
	UPROPERTY(EditDefaultsOnly, Category = "Progression", meta = (ClampMin = "1"))
	int32 MilestoneInterval{ 5 };

	/** Computes the XP threshold to level up from a given level
	 * Formula: 60 + (Level - 1) * 5
	 * Targets ~3700 total XP for a 30-level run — tunable
	 */
	int32 CalculateXPThreshold(int32 Level) const;
};

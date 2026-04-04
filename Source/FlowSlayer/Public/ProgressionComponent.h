#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "UpgradeData.h"
#include "WeaponPartData.h"
#include "ProgressionComponent.generated.h"

class AFSWeapon;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnXPGained, int32, Amount, int32, NewTotal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelUp, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMilestoneLevelUp, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemDrop, const FWeaponPartData&, WeaponPartData);

/** Broadcasted when the player confirms an upgrade selection — all concerned systems bind here */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeSelected, const FUpgradeData&, Upgrade);

/** Broadcasted when a weapon part is selected from the reward screen — FlowSlayerCharacter binds here to forward to AFSWeapon::EquipPart() */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponPartSelected, const FWeaponPartData&, WeaponPart);

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

	/**
	 * Draws N unique upgrades at random from the available pool (excludes already active upgrades).
	 * Called by the upgrade screen to populate the three choices shown to the player.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	TArray<FUpgradeData> DrawUpgrades(int32 Count = 3);

	/**
	 * Draws Count reward cards from a mixed pool of eligible upgrades and eligible weapon parts.
	 * Replaces DrawUpgrades() at all milestone and chest call sites.
	 * Returns fewer than Count cards if the combined eligible pool is smaller.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|Rewards")
	TArray<FRewardCard> DrawMixedRewards(int32 Count = 3);

	/**
	 * Called by the upgrade screen when the player confirms a choice.
	 * Stores the upgrade ID, broadcasts OnUpgradeSelected so each system can react.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|Upgrades")
	void SelectUpgrade(const FUpgradeData& Upgrade);

	/**
	 * Called by the reward screen when the player confirms a weapon part card.
	 * Broadcasts OnWeaponPartSelected — FlowSlayerCharacter forwards it to AFSWeapon::EquipPart().
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|WeaponParts")
	void SelectWeaponPart(const FWeaponPartData& WeaponPart);

	/**
	 * Rolls a random eligible weapon part slot and applies the next tier directly to the weapon.
	 * Used for enemy drop rewards — bypasses the player-choice flow entirely.
	 * Returns true if a part was applied, false if all slots are at T3 or refs are missing.
	 */
	UFUNCTION(BlueprintCallable, Category = "Progression|WeaponParts")
	bool ApplyRandomWeaponPartDrop();

	/**
	 * Injects the equipped weapon reference so eligibility checks can query current tiers.
	 * Called by FlowSlayerCharacter::BeginPlay() after CombatComponent spawns the weapon.
	 */
	void SetEquippedWeaponRef(AFSWeapon* Weapon);

	/** Returns true if the player already has the given upgrade active this run */
	UFUNCTION(BlueprintPure, Category = "Progression|Upgrades")
	bool HasUpgrade(FName UpgradeID) const;

	// ==================== DELEGATES ====================

	/** Broadcasted when XP is gained */
	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnXPGained OnXPGained;

	/** Broadcasted on every level up */
	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnLevelUp OnLevelUp;

	/** Broadcasted on milestone levels — triggers the upgrade choice screen */
	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnMilestoneLevelUp OnMilestoneLevelUp;

	/**
	 * Broadcasted when the player confirms an upgrade selection.
	 * CombatComponent, DashComponent, HealthComponent etc. bind here
	 * and apply the modification if the upgrade targets their stat.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Progression|Upgrades")
	FOnUpgradeSelected OnUpgradeSelected;

	/** Broadcasted when the player confirms a weapon part card on the reward screen */
	UPROPERTY(BlueprintAssignable, Category = "Progression|WeaponParts")
	FOnWeaponPartSelected OnWeaponPartSelected;

	/** Broadcasted when a weapon part is dropped from an enemy */
	UPROPERTY(BlueprintAssignable, Category = "Progression|WeaponParts")
	FOnItemDrop OnItemDrop;

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

	/** DataTable containing all available upgrades for the run — assign DT_Upgrades in the Blueprint */
	UPROPERTY(EditDefaultsOnly, Category = "Progression|Upgrades")
	UDataTable* UpgradeTable{ nullptr };

	/** DataTable containing all weapon part tiers — assign DT_WeaponParts in the Blueprint */
	UPROPERTY(EditDefaultsOnly, Category = "Progression|WeaponParts")
	UDataTable* WeaponPartDataTable{ nullptr };

	/** IDs of upgrades the player has selected this run — used to exclude them from future draws */
	UPROPERTY()
	TSet<FName> ActiveUpgradeIDs;

	/** Weak reference to the player's equipped weapon — injected by FlowSlayerCharacter after weapon spawn */
	UPROPERTY()
	AFSWeapon* EquippedWeapon{ nullptr };

	/** Computes the XP threshold to level up from a given level
	 * Formula: 60 + (Level - 1) * 5
	 * Targets ~3700 total XP for a 30-level run — tunable
	 */
	int32 CalculateXPThreshold(int32 Level) const;

	/** Returns true if the upgrade is neither already active nor tier-gated by an unmet prerequisite */
	bool IsUpgradeEligible(const FUpgradeData* Upgrade) const;

	/** Builds an upgrade that undoes the effect of the given one (negates additive, inverts multiplicative)
	* @return Prerequisite upgrade (tier below that new upgrade) with negative effect
	*/
	FUpgradeData BuildReverseUpgrade(const FUpgradeData& Upgrade) const;

	/** If the upgrade has an active prerequisite, broadcasts its reverse to undo the previous tier's effect */
	void UndoPreviousTier(const FUpgradeData& Upgrade);

	/**
	 * Returns true if the given part is the next eligible tier for its slot.
	 * Eligible = EquippedWeapon's current tier for that slot is exactly Tier - 1.
	 */
	bool IsWeaponPartEligible(const FWeaponPartData& PartData) const;

	/**
	 * Returns a pointer to the next eligible FWeaponPartData for the given slot,
	 * or nullptr if the slot is already at T3 or no matching row exists.
	 */
	const FWeaponPartData* FindNextPartForSlot(EWeaponPartType SlotType) const;
};

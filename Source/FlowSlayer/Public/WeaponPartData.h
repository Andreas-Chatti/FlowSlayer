#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UpgradeData.h"
#include "WeaponPartData.generated.h"

/** Which physical slot on the weapon this part occupies */
UENUM(BlueprintType)
enum class EWeaponPartType : uint8
{
	/** Blade slot — governs damage-facing stats */
	Blade,

	/** Handle slot — governs handling-facing stats (e.g. attack speed, cooldown) */
	Handle,

	/** Gem slot — governs special/auxiliary stats (defined per-part in DataTable) */
	Gem,
};

/**
 * Data row describing a single weapon part tier offered to the player.
 * Stored in DT_WeaponParts — one row per part tier (e.g. Blade_T1, Blade_T2, Blade_T3).
 * AFSWeapon owns the equipped tier state; ProgressionComponent queries it to build the eligible pool.
 */
USTRUCT(BlueprintType)
struct FWeaponPartData : public FTableRowBase
{
	GENERATED_BODY()

	/** Unique identifier for this part tier — used as the row name key (e.g. Blade_T1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PartID{ NAME_None };

	/** Which weapon slot this part occupies */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EWeaponPartType PartType{ EWeaponPartType::Blade };

	/** Tier number of this part (1, 2, or 3) — enforced by ProgressionComponent eligibility check */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1", ClampMax = "3"))
	int32 Tier{ 1 };

	/** Localized name displayed on the reward card */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	/** Localized description of the stat effect — shown on the reward card */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	/** Icon displayed on the reward card */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* Icon{ nullptr };

	/**
	 * Which stat this part modifies.
	 * Reuses EUpgradeStat so the ApplyPartStat switch in AFSWeapon can dispatch on it.
	 * Leave as None for Gem parts until Gem stats are defined.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EUpgradeStat Stat{ EUpgradeStat::None };

	/** Whether Value is additive or multiplicative — same semantics as FUpgradeData */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EUpgradeValueType ValueType{ EUpgradeValueType::Multiplicative };

	/** Magnitude of the stat effect */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Value{ 1.f };

	/**
	 * PartID of the part that must be equipped before this one appears in the reward pool.
	 * Leave as None for T1 parts. Example: Blade_T2 sets this to Blade_T1.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PrerequisitePartID{ NAME_None };
};

/** Identifies whether a reward card contains an upgrade or a weapon part */
UENUM(BlueprintType)
enum class ERewardType : uint8
{
	/** Card holds an FUpgradeData payload */
	Upgrade,

	/** Card holds an FWeaponPartData payload */
	WeaponPart,
};

/**
 * Blueprint-friendly wrapper returned by DrawMixedRewards().
 * WBP_UpgradeScreen inspects RewardType to decide which payload fields to read and display.
 * Only one of UpgradeData / WeaponPartData is meaningful per instance — determined by RewardType.
 */
USTRUCT(BlueprintType)
struct FRewardCard
{
	GENERATED_BODY()

	/** Discriminator — always read this before accessing UpgradeData or WeaponPartData */
	UPROPERTY(BlueprintReadOnly)
	ERewardType RewardType{ ERewardType::Upgrade };

	/** Valid when RewardType == ERewardType::Upgrade */
	UPROPERTY(BlueprintReadOnly)
	FUpgradeData UpgradeData;

	/** Valid when RewardType == ERewardType::WeaponPart */
	UPROPERTY(BlueprintReadOnly)
	FWeaponPartData WeaponPartData;
};

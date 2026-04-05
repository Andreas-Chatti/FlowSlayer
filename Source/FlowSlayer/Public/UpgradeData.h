#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UpgradeData.generated.h"

/**
 * Identifies which character stat an upgrade modifies.
 * Used by ProgressionComponent so each system
 * can query whether an active upgrade targets its stat and apply the value accordingly.
 */
UENUM(BlueprintType)
enum class EUpgradeStat : uint8
{
	/** No stat targeted — reserved for mechanical upgrades (future) */
	None,

	/** Outgoing attack damage multiplier */
	Damage,

	/** Maximum player health points */
	MaxHealth,

	/** Rate at which the Flow resource decays over time */
	FlowDecayRate,

	/** Flow cost consumed per dash */
	DashFlowCost,

	/** Cooldown duration between two heal uses */
	HealCooldown,

	/** Flow cost consumed when triggering the heal skill */
	HealFlowCost,

	/** Maximum movement speed of the player */
	MoveSpeed,

	/** Cooldown duration multiplier applied to all individual attack cooldowns */
	AttackCooldown,

	/** Play rate multiplier applied to all attack montages — higher value = faster animations */
	AttackPlayRate,

	/** Multiplier applied to the flow reward gained on each hit */
	FlowGainPerHit,
};

/**
 * Determines how the upgrade Value is interpreted and applied to the target stat.
 * Additive: raw value added to the stat (e.g. +10 HP).
 * Multiplicative: ratio applied as a percentage modifier (e.g. 0.15 = +15%).
 */
UENUM(BlueprintType)
enum class EUpgradeValueType : uint8
{
	/** Value is added directly to the stat (e.g. Value = 10 → +10 HP) */
	Additive,

	/** Value is a ratio applied as a multiplier (e.g. Value = 0.15 → +15%) */
	Multiplicative,
};

/**
 * Data row describing a single upgrade option presented to the player at milestone level-ups.
 * Stored in a UDataTable (DT_Upgrades) — one row per upgrade.
 * ProgressionComponent holds the list of active UpgradeIDs for the current run.
 * Each system queries ProgressionComponent to check whether an upgrade affecting it is active.
 */
USTRUCT(BlueprintType)
struct FUpgradeData : public FTableRowBase
{
	GENERATED_BODY()

	/** Unique identifier for this upgrade — used as the key in ProgressionComponent's active upgrade list */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName UpgradeID;

	/** Localized name displayed on the upgrade card in the UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	/** Localized description explaining the effect — shown below the name on the upgrade card */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	/** Icon displayed on the upgrade card — assign in the DataTable editor */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* Icon{ nullptr };

	/** Which character stat this upgrade targets — determines which system reads and applies the Value */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EUpgradeStat Stat{ EUpgradeStat::None };

	/** Whether Value is applied as a flat addition or a percentage multiplier to the target stat */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EUpgradeValueType ValueType{ EUpgradeValueType::Additive };

	/** Magnitude of the upgrade effect — interpreted according to ValueType */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Value{ 0.f };

	/**
	 * UpgradeID of the upgrade that must be active before this one can appear in the draw pool.
	 * Leave as None for tier-1 upgrades (no prerequisite).
	 * Example: Dmg_T2 sets this to Dmg_T1 → T2 only appears after T1 is selected.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PrerequisiteUpgradeID{ NAME_None };
};

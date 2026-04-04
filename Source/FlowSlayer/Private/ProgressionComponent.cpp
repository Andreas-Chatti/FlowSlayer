#include "ProgressionComponent.h"
#include "FSWeapon.h"
#include "Algo/RandomShuffle.h"

UProgressionComponent::UProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UProgressionComponent::AddXP(int32 Amount)
{
	if (Amount <= 0)
		return;

	CurrentXP += Amount;

	// Process level up(s) BEFORE broadcasting OnXPGained so GetXPRatio() is always in [0, 1]
	while (CurrentXP >= XPToNextLevel)
	{
		CurrentXP -= XPToNextLevel;
		CurrentLevel++;
		XPToNextLevel = CalculateXPThreshold(CurrentLevel);

		OnLevelUp.Broadcast(CurrentLevel);

		if (IsMilestoneLevel(CurrentLevel))
			OnMilestoneLevelUp.Broadcast(CurrentLevel);
	}

	// Broadcast after level up so ratio = CurrentXP/XPToNextLevel is always in [0, 1]
	OnXPGained.Broadcast(Amount, CurrentXP);
}

TArray<FUpgradeData> UProgressionComponent::DrawUpgrades(int32 Count)
{
	TArray<FUpgradeData> result;

	if (!UpgradeTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ProgressionComponent] DrawUpgrades called but UpgradeTable is not assigned."));
		return result;
	}

	// Making a pool array of all the upgrades we don't have yet
	TArray<const FUpgradeData*> pool;
	for (auto& row : UpgradeTable->GetRowMap())
	{
		const FUpgradeData* upgrade{ reinterpret_cast<const FUpgradeData*>(row.Value) };
		if (IsUpgradeEligible(upgrade))
			pool.Add(upgrade);
	}

	Algo::RandomShuffle(pool);

	// Picking the first three randomized elements of the shuffled pool
	for (int32 i{ 0 }; i < FMath::Min(Count, pool.Num()); i++)
		result.Add(*pool[i]);

	return result;
}

void UProgressionComponent::SelectUpgrade(const FUpgradeData& Upgrade)
{
	UndoPreviousTier(Upgrade);

	ActiveUpgradeIDs.Add(Upgrade.UpgradeID);
	OnUpgradeSelected.Broadcast(Upgrade);

	UE_LOG(LogTemp, Log, TEXT("[ProgressionComponent] Upgrade selected: %s"), *Upgrade.UpgradeID.ToString());
}

bool UProgressionComponent::IsUpgradeEligible(const FUpgradeData* Upgrade) const
{
	if (ActiveUpgradeIDs.Contains(Upgrade->UpgradeID))
		return false;

	// Tier gate: only available if prerequisite is already owned
	bool bPrerequisiteMet{ Upgrade->PrerequisiteUpgradeID == NAME_None || ActiveUpgradeIDs.Contains(Upgrade->PrerequisiteUpgradeID) };
	return bPrerequisiteMet;
}

FUpgradeData UProgressionComponent::BuildReverseUpgrade(const FUpgradeData& Upgrade) const
{
	FUpgradeData reverse{ Upgrade };

	if (reverse.ValueType == EUpgradeValueType::Additive)
		reverse.Value = -reverse.Value;
	else
		reverse.Value = (reverse.Value != 0.f) ? (1.f / reverse.Value) : 1.f;

	return reverse;
}

void UProgressionComponent::UndoPreviousTier(const FUpgradeData& Upgrade)
{
	// No need to undo if :
	// The new upgrade has no prerequisite
	// The player hasn't the prerequisite upgrade for that new upgrade
	if (Upgrade.PrerequisiteUpgradeID == NAME_None || !ActiveUpgradeIDs.Contains(Upgrade.PrerequisiteUpgradeID))
		return;

	FUpgradeData* prereqData{ UpgradeTable->FindRow<FUpgradeData>(Upgrade.PrerequisiteUpgradeID, TEXT("UndoPreviousTier")) };
	if (!prereqData)
		return;

	// If we are at TIER 2+ upgrade, we copy the prerequisite and negate that upgrade (back to default stat)
	// So it doesn't add with the current upgrade (e.g.: old upgrade: 20% more dmg -> new upgrade 40% more damage -
	// Current damage: +20% -> we delete that upgrade -> now +0% -> apply new upgrade -> +40% -> total: +40% and not +60%
	OnUpgradeSelected.Broadcast(BuildReverseUpgrade(*prereqData));
}

TArray<FRewardCard> UProgressionComponent::DrawMixedRewards(int32 Count)
{
	TArray<FRewardCard> pool;

	// Collect eligible upgrades
	if (UpgradeTable)
	{
		for (auto& row : UpgradeTable->GetRowMap())
		{
			const FUpgradeData* upgrade{ reinterpret_cast<const FUpgradeData*>(row.Value) };
			if (IsUpgradeEligible(upgrade))
			{
				FRewardCard card;
				card.RewardType = ERewardType::Upgrade;
				card.UpgradeData = *upgrade;
				pool.Add(card);
			}
		}
	}

	// Collect eligible weapon parts (only the next tier per slot)
	if (WeaponPartDataTable && EquippedWeapon)
	{
		for (auto& row : WeaponPartDataTable->GetRowMap())
		{
			const FWeaponPartData* part{ reinterpret_cast<const FWeaponPartData*>(row.Value) };
			if (IsWeaponPartEligible(*part))
			{
				FRewardCard card;
				card.RewardType = ERewardType::WeaponPart;
				card.WeaponPartData = *part;
				pool.Add(card);
			}
		}
	}
	else if (WeaponPartDataTable && !EquippedWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ProgressionComponent] DrawMixedRewards: EquippedWeapon is null — weapon parts excluded from pool."));
	}

	Algo::RandomShuffle(pool);

	TArray<FRewardCard> result;
	for (int32 i{ 0 }; i < FMath::Min(Count, pool.Num()); i++)
		result.Add(pool[i]);

	return result;
}

void UProgressionComponent::SelectWeaponPart(const FWeaponPartData& WeaponPart)
{
	OnWeaponPartSelected.Broadcast(WeaponPart);

	UE_LOG(LogTemp, Log, TEXT("[ProgressionComponent] WeaponPart selected: %s (T%d)"),
		*WeaponPart.PartID.ToString(), WeaponPart.Tier);
}

bool UProgressionComponent::ApplyRandomWeaponPartDrop()
{
	if (!WeaponPartDataTable || !EquippedWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ProgressionComponent] ApplyRandomWeaponPartDrop: missing DataTable or Weapon ref."));
		return false;
	}

	// Shuffle slot order to avoid always trying Blade first
	TArray<EWeaponPartType> slotOrder{ EWeaponPartType::Blade, EWeaponPartType::Handle, EWeaponPartType::Gem };
	Algo::RandomShuffle(slotOrder);

	for (EWeaponPartType slotType : slotOrder)
	{
		const FWeaponPartData* nextPart{ FindNextPartForSlot(slotType) };
		if (!nextPart)
			continue;

		EquippedWeapon->EquipPart(slotType, *nextPart);
		UE_LOG(LogTemp, Log, TEXT("[ProgressionComponent] Drop applied: %s T%d"), *nextPart->PartID.ToString(), nextPart->Tier);
		OnItemDrop.Broadcast(*nextPart);
		return true;
	}

	UE_LOG(LogTemp, Log, TEXT("[ProgressionComponent] ApplyRandomWeaponPartDrop: all slots at T3, nothing to drop."));
	return false;
}

void UProgressionComponent::SetEquippedWeaponRef(AFSWeapon* Weapon)
{
	EquippedWeapon = Weapon;
}

bool UProgressionComponent::IsWeaponPartEligible(const FWeaponPartData& PartData) const
{
	if (!EquippedWeapon)
		return false;

	int32 currentTier{ EquippedWeapon->GetCurrentTier(PartData.PartType) };
	return PartData.Tier == currentTier + 1;
}

const FWeaponPartData* UProgressionComponent::FindNextPartForSlot(EWeaponPartType SlotType) const
{
	if (!WeaponPartDataTable || !EquippedWeapon)
		return nullptr;

	int32 currentTier{ EquippedWeapon->GetCurrentTier(SlotType) };
	if (currentTier >= 3)
		return nullptr;

	int32 targetTier{ currentTier + 1 };

	for (auto& row : WeaponPartDataTable->GetRowMap())
	{
		const FWeaponPartData* part{ reinterpret_cast<const FWeaponPartData*>(row.Value) };
		if (part->PartType == SlotType && part->Tier == targetTier)
			return part;
	}

	return nullptr;
}

bool UProgressionComponent::HasUpgrade(FName UpgradeID) const
{
	return ActiveUpgradeIDs.Contains(UpgradeID);
}

float UProgressionComponent::GetXPRatio() const
{
	if (XPToNextLevel <= 0)
		return 1.f;

	return static_cast<float>(CurrentXP) / static_cast<float>(XPToNextLevel);
}

bool UProgressionComponent::IsMilestoneLevel(int32 Level) const
{
	return Level % MilestoneInterval == 0;
}

int32 UProgressionComponent::CalculateXPThreshold(int32 Level) const
{
	// Base 60 XP at level 1, +5 per level
	// Targets ~3700 total XP for a 30-level run — tunable
	return 60 + (Level - 1) * 5;
}

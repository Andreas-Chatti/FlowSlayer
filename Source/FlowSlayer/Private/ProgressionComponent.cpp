#include "ProgressionComponent.h"

UProgressionComponent::UProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	XPToNextLevel = CalculateXPThreshold(CurrentLevel);
}

void UProgressionComponent::AddXP(int32 Amount)
{
	if (Amount <= 0)
		return;

	CurrentXP += Amount;
	OnXPGained.Broadcast(Amount, CurrentXP);

	// Handle multi-levelup in a single XP gain (e.g. burst kill at low XP)
	while (CurrentXP >= XPToNextLevel)
	{
		CurrentXP -= XPToNextLevel;
		CurrentLevel++;
		XPToNextLevel = CalculateXPThreshold(CurrentLevel);

		OnLevelUp.Broadcast(CurrentLevel);

		if (IsMilestoneLevel(CurrentLevel))
			OnMilestoneLevelUp.Broadcast(CurrentLevel);
	}
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

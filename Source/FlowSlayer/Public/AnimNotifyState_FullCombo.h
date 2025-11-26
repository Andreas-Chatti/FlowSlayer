#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "FSCombatComponent.h"
#include "AnimNotifyState_FullCombo.generated.h"

/**
 * AnimNotifyState for FULL COMBO system
 *
 * Use this for combos where all attacks are in ONE Animation Montage.
 * Example: Array [FullCombo.uasset] - single montage containing Attack1+Attack2+Attack3
 *
 * How it works:
 * - The animation plays through automatically (no montage switching)
 * - NotifyBegin: Opens input window at each attack transition point
 * - NotifyEnd: If player didn't input, STOP the montage early (combo interrupted)
 *              If player did input, let the animation continue naturally
 *
 * Place MULTIPLE NotifyStates on the timeline:
 * - One for each attack transition (e.g., between Attack1->Attack2, Attack2->Attack3)
 * - If player doesn't input during any window, the combo stops early
 * - If player inputs during all windows, all attacks play through
 *
 * Example timeline:
 * [Attack1]----[Window1]----[Attack2]----[Window2]----[Attack3]----[End]
 *              ↑ If no input, stop here   ↑ If no input, stop here
 */
UCLASS()
class FLOWSLAYER_API UAnimNotifyState_FullCombo : public UAnimNotifyState
{
	GENERATED_BODY()

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY()
	UFSCombatComponent* CombatComp{ nullptr };
};

#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "FSCombatComponent.h"
#include "AnimNotifyState_ModularCombo.generated.h"

/**
 * AnimNotifyState for the COMBO system
 *
 * Use this for combos where each attack is a separate Animation Montage.
 * Example: Array [Attack1.uasset, Attack2.uasset, Attack3.uasset]
 *
 * How it works:
 * - NotifyBegin: Opens the combo input window (player can buffer next attack)
 * - NotifyEnd: Closes the window and checks if player queued a continue input
 *
 * Place this NotifyState on the timeline in the animation editor:
 * - Start: When you want to allow player input (usually mid-attack)
 * - End: When the input window should close (usually near end of attack)
 */
UCLASS()
class FLOWSLAYER_API UAnimNotifyState_ModularCombo : public UAnimNotifyState
{
	GENERATED_BODY()

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY()
	UFSCombatComponent* CombatComp{ nullptr };
};

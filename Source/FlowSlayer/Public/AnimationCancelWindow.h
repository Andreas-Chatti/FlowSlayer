#pragma once
#include "CoreMinimal.h"
#include "../FlowSlayerCharacter.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimationCancelWindow.generated.h"

UCLASS()
class FLOWSLAYER_API UAnimationCancelWindow : public UAnimNotifyState
{
	GENERATED_BODY()
	
public:

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:

	UPROPERTY()
	const AFlowSlayerCharacter* FSCharacter{ nullptr };

	UPROPERTY()
	const UFSCombatComponent* CombatComp{ nullptr };
};

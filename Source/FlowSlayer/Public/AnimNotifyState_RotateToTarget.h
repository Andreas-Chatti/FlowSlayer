#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "AnimNotifyState_RotateToTarget.generated.h"

/**
 * Rotates the owning character toward a target direction during the notify window.
 * - Player: rotates toward the camera view (GetControlRotation)
 * - Enemy: rotates toward the player's location
 * Supports instant snap (NotifyBegin) or smooth interpolation (NotifyTick).
 */
UCLASS(meta = (DisplayName = "RotateToTarget"))
class FLOWSLAYER_API UAnimNotifyState_RotateToTarget : public UAnimNotifyState
{
	GENERATED_BODY()

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

    /** If true, rotation snaps instantly at NotifyBegin instead of interpolating each tick */
    UPROPERTY(EditAnywhere)
    bool bSnapRotation{ false };

    /** If true, rotates toward the player's location (enemy behavior)
     * If false, rotates toward the owner's camera view (player behavior)
     */
    UPROPERTY(EditAnywhere)
    bool bIsEnemy{ false };

    /** Interpolation speed toward the target rotation */
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", EditCondition = "!bSnapRotation"))
    float RotationSpeed{ 10.f };
};

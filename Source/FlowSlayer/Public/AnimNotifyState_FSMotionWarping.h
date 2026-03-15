#pragma once
#include "CoreMinimal.h"
#include "FSLockOnComponent.h"
#include "GameFramework/Character.h"
#include "AnimNotifyState_MotionWarping.h"
#include "AnimNotifyState_FSMotionWarping.generated.h"

UENUM()
enum class EFSMotionWarpingAttackType : uint8
{
    Ground,
    Launcher,
    Air,
};

/**
 * Custom Motion Warping notify state for FlowSlayer attacks.
 * Extends UAnimNotifyState_MotionWarping to automatically resolve the warp target
 * (locked-on enemy or nearest enemy within radius) and handle air/ground movement
 * mode switching. All warp parameters are configurable directly in the montage editor.
 */
UCLASS(meta = (DisplayName = "FSMotionWarping"))
class FLOWSLAYER_API UAnimNotifyState_FSMotionWarping : public UAnimNotifyState_MotionWarping
{

	GENERATED_BODY()

protected:

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:

	/** Maximum detection radius from player position to find the nearest enemy (cm) */
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0"))
	float SearchRadius{ 300.f };

	/** Vertical offset added to the warp target position. Only relevant for air attacks */
    UPROPERTY(EditAnywhere, meta = (EditCondition = "bIsAirAttack"))
	float ZOffset{ 0.f };

	/** Forward distance subtracted from the warp target to land in front of the enemy instead of directly on them */
	UPROPERTY(EditAnywhere)
	float ForwardOffset{ 0.f };

	/** Attack type during this instance lifetime 
    * Ground: Will ignore Z axis and will stay in MovementMode::Walking
    * Launcher: Will not ignore Z axis. At NotifyBegin MovementMode will be switched to Flying then set back to Falling at NotifyEnd
    * Air: Will not ignore Z axis but will not change the current movement mode.
    */
	UPROPERTY(EditAnywhere)
	EFSMotionWarpingAttackType attackType{ EFSMotionWarpingAttackType::Ground };

	/** If true, draws debug sphere at the warp target location */
	UPROPERTY(EditAnywhere)
	bool bDebugLines{ false };

	/** Cached lock-on component reference, resolved in NotifyBegin */
    UPROPERTY()
    UFSLockOnComponent* LockOnCompRef{ nullptr };

	/** Cached character owner reference, resolved in NotifyBegin */
    UPROPERTY()
    ACharacter* PlayerOwner{ nullptr };

    /** Cached MotionWarpingComponent reference, resolved in NotifyBegin */
    UPROPERTY()
    UMotionWarpingComponent* MotionWarpingComponent;

    /** Returns the best warp target: locked-on enemy if within radius, otherwise nearest enemy
    * @param searchRadius Maximum detection radius
    * @param debugLines Whether to draw debug sphere
    * @return Target actor or nullptr if none found within radius
    */
    const AActor* GetTargetForMotionWarp(float searchRadius, bool debugLines = false);

    /** Setup motion warp for air-based attacks (air combos, aerial slams)
    * Enables MOVE_Flying to suppress gravity during the warp window.
    * @param motionWarpingTargetName Name of the warp target, must match the RootMotionModifier's WarpTargetName
    * @param targetActor Enemy to warp toward
    * @param zOffset Vertical offset added to enemy position (positive = higher, negative = lower)
    * @param forwardOffset Distance subtracted along the direction to enemy to avoid overshooting
    * @param debugLines Whether to draw debug sphere at warp target
    */
    void SetupAirAttackMotionWarp(FName motionWarpingTargetName, const AActor* targetActor, float zOffset = 0.f, float forwardOffset = 0.f, bool debugLines = false);

    /** Setup motion warp for ground-based attacks (dash attacks, ground slams)
    * Does NOT change movement mode (stays in MOVE_Walking).
    * @param motionWarpingTargetName Name of the warp target, must match the RootMotionModifier's WarpTargetName
    * @param targetActor Enemy to warp toward
    * @param forwardOffset Distance subtracted along forward vector to avoid overshooting
    * @param debugLines Whether to draw debug sphere at warp target
    */
    void SetupGroundAttackMotionWarp(FName motionWarpingTargetName, const AActor* targetActor, float forwardOffset = 0.f, bool debugLines = false);

    /** Sphere traces around the player to find the nearest living enemy within radius
    * @param distanceRadius Sphere radius in cm
    * @param debugLines Whether to draw debug sphere
    * @return Nearest enemy actor or nullptr if none found
    */
    AActor* GetNearestEnemyFromPlayer(float distanceRadius, bool debugLines = false) const;
};

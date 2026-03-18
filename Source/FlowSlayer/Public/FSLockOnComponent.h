#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "FSFocusable.h"
#include "FSDamageable.h"
#include "FSCombatComponent.h"
#include "FSLockOnComponent.generated.h"

/** Delegate when lock-on is engaged */
DECLARE_DELEGATE_OneParam(FOnLockOnStarted, AActor* lockedOnTarget);

/** Delegate when lock-on is stopped */
DECLARE_MULTICAST_DELEGATE(FOnLockOnStopped);

/**
 * Lock-On targeting system component
 * Handles target detection, switching, camera control, and lock-on state management
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FLOWSLAYER_API UFSLockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFSLockOnComponent();

	/** Broadcasted when lock-on starts and have a valid target */
	FOnLockOnStarted OnLockOnStarted;

	/** Broadcasted when the lock-on is stopped or interrupted by distance or target's death */
	FOnLockOnStopped OnLockOnStopped;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Whether the player is currently locked on a valid target */
	UFUNCTION(BlueprintCallable)
	bool IsLockedOnTarget() const { return bIsLockedOnEngaged; }

	/** @return Current locked-on target or nullptr if there's none */
	const AActor* GetCurrentLockedOnTarget() const { return CurrentLockedOnTarget; }

	/** Lock-on the nearest target in a specific sphere radius the character being the center
	* @return FALSE if no valid target is found
	* @return TRUE if a valid target is found
	*/
	bool EngageLockOn();

	/** Switch lock-on target based on camera look direction
	* @param axisValueX Mouse/controller X axis input (negative = left, positive = right)
	*/
	UFUNCTION(BlueprintCallable)
	void SwitchLockOnTarget(float axisValueX);

	/** Stop the lock-on */
	void DisengageLockOn();

	/** Delay in which the player can switch lock-on in-between targets */
	const float targetSwitchDelay{ 0.65f };

	/** Sensibility on the X axis LEFT and RIGHT when the Player moves the mouse to switch lock-on target */
	const double XAxisSwitchSensibility{ 1.0 };

protected:
	virtual void BeginPlay() override;

private:

	/** Player owner reference */
	UPROPERTY()
	ACharacter* PlayerOwner{ nullptr };

	/** Cached combat component reference */
	UPROPERTY()
	UFSCombatComponent* CombatComponent{ nullptr };

	/** List of valid targets currently in lock-on detection radius */
	UPROPERTY()
	TSet<AActor*> TargetsInLockOnRadius{};

	/** Current locked-on target */
	UPROPERTY()
	AActor* CurrentLockedOnTarget{ nullptr };

	/** IFSDamageable cache of the current locked-on target */
	IFSDamageable* CachedDamageableLockOnTarget{ nullptr };

	/** IFSFocusable cache of the current locked-on target */
	IFSFocusable* CachedFocusableTarget{ nullptr };

	/** Radius where focusable targets can be detected and locked-on */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
	float LockOnDetectionRadius{ 2000.f };

	/** TRUE if player is locked-on to a target */
	bool bIsLockedOnEngaged{ false };

	/** Timer responsible for the lock-on delay in-between targets */
	UPROPERTY()
	FTimerHandle delaySwitchLockOnTimer;

	// ==================== Camera offsets ====================

	/** Pitch offset when locked-on FAR from target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
	float FarCameraPitchOffset{ -10.0f };

	/** Pitch offset when locked-on CLOSE to target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
	float CloseCameraPitchOffset{ -5.0f };

	/** Yaw offset when locked-on CLOSE to target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
	float CloseCameraYawOffset{ 30.0f };

	/** Yaw offset when locked-on FAR from target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
	float FarCameraYawOffset{ 0.0f };

	/** Camera rotation interpolation speed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
	float CameraRotationInterpSpeed{ 8.0f };

	// ==================== Core ====================

	/** Checks every tick if the current target is still valid (alive and in range).
	 * Attempts to switch target on death before disengaging.
	 */
	void LockOnValidCheck();

	/** Rotates the player character to face the locked-on target */
	void UpdatePlayerFacingTarget(float deltaTime);

	/** Updates the camera rotation to face the locked-on target with distance-based offsets */
	void UpdateCameraFacingTarget(float deltaTime);

	/** Calls UpdatePlayerFacingTarget and UpdateCameraFacingTarget */
	void UpdateLockOnCamera(float deltaTime);

	// ==================== Helpers ====================

	/** @return TRUE if the actor is a valid lock-on candidate (Focusable, Damageable, alive) */
	bool IsValidLockOnTarget(AActor* actor) const;

	/** Assigns CurrentLockedOnTarget, CachedDamageableLockOnTarget and CachedFocusableTarget */
	void SetCurrentTarget(AActor* newTarget);

	/** Configures player movement and input mode for lock-on or free movement */
	void SetPlayerLockOnMovementMode(bool bLockOnActive);

	/** Populates TargetsInLockOnRadius with valid targets from the hit results */
	void CollectValidTargets(const TArray<FHitResult>& hits);

	/** Scores and returns the best target from TargetsInLockOnRadius based on camera alignment */
	AActor* FindBestScoredTarget(const FVector& cameraForward) const;

	/** Returns the best target in the given horizontal direction relative to the camera */
	AActor* FindBestTargetInDirection(const TArray<FHitResult>& hits, bool bLookingRight) const;

	/** Switches to the nearest valid target regardless of direction.
	 * @return The new locked-on target, or nullptr if no valid target was found
	 */
	AActor* SwitchToNearestTarget();

	/** Sphere trace to find all pawns in lock-on radius */
	bool FindTargetsInRadius(TArray<FHitResult>& outHits);

	/** Hides widgets of the previous target based on its health state */
	void HidePreviousTargetWidgets();
};

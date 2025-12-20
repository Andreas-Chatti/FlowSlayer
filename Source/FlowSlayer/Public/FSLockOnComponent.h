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
	* @return TRUE if successfully switched to new target, FALSE otherwise
	*/
	UFUNCTION(BlueprintCallable)
	bool SwitchLockOnTarget(float axisValueX);

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

	/** List of valid targets currently in lock-on detection radius
	* Only the nearest target is locked-on
	*/
	UPROPERTY()
	TSet<AActor*> TargetsInLockOnRadius{};

	/** Current locked-on target nearest to the player */
	UPROPERTY()
	AActor* CurrentLockedOnTarget{ nullptr };

	/** IFSDamageable version of the current locked-on target
	* Mainly to access IsDead() method to disengage lock-on when target dies
	*/
	IFSDamageable* CachedDamageableLockOnTarget{ nullptr };

	/** Radius where focusable targets can be detected and locked-on */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
	float LockOnDetectionRadius{ 2000.f };

	/** TRUE if player is locked-on to a target */
	bool bIsLockedOnEngaged{ false };

	/** Disable the lock-on if the target is outside the detection radius
	* Is call every LockOnDistanceCheckDelay in EngageLockOn()
	* If distance between player and locked-on target is >= LockOnDetectionRadius
	* OR target is dead
	* Calls DisengageLockOn() which deactivate the LockOnValidCheckTimer to stop this method running every
	*/
	void LockOnValidCheck();

	/** Update Pitch and Yaw rotation of the camera every frame (called in Tick) based on the locked-on target distance from the player
	* Yaw and pitch values will be interpolated based on LockOnDetectionRadius / 2
	* At minimum distance yaw and pitch offset will be equal to CloseCameraPitchOffset and CloseCameraYawOffset
	* At maximum distance (LockOnDetectionRadius / 2), yaw and pitch offset will be equal to FarCameraPitchOffset and FarCameraYawOffset
	* Check whether the target is on the right or left side of the screen and adjust dynamically the yaw (negative or positive) based on the target's position
	*/
	void UpdateLockOnCamera(float deltaTime);

	/** Min pitch offset when locked-on FAR from target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
	float FarCameraPitchOffset{ -10.0f };

	/** Max pitch offset when locked-on CLOSE to target */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
	float CloseCameraPitchOffset{ -5.0f };

	/** Max yaw offset when locked-on CLOSE an enemy */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
	float CloseCameraYawOffset{ 30.0f };

	/** Min yaw offset when locked-on FAR an enemy */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
	float FarCameraYawOffset{ 0.0f };

	/** Camera rotation interp speed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lock-On System", meta = (AllowPrivateAccess = "true"))
	float CameraRotationInterpSpeed{ 8.0f };

	/** Timer responsible for the lock-on delay in-between targets */
	UPROPERTY()
	FTimerHandle delaySwitchLockOnTimer;

	UPROPERTY()
	FTimerHandle LockOnValidCheckTimer;

	/** Delay in-between each distance checks */
	float LockOnDistanceCheckDelay{ 0.5f };
};

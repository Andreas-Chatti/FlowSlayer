#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KismetAnimationLibrary.h"
#include "Logging/LogMacros.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "MotionWarpingComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "FSWeapon.h"
#include "Public/FSDamageable.h"
#include "Public/FSCombatComponent.h"
#include "Public/CombatData.h"
#include "Public/FSLockOnComponent.h"
#include "Public/FSFlowComponent.h"
#include "Public/DashComponent.h"
#include "Public/HealthComponent.h"
#include "Public/InputManagerComponent.h"
#include "Public/ProgressionComponent.h"
#include "Components/WidgetComponent.h"
#include "FlowSlayerCharacter.generated.h"

/** Broadcasted when an attack animation cancel window opens and the player inputs a cancel action (dash or jump) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnimationCanceled, float, blendOutTime);

/** Broadcasted on this player's death */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDeath, AFlowSlayerCharacter*, player);

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class FLOWSLAYER_API AFlowSlayerCharacter : public ACharacter, public IFSDamageable
{
	GENERATED_BODY()

	////////////////////////////////////////////////
	// COMPONENTS
	////////////////////////////////////////////////
private:

	// --- Camera ---

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Handles motion warp targets for attack animations */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Animations, meta = (AllowPrivateAccess = "true"))
	UMotionWarpingComponent* MotionWarpingComponent;

	// --- Gameplay ---

	/** Handles all combat logic: combos, attack execution, hit detection */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UFSCombatComponent* CombatComponent;

	/** Manages the Flow/Momentum resource system */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UFSFlowComponent* FlowComponent;

	/** Handles lock-on targeting logic */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UFSLockOnComponent* LockOnComponent;

	/** Handles dash movement and dash cooldown */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UDashComponent* DashComponent;

	/** Manages player health and death */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UHealthComponent* HealthComponent;

	/** Handles all Enhanced Input bindings and fires input delegates */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UInputManagerComponent* InputManagerComponent;

	/** Tracks player level and XP — entry point for the upgrade system */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Progression, meta = (AllowPrivateAccess = "true"))
	UProgressionComponent* ProgressionComponent;

	////////////////////////////////////////////////
	// CACHED REFERENCES
	////////////////////////////////////////////////

	/** Cached AnimInstance reference */
	UPROPERTY()
	UAnimInstance* AnimInstance;

	////////////////////////////////////////////////
	// UI
	////////////////////////////////////////////////

	/** Main HUD widget class */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> HUDWidgetClass;

	/** Current HUD widget instance */
	UPROPERTY()
	UUserWidget* HUDWidgetInstance{ nullptr };


	/** Spawns and adds the HUD widget to the viewport */
	void InitializeHUD();

public:

	AFlowSlayerCharacter();

	// --- Lifecycle ---

	/** Prevents jumping while the player is currently executing an attack */
	virtual bool CanJumpInternal_Implementation() const override;

	// --- Queries ---

	/** Returns true if the character is currently moving */
	UFUNCTION(BlueprintCallable)
	bool IsMoving() const { return GetCharacterMovement()->Velocity.Length() > 0; }

	/** Returns the current movement speed */
	UFUNCTION(BlueprintPure, Category = "Movement")
	float GetSpeed() const { return GetCharacterMovement()->Velocity.Size(); }

	/** Returns true if the character is currently executing an attack */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAttacking() const { return CombatComponent->IsAttacking(); }

	// --- Camera accessors ---

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	// --- Component accessors ---

	UFSCombatComponent* GetCombatComponent() const { return CombatComponent; }
	UDashComponent* GetDashComponent() const { return DashComponent; }
	virtual UHealthComponent* GetHealthComponent() override { return HealthComponent; }
	UInputManagerComponent* GetInputManagerComponent() const { return InputManagerComponent; }
	UFSLockOnComponent* GetLockOnComponent() const { return LockOnComponent; }
	UProgressionComponent* GetProgressionComponent() const { return ProgressionComponent; }

	// --- WalkSpeed accessors ---

	float GetWalkSpeedThreshold() const { return WalkSpeedThreshold; }
	float GetRunSpeedThreshold() const { return RunSpeedThreshold; }
	float GetSprintSpeedThreshold() const { return SprintSpeedThreshold; }

	// --- Delegates ---

	/** Broadcasted on this player's death */
	FOnPlayerDeath OnPlayerDeath;

	/** Broadcasted when this player gets hit */
	FOnHitReceived OnHitReceived;

	/** Fired when an animation cancel window opens and the player inputs a cancel action */
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnAnimationCanceled OnAnimationCanceled;

protected:

	// --- Lifecycle ---

	virtual void BeginPlay() override;

	/** Registers all Enhanced Input bindings via InputManagerComponent */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Called by HealthComponent when the player's HP reaches 0 */
	virtual void HandleOnDeath();

	// --- Movement speed thresholds ---

	/** Speed at which the character transitions from walk to run animation */
	UPROPERTY(BlueprintReadOnly)
	float WalkSpeedThreshold{ 300.f };

	/** Speed at which the character transitions from run to sprint
	* Also used as MaxWalkSpeed while locked on
	*/
	UPROPERTY(BlueprintReadOnly)
	float RunSpeedThreshold{ 600.f };

	/** Maximum movement speed of the player */
	UPROPERTY(BlueprintReadOnly)
	float SprintSpeedThreshold{ 900.f };

	// --- IFSDamageable interface ---

	/** Called when this character receives a hit from a melee attack
	* Applies damage, knockback, hitstop, VFX, SFX and camera shake
	*/
	UFUNCTION()
	void HandleOnHitReceived(AActor* instigatorActor, const FAttackData& usedAttack);

	////////////////////////////////////////////////
	// CALLBACKS
	////////////////////////////////////////////////
private:

	/** Maps each attack-specific InputAction to its corresponding EAttackType
	* Populated in BeginPlay — used by OnAttackInputActionReceived to resolve the correct attack
	*/
	TMap<const UInputAction*, EAttackType> InputActionToAttackType;

	/** Populates InputActionToAttackType with all attack InputAction → EAttackType pairs */
	void InitializeInputActionMap();

	/** Called when the animation cancel window opens and the player inputs a cancel action */
	UFUNCTION()
	void HandleOnAnimationCanceled(float blendOutTime);

	/** Called when lock-on engages on a target */
	void HandleOnLockOnStarted(AActor* lockedOnTarget);

	/** Called when lock-on is disengaged or interrupted in any way */
	void HandleOnLockOnStopped();

	/** Called by CombatComponent when an attack successfully lands on a target */
	UFUNCTION()
	void HandleOnHitLanded(AActor* hitActor, const FVector& hitLocation, const FAttackData& usedAttack);

	/** Applies upgrade effects that concern movement speed (MoveSpeed stat) */
	UFUNCTION()
	void HandleOnUpgradeSelected(const FUpgradeData& Upgrade);

	/** Forwards weapon part selection to the equipped weapon — bound to ProgressionComponent::OnWeaponPartSelected */
	UFUNCTION()
	void HandleOnWeaponPartSelected(const FWeaponPartData& WeaponPart);

	/** IFSDamageable interface - broadcasts OnHitReceived when this character is hit */
	UFUNCTION()
	virtual void NotifyHitReceived(AActor* instigatorActor, const FAttackData& usedAttack) override;

	////////////////////////////////////////////////
	// INPUT
	////////////////////////////////////////////////

	/** Toggles lock-on ON/OFF if a valid target is within range */
	void ToggleLockOn() const;

	// --- Input delegate handlers ---
	// Bound to InputManagerComponent delegates in the constructor.
	// Each handler receives input from InputManagerComponent and forwards it to CombatComponent.

	/** Handles movement input: applies AddMovementInput in lock-on or free mode */
	void HandleMoveInput(FVector2D moveAxis);

	/** Handles look input: rotates controller and switches lock-on target on strong mouse movement */
	void HandleLookInput(FVector2D lookAxis);

	/** Handle guard input: when 'A' key is pressed, activate/deactivate guard */
	void HandleGuardInput();

	/** Handle Heal input: when LSHIFT + SPACE are pressed, activates healing skill */
	void HandleHealInput();

	/** LSHIFT - Starts a dash and optionally triggers a dash attack if LMB/RMB is held */
	void OnDashAction();

	/** Called when DashComponent starts a dash - receives the flow cost of the dash */
	UFUNCTION()
	void HandleOnDashStarted(float flowCost);

	/** Called when SPACE is pressed - triggers Jump */
	void HandleOnSpaceKeyStarted();

	/** Called when SPACE is released - triggers StopJumping */
	void HandleOnSpaceKeyCompleted();

	// --- Attack trigger ---

	/** Forwards the resolved attack type to CombatComponent with the current movement context */
	void OnAttackInputActionReceived(const UInputAction* inputAction);
};

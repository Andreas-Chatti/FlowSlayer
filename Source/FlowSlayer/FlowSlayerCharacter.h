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
#include "Kismet/KismetMathLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "FSWeapon.h"
#include "Public/FSDamageable.h"
#include "Public/FSCombatComponent.h"
#include "FlowSlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

namespace FlowSlayerInput
{
	UENUM(BlueprintType)
		enum class EActionType : uint8
	{
		NONE,
		Jump,
		Dash
	};
}

/** Delegate for animations cancel window OPEN and CLOSE */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAnimationCanceled, FlowSlayerInput::EActionType actionType);

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class FLOWSLAYER_API AFlowSlayerCharacter : public ACharacter, public IFSDamageable
{
	GENERATED_BODY()

public:

	/** Called during an animation cancel window if the player has tried to dash or jump */
	FOnAnimationCanceled OnAnimationCanceled;

private:

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Combat component class */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UFSCombatComponent* CombatComponent;

	/** Default MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Dash Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DashAction;

	// ========== ATTACK INPUT ACTIONS ==========
	// NOTE: Simplified to base actions only - variants detected via LMB/RMB and direction in callbacks
	// ===========================================

	/** LMB - Light attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LightAttackAction;

	/** RMB - Heavy attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* HeavyAttackAction;

	/** A + LMB/RMB - Launcher attacks (Launcher, PowerLauncher) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LauncherAttackAction;

	/** E + LMB/RMB - Spin attacks (SpinAttack, HorizontalSweep) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SpinAttackAction;

	/** F input key - Forward power attacks (PierceThrust, PowerSlash) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ForwardPowerAttackAction;

	/** Toggle ON / OFF lock-on ONLY if there's a target within range */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ToggleLockOnAction;

	/** Callback called when player dashed or jumped sucessfully in the animation cancel window during an attack animation */
	void HandleOnAnimationCanceled(FlowSlayerInput::EActionType actionType);

	/** Callback called when lock-on is stopped or interrupted in any way */
	void HandleOnLockOnStopped();

	/** Cached AnimInstance reference */
	UPROPERTY()
	UAnimInstance* AnimInstance;

	/** Cached PlayerController reference */
	UPROPERTY()
	APlayerController* PlayerController{ nullptr };

	/** Dash animation
	* Forward dash
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* FwdDashAnim{ nullptr };

	/** Dash animation
	* Backward dash
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* BwdDashAnim{ nullptr };

public:

	AFlowSlayerCharacter();

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	virtual bool IsDead() const override { return bIsDead; }

	virtual void ReceiveDamage(float DamageAmount, AActor* DamageDealer) override;

	virtual bool CanJumpInternal_Implementation() const override;

	UFUNCTION(BlueprintCallable)
	bool IsMoving() const { return GetCharacterMovement()->Velocity.Length() > 0; }

	UFUNCTION(BlueprintCallable)
	bool HasMovementInput() const { return bHasMovementInput; }

	const UFSCombatComponent* GetCombatComponent() const { return CombatComponent; }

protected:

	/** Is player currently giving movement input? (for ABP) */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bHasMovementInput{ false };

	/** If the last fall was cause by a jump */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bWasJumpFall{ false };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movements")
	float dashDistance{ 1250.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movements")
	float dashCooldown{ 0.3f };

	/** Was dash input pressed recently? (cleared after short delay) */
	bool bWantsToDash{ false };

	/** Timer to clear bWantsToDash */
	FTimerHandle DashInputWindowTimer;

	/** How long dash input stays "active" for cancel detection */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float DashInputWindowDuration{ 0.2f }; // 200ms

	void ClearDashInput() { bWantsToDash = false; }

	/** Was jump input pressed recently? (cleared after short delay) */
	bool bWantsToJump{ false };

	/** Timer to clear bWantsToJump */
	FTimerHandle JumpInputWindowTimer;

	/** How long Jump input stays "active" for cancel detection */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float JumpInputWindowDuration{ 0.2f }; // 200ms

	void ClearJumpInput() { bWantsToJump = false; }

public:

	bool WantsToDash() const { return bWantsToDash; }

	bool WantsToJump() const { return bWantsToJump; }

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerStats")
	float MaxHealth{ 100.f };

	/** Can Player use Dash ? */
	UPROPERTY(BlueprintReadOnly)
	bool bCanDash{ true };

	UPROPERTY(BlueprintReadOnly)
	bool bIsDashing{ false };

	/** Is Player Dead ?*/
	UPROPERTY(BlueprintReadOnly)
	bool bIsDead{ false };

	/** Max speed until player change into run state */
	UPROPERTY(BlueprintReadOnly)
	float WalkSpeedThreshold{ 300.f };

	/** Max speed until player change into sprint state 
	* Above this speed, the player will sprint until capped max speed value GetCharacterMovement()->MaxWalkSpeed
	*/
	UPROPERTY(BlueprintReadOnly)
	float RunSpeedThreshold{ 600.f };

	/* Max speed of the player 
	* Cannot go faster then this
	*/
	UPROPERTY(BlueprintReadOnly)
	float SprintSpeedThreshold{ 900.f };

	/** Used to know Player Input to determine the direction the character should go */
	UPROPERTY(BlueprintReadOnly)
	FVector2D MoveInputAxis{ 0.0, 0.0 };

public:

	FVector2D GetMoveInputAxis() const { return MoveInputAxis; }

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called when movement input is released */
	void StopMoving(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for dashing input */
	void Dash(const FInputActionValue& Value);

	/** Called for attack input (RIGHT or LEFT click) */
	void OnAttackTriggered(EAttackType attackType);

	/*
	* LEFT and RIGHT click input management 
	*/

	/** Timer input buffer */
	UPROPERTY()
	FTimerHandle InputBufferTimer;

	/** Delay before the input trigger the related events */
	static constexpr float InputBufferDelay{ 0.05f };

	/** Helper to query mouse button states (returns pair: {isLMBPressed, isRMBPressed}) */
	TPair<bool, bool> GetMouseButtonStates() const;

	/** Helper to know if a specific key is PRESSED or was just PRESSED 
	* @param inputKey we want to know the state
	* @return TRUE if the specific key has been either pressed or is currently pressed
	*/
	bool GetInputKeyState(FKey inputKey) const;

	/** Called ONCE, when LEFT click is PRESSED */
	void OnLeftClickStarted(const FInputActionInstance& Value);

	/** Called ONCE, when RIGHT click is PRESSED */
	void OnRightClickStarted(const FInputActionInstance& Value);

	/** LSHIFT + MoveAction input + LMB / RMB
	* DashAttackAction clicked state
	*/
	void OnDashAttackActionStarted();

	/** SPACE + LMB / RMB
	* JumpAttackAction clicked state
	*/
	void OnJumpAttackActionStarted();

	/** A + LMB / RMB
	* LauncherAction clicked state
	*/
	void OnLauncherActionStarted(const FInputActionInstance& Value);

	/** E / E + RMB
	* SpinAttackAction clicked state
	*/
	void OnSpinAttackActionStarted(const FInputActionInstance& Value);

	/** Z + RMB / Z + LMB
	* ForwardPowerAction clicked state
	*/
	void OnForwardPowerActionStarted(const FInputActionInstance& Value);

	/** S + RMB / S + LMB
	* SlamAction clicked state
	*/
	void OnSlamActionStarted();

	/** Switch Player's movement mode
	* Normal mode : Character is rotating directly on move direction input
	* Combat mode : Character is focused on wherever the player's camera is looking at
	*/
	void ToggleLockOn(const FInputActionInstance& Value);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	void Ragdoll();
	void DisableAllInputs();

private:

	float CurrentHealth{};

	/** Minimum velocity required in order to use Dash */
	static constexpr float MIN_DASH_VELOCITY{ 10.0f };

	void Die();

	virtual void Jump() override;

	bool bHasPressedJump{ false };

	UFUNCTION()
	virtual void Falling() override;

	/** Simple function helper to rotate the character to where player is looking
	* Smooth transition during local variable rotationDuration (0.3f default) 
	*/
	void RotatePlayerToCameraDirection();
};


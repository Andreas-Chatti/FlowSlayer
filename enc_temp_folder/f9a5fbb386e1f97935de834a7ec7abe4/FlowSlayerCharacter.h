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

	/** MappingContext */
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

	/*
	* LEFT CLICK
	* LIGHT Attack Input Action 
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LightAttackAction;

	/*
	* RIGHT CLICK
	* HEAVY Attack Input Action 
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* HeavyAttackAction;

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

	bool IsTurningInPlace() const { return bIsTurningInPlace; }

protected:

	/** Dash SFX */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds")
	USoundBase* DashSound{ nullptr };

	/** Turn in-place 180° idle animation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
	UAnimMontage* IdleTurnInPlace180Montage;

	/** Turn in-place 90° left idle animation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
	UAnimMontage* IdleTurnInPlace90LeftMontage;

	/** Turn in-place 90° right idle animation */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
	UAnimMontage* IdleTurnInPlace90RightMontage;

	/** Is player currently giving movement input? (for ABP) */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bHasMovementInput{ false };

	/** If the last fall was cause by a jump */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bWasJumpFall{ false };

	/** Is Character currently turning ? */
	UPROPERTY(BlueprintReadWrite, Category = "Movement")
	bool bIsTurningInPlace{ false };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movements")
	float dashDistance{ 1250.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movements")
	float dashCooldown{ 0.75f };

	/** Was dash input pressed recently? (cleared after short delay) */
	bool bWantsToDash{ false };

	/** Timer to clear bWantsToDash */
	FTimerHandle DashInputWindowTimer;

	/** How long dash input stays "active" for cancel detection */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float DashInputWindowDuration{ 0.2f }; // 200ms

	void ClearDashInput() { bWantsToDash = false; }

public:

	bool WantsToDash() const { return bWantsToDash; }

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

	/* Called for attack input (RIGHT or LEFT click) */
	void OnAttackTriggered(const FInputActionInstance& Value);

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

	/** Rotate the player character in the direction of the player camera view */
	void RotatePlayerToCameraDirection();

	/* If possible, make the player turn in-place
	* Check whether the player can turn in-place 
	* If TRUE play the correct animations depending on the player's rotation delta
	* Else does nothing
	*/
	void TurnInPlace();

	/** Play inplace turn idle animations 
	* @param duration : Time during the turn-in-place animation cannot be played again
	*/
	void PlayTurn(UAnimMontage* montageToPlay, float playRate, float duration);

	/** Clear rootmotion and stop turnInPlace animation to allow the player to keep moving 
	* @param force : Axis value from move inputs
	*/
	void ClearTurnInPlace(float force);
};


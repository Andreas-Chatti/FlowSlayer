#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
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

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class FLOWSLAYER_API AFlowSlayerCharacter : public ACharacter, public IFSDamageable
{
	GENERATED_BODY()

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

protected:

	/** Is player currently giving movement input? (for ABP) */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bHasMovementInput{ false };

	/* If the last fall was cause by a jump */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bWasJumpFall{ false };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movements")
	float dashDistance{ 1250.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movements")
	float dashCooldown{ 0.75f };

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

	/** Player velocity */
	UPROPERTY(BlueprintReadOnly)
	float PlayerCurrentSpeed{ 0.f };

	/* */
	UPROPERTY(BlueprintReadOnly)
	float MoveInputAxis{ 0.f };

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
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	UFUNCTION()
	virtual void Falling() override;

	/** Rotate the player character in the direction of the player camera view */
	void RotatePlayerToCameraDirection();
};


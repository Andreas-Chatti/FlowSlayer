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
#include "FlowSlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

// Delegates for hitbox activation/deactivation
DECLARE_MULTICAST_DELEGATE(FOnHitboxActivated);
DECLARE_MULTICAST_DELEGATE(FOnHitboxDeactivated);

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

	/** Attack Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* attackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* RunningAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* deathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* IdleJumpMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ForwardJumpMontage;

public:

	AFlowSlayerCharacter();

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Event delegates notify 
	* Notified during a MELEE attack Animation
	*/
	FOnHitboxActivated OnHitboxActivated;
	FOnHitboxDeactivated OnHitboxDeactivated;

	virtual bool IsDead() const override { return bIsDead; }

	virtual void ReceiveDamage(float DamageAmount, AActor* DamageDealer) override;

	virtual bool CanJumpInternal_Implementation() const override;

	bool IsMoving() const { return GetCharacterMovement()->Velocity.Length() > 0; }

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movements")
	FRotator RotationSpeed{ 0.f, 500.f, 0.f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movements")
	float MaxWalkSpeed{ 600.f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movements")
	float dashDistance{ 1250.0f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movements")
	float dashCooldown{ 0.75f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerStats")
	float MaxHealth{ 100.f };

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerStats")
	float Damage{ 50.f };

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<AFSWeapon> weaponClass;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for dashing input */
	void Dash(const FInputActionValue& Value);

	/** Called for attacking input */
	void Attack(const FInputActionValue& Value);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	virtual void BeginPlay() override;

	void Ragdoll();
	void DisableAllInputs();

private:

	bool bCanDash{ true };
	bool bIsDead{ false };
	bool bIsAttacking{ false };

	float CurrentHealth;

	/** Minimum velocity required in order to use Dash */
	static constexpr float MIN_DASH_VELOCITY{ 10.0f };

	/** Dash cooldown timer */
	UPROPERTY()
	FTimerHandle dashCooldownTimerHandle;

	/** Player's Weapon */
	UPROPERTY()
	AFSWeapon* equippedWeapon;

	/** Weapon socket */
	FString weaponSocket{ "WeaponSocket" };

	bool InitializeAndAttachWeapon();

	void Die();

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	virtual void Jump() override;

	/** Rotate the player character in the direction of the player camera view */
	void RotatePlayerToCameraDirection();
};


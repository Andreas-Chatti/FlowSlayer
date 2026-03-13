#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/ActorComponent.h"
#include "InputManagerComponent.generated.h"

class UCharacterMovementComponent;

UENUM(BlueprintType)
enum class EActionType : uint8
{
	NONE,
	Jump,
	Dash,
	Move
};

DECLARE_DELEGATE(FOnMiddleMouseButtonClicked);
DECLARE_DELEGATE(FOnShiftKeyTriggered);
DECLARE_DELEGATE(FOnLeftClickTriggered);
DECLARE_DELEGATE(FOnRightClickTriggered);
DECLARE_DELEGATE(FOnLauncherKeyTriggered);
DECLARE_DELEGATE(FOnSpinKeyTriggered);
DECLARE_DELEGATE(FOnForwardPowerKeyTriggered);
DECLARE_DELEGATE_OneParam(FOnSwitchLockOnTargetKeyTriggered, float xAxisValue);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FLOWSLAYER_API UInputManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UInputManagerComponent();

	/** Helper to query mouse button states (returns pair: {isLMBPressed, isRMBPressed}) */
	TPair<bool, bool> GetMouseButtonStates() const;

	/** Helper to know if a specific key is PRESSED or was just PRESSED
	* @param inputKey we want to know the state
	* @return TRUE if the specific key has been either pressed or is currently pressed
	*/
	bool GetInputKeyState(FKey inputKey) const;

	UFUNCTION(BlueprintPure, Category = "Input")
	FVector2D GetMoveInputAxis() const { return MoveInputAxis; }

	UFUNCTION(BlueprintCallable)
	bool HasMovementInput() const { return bHasMovementInput; }

	void SetupInputBindings(UInputComponent* PlayerInputComponent);

	void DisableAllInputs();
	void SetIsLockedOn(bool bIsLockedOn) { bLockOnActive = bIsLockedOn; }

	/** Toggle ON / OFF lock-on ONLY if there's a target within range */
	FOnMiddleMouseButtonClicked OnMiddleMouseButtonClicked;

	/** LSHIFT - Dash and dash attacks */
	FOnShiftKeyTriggered OnShiftKeyTriggered;

	/** LMB - Light attack variants (standing, running, air, slam) */
	FOnLeftClickTriggered OnLeftClickTriggered;

	/** RMB - Heavy attack variants (standing, running, air, slam) */
	FOnRightClickTriggered OnRightClickTriggered;

	/** A + LMB/RMB - Launcher attacks */
	FOnLauncherKeyTriggered OnLauncherKeyTriggered;

	/** E + LMB/RMB - Spin attacks */
	FOnSpinKeyTriggered OnSpinKeyTriggered;

	/** F + Z/S - Forward power attacks */
	FOnForwardPowerKeyTriggered OnForwardPowerKeyTriggered;

	/** Broadcasted when mouse moves enough during lock-on to switch target */
	FOnSwitchLockOnTargetKeyTriggered OnSwitchLockOnTargetKeyTriggered;

protected:

	/** Default MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	/** Is player currently giving movement input? (for ABP) */
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bHasMovementInput{ false };

	/** Used to know Player Input to determine the direction the character should go */
	UPROPERTY(BlueprintReadOnly)
	FVector2D MoveInputAxis{ 0.0, 0.0 };

private:

	UPROPERTY()
	ACharacter* OwningPlayer{ nullptr };

	UPROPERTY()
	APlayerController* PlayerController{ nullptr };

	UPROPERTY()
	UEnhancedInputComponent* EnhancedInputComponent{ nullptr };

	bool bLockOnActive{ false };

	// ========== ATTACK INPUT ACTIONS ==========
	// NOTE: Simplified to base actions only - variants detected via LMB/RMB and direction in callbacks
	// ===========================================

	/** LMB - Light attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LeftClickAction;

	/** RMB - Heavy attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RightClickAction;

	/** A + LMB/RMB - Launcher attacks (Launcher, PowerLauncher) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LauncherAttackAction;

	/** E + LMB/RMB - Spin attacks (SpinAttack, HorizontalSweep) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SpinAttackAction;

	/** F + Z/S - Forward power attacks (PierceThrust, PowerSlash) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ForwardPowerAttackAction;

	/** Toggle ON / OFF lock-on ONLY if there's a target within range */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MiddleMouseAction;

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
	UInputAction* ShiftAction;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called when movement input is released */
	void StopMoving(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for dashing input */
	void OnShiftTriggered(const FInputActionValue& Value);

	/** Called ONCE, when LEFT click is PRESSED */
	void OnLeftClickStarted(const FInputActionInstance& Value);

	/** Called ONCE, when RIGHT click is PRESSED */
	void OnRightClickStarted(const FInputActionInstance& Value);

	/** A + LMB / RMB */
	void OnLauncherActionStarted(const FInputActionInstance& Value);

	/** E + LMB / RMB */
	void OnSpinAttackActionStarted(const FInputActionInstance& Value);

	/** F + Z / F + S */
	void OnForwardPowerActionStarted(const FInputActionInstance& Value);

	/** Middle mouse button */
	void HandleOnMiddleMouseButtonStarted(const FInputActionInstance& Value);
};

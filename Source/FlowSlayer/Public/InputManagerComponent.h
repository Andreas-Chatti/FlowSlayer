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
DECLARE_DELEGATE(FOnLShiftKeyTriggered);
DECLARE_DELEGATE(FOnLMBTriggered);
DECLARE_DELEGATE(FOnRMBTriggered);
DECLARE_DELEGATE(FOnAKeyTriggered);
DECLARE_DELEGATE(FOnEKeyTriggered);
DECLARE_DELEGATE(FOnFKeyTriggered);
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
	FOnLShiftKeyTriggered OnLShiftKeyTriggered;

	/** LMB - Light attack variants (standing, running, air, slam) */
	FOnLMBTriggered OnLMBTriggered;

	/** RMB - Heavy attack variants (standing, running, air, slam) */
	FOnRMBTriggered OnRMBTriggered;

	/** A + LMB/RMB - Launcher attacks */
	FOnAKeyTriggered OnAKeyTriggered;

	/** E + LMB/RMB - Spin attacks */
	FOnEKeyTriggered OnEKeyTriggered;

	/** F + Z/S - Forward power attacks */
	FOnFKeyTriggered OnFKeyTriggered;

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

	/** LMB InputAction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LMBAction;

	/** RMB InputAction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RMBAction;

	/** 'A' InputAction*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* A_KeyAction;

	/** 'E' InputAction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* E_KeyAction;

	/** 'F' InputAction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* F_KeyAction;

	/** Middle mouse button InputAction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MiddleMouseAction;

	/** Jump (Space) Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move ('Z', 'Q', 'S', 'D') Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look (Mouse movements) Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Dash (LShift) Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LShiftAction;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called when movement input is released */
	void StopMoving(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for dashing input */
	void OnLShiftTriggered(const FInputActionValue& Value);

	/** Called ONCE, when LEFT click is PRESSED */
	void OnLMBActionStarted(const FInputActionInstance& Value);

	/** Called ONCE, when RIGHT click is PRESSED */
	void OnRMBActionStarted(const FInputActionInstance& Value);

	/** A + LMB / RMB */
	void OnAKeyActionStarted(const FInputActionInstance& Value);

	/** E + LMB / RMB */
	void OnEKeyActionStarted(const FInputActionInstance& Value);

	/** F + Z / F + S */
	void OnFKeyActionStarted(const FInputActionInstance& Value);

	/** Middle mouse button */
	void OnMiddleMouseButtonStarted(const FInputActionInstance& Value);
};

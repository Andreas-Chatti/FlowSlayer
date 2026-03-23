#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/ActorComponent.h"
#include "InputManagerComponent.generated.h"

DECLARE_DELEGATE(FOnMiddleMouseButtonClicked);
DECLARE_DELEGATE(FOnLShiftKeyTriggered);
DECLARE_DELEGATE(FOnSpaceKeyStarted);
DECLARE_DELEGATE(FOnSpaceKeyCompleted);
DECLARE_DELEGATE(FOnGuardActionTriggered);
DECLARE_DELEGATE_OneParam(FOnAttackInputReceived, const UInputAction*);
DECLARE_DELEGATE_OneParam(FOnMoveInput, FVector2D);
DECLARE_DELEGATE_OneParam(FOnLookInput, FVector2D);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FLOWSLAYER_API UInputManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UInputManagerComponent();

	/** Helper to know if a specific key is PRESSED or was just PRESSED
	* @param inputKey we want to know the state
	* @return TRUE if the specific key has been either pressed or is currently pressed
	*/
	bool GetInputKeyState(FKey inputKey) const;

	/** Returns true if any key mapped to the given InputAction is currently held down */
	bool IsInputActionTriggered(const UInputAction* inputAction) const;

	UFUNCTION(BlueprintPure, Category = "Input")
	FVector2D GetMoveInputAxis() const { return MoveInputAxis; }

	UFUNCTION(BlueprintCallable)
	bool HasMovementInput() const { return bHasMovementInput; }

	void SetupInputBindings(UInputComponent* PlayerInputComponent);

	void DisableAllInputs();

	// --- Attack InputAction getters ---
	const UInputAction* GetDashPierceAction() const { return DashPierceAction; }
	const UInputAction* GetDashSpinningSlashAction() const { return DashSpinningSlashAction; }
	const UInputAction* GetDashDoubleSlashAction() const { return DashDoubleSlashAction; }
	const UInputAction* GetDashBackSlashAction() const { return DashBackSlashAction; }
	const UInputAction* GetJumpSlamAttackAction() const { return JumpSlamAttackAction; }
	const UInputAction* GetJumpForwardSlamAttackAction() const { return JumpForwardSlamAttackAction; }
	const UInputAction* GetJumpUpperSlamAttackAction() const { return JumpUpperSlamAttackAction; }
	const UInputAction* GetLauncherAttackAction() const { return LauncherAttackAction; }
	const UInputAction* GetPowerLauncherAttackAction() const { return PowerLauncherAttackAction; }
	const UInputAction* GetSpinAttackAction() const { return SpinAttackAction; }
	const UInputAction* GetHorizontalSweepAttackAction() const { return HorizontalSweepAttackAction; }
	const UInputAction* GetPowerSlashAttackAction() const { return PowerSlashAttackAction; }
	const UInputAction* GetPierceThrustAttackAction() const { return PierceThrustAttackAction; }
	const UInputAction* GetGroundSlamAttackAction() const { return GroundSlamAttackAction; }
	const UInputAction* GetDiagonalRetourneAttackAction() const { return DiagonalRetourneAttackAction; }
	const UInputAction* GetLightAttackAction() const { return LightAttackAction; }
	const UInputAction* GetHeavyAttackAction() const { return HeavyAttackAction; }
	const UInputAction* GetGuardAction() const { return GuardAction; }

	/** Toggle ON / OFF lock-on ONLY if there's a target within range */
	FOnMiddleMouseButtonClicked OnMiddleMouseButtonClicked;

	/** LSHIFT alone — regular dash */
	FOnLShiftKeyTriggered OnLShiftKeyTriggered;

	/** SPACE - Jump started */
	FOnSpaceKeyStarted OnSpaceKeyStarted;

	/** SPACE - Jump completed (key released) */
	FOnSpaceKeyCompleted OnSpaceKeyCompleted;

	/** Broadcasted for every attack input — consumer receives the source UInputAction* and resolves it to EAttackType */
	FOnAttackInputReceived OnAttackInputReceived;

	/** Broadcasted every frame movement input is active, with the 2D axis value */
	FOnMoveInput OnMoveInput;

	/** Broadcasted every frame look input is active, with the 2D axis value */
	FOnLookInput OnLookInput;

	/** 'A' Key — guard toggle */
	FOnGuardActionTriggered OnGuardActionTriggered;

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
	UEnhancedInputLocalPlayerSubsystem* Subsystem{ nullptr };

	UPROPERTY()
	UEnhancedInputComponent* EnhancedInputComponent{ nullptr };

	// ========== BASE INPUT ACTIONS ==========

	/** Jump (Space) Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Base", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move ('Z', 'Q', 'S', 'D') Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Base", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look (Mouse movements) Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Base", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Middle mouse button InputAction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Base", meta = (AllowPrivateAccess = "true"))
	UInputAction* MiddleMouseAction;

	/** Guard ('A') InputAction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Base", meta = (AllowPrivateAccess = "true"))
	UInputAction* GuardAction;

	// ========== ATTACK INPUT ACTIONS ==========
	// Each attack has its own InputAction — chord combinations are configured in the editor

	/** LMB — covers StandingLight, RunningLight, AirCombo (state-determined) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks", meta = (AllowPrivateAccess = "true"))
	UInputAction* LightAttackAction;

	/** RMB — covers StandingHeavy, RunningHeavy, AerialSlam (state-determined) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks", meta = (AllowPrivateAccess = "true"))
	UInputAction* HeavyAttackAction;

	/** Dash (LShift) Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|Dash", meta = (AllowPrivateAccess = "true"))
	UInputAction* LShiftAction;

	/** DashPierce — LShift + 'Z' + LMB */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|Dash", meta = (AllowPrivateAccess = "true"))
	UInputAction* DashPierceAction;

	/** DashSpinningSlash — LShift + 'Q' + LMB */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|Dash", meta = (AllowPrivateAccess = "true"))
	UInputAction* DashSpinningSlashAction;

	/** DashDoubleSlash — LShift + F (ground only) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|Dash", meta = (AllowPrivateAccess = "true"))
	UInputAction* DashDoubleSlashAction;

	/** DashBackSlash — LShift + E (ground only) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|Dash", meta = (AllowPrivateAccess = "true"))
	UInputAction* DashBackSlashAction;

	/** JumpSlam — E + Z (ground + air) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|Jump", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpSlamAttackAction;

	/** JumpForwardSlam — LShift + A (ground + air) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|Jump", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpForwardSlamAttackAction;

	/** JumpUpperSlam — RMB + Z (ground + air) — TODO: revoir les touches */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|Jump", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpUpperSlamAttackAction;

	/** Launcher — 'A' + LMB */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|Launcher", meta = (AllowPrivateAccess = "true"))
	UInputAction* LauncherAttackAction;

	/** PowerLauncher — 'A' + RMB */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|Launcher", meta = (AllowPrivateAccess = "true"))
	UInputAction* PowerLauncherAttackAction;

	/** SpinAttack — 'E' + LMB */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|Spin", meta = (AllowPrivateAccess = "true"))
	UInputAction* SpinAttackAction;

	/** HorizontalSweep — 'E' + RMB */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|Spin", meta = (AllowPrivateAccess = "true"))
	UInputAction* HorizontalSweepAttackAction;

	/** PowerSlash — 'Z' + Hold RMB */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|PowerAttack", meta = (AllowPrivateAccess = "true"))
	UInputAction* PowerSlashAttackAction;

	/** PierceThrust — 'Z' + LMB (double tap) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|PowerAttack", meta = (AllowPrivateAccess = "true"))
	UInputAction* PierceThrustAttackAction;

	/** GroundSlam — 'S' + RMB */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|PowerAttack", meta = (AllowPrivateAccess = "true"))
	UInputAction* GroundSlamAttackAction;

	/** DiagonalRetourne — 'S' + LMB */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Attacks|PowerAttack", meta = (AllowPrivateAccess = "true"))
	UInputAction* DiagonalRetourneAttackAction;

	// ========== HANDLER METHODS ==========

	/** Called when SPACE is pressed */
	void HandleOnJumpStarted(const FInputActionValue& Value);

	/** Called when SPACE is released */
	void HandleOnJumpCompleted(const FInputActionValue& Value);

	/** Called for movement input */
	void HandleOnMoveTriggered(const FInputActionValue& Value);

	/** Called when movement input is released */
	void HandleOnMoveCompleted(const FInputActionValue& Value);

	/** Called for looking input */
	void HandleOnLookTriggered(const FInputActionValue& Value);

	/** Called for regular dash input (LShift alone) */
	void HandleOnDashTriggered(const FInputActionValue& Value);

	/** Called for guard input ('A' Key) */
	void HandleOnGuardTriggered(const FInputActionValue& Value);

	/** Middle mouse button */
	void HandleOnMiddleMouseButtonStarted(const FInputActionInstance& Value);
};

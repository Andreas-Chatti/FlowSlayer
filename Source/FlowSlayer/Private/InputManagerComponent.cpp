#include "InputManagerComponent.h"

UInputManagerComponent::UInputManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInputManagerComponent::SetupInputBindings(UInputComponent* PlayerInputComponent)
{
	OwningPlayer = Cast<ACharacter>(GetOwner());
	checkf(OwningPlayer, TEXT("FATAL: OwningPlayer is NULL or INVALID !"));

	PlayerController = Cast<APlayerController>(OwningPlayer->GetController());
	checkf(PlayerController, TEXT("FATAL: PlayerController is NULL or INVALID !"));

	Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	checkf(Subsystem, TEXT("FATAL: Subsystem is NULL or INVALID !"));

	Subsystem->AddMappingContext(DefaultMappingContext, 0);

	EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	checkf(EnhancedInputComponent, TEXT("FATAL: EnhancedInputComponent is NULL or INVALID !"));

	// Jumping action - SPACE
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &UInputManagerComponent::HandleOnJumpStarted);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &UInputManagerComponent::HandleOnJumpCompleted);

	// Moving action
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Started, this, &UInputManagerComponent::HandleOnMoveTriggered);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::HandleOnMoveTriggered);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &UInputManagerComponent::HandleOnMoveCompleted);

	// Looking action
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::HandleOnLookTriggered);

	// Dash - LSHIFT + 'Z' / 'Q' / 'S' / 'D' key
	EnhancedInputComponent->BindAction(LShiftAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::HandleOnDashTriggered);

	// Guard - 'A' Key
	EnhancedInputComponent->BindAction(GuardAction, ETriggerEvent::Started, this, &UInputManagerComponent::HandleOnGuardTriggered);

	// Heal - LSHIFT + SPACE
	EnhancedInputComponent->BindAction(HealAction, ETriggerEvent::Started, this, &UInputManagerComponent::HandleOnHealTriggered);

	// Lock-on - Middle mouse button
	EnhancedInputComponent->BindAction(MiddleMouseAction, ETriggerEvent::Started, this, &UInputManagerComponent::HandleOnMiddleMouseButtonStarted);

	// Pause game - 'P' key
	EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &UInputManagerComponent::HandleOnPauseActionStarted);

	// All standard attack actions — no special conditions, forward source action directly to OnAttackInputReceived
	const TArray<UInputAction*> AttackActions
	{
		DashPierceAction, DashSpinningSlashAction, DashDoubleSlashAction, DashBackSlashAction,
		JumpSlamAttackAction, JumpForwardSlamAttackAction, JumpUpperSlamAttackAction,
		LauncherAttackAction, PowerLauncherAttackAction,
		SpinAttackAction, HorizontalSweepAttackAction,
		PowerSlashAttackAction, PierceThrustAttackAction, GroundSlamAttackAction, DiagonalRetourneAttackAction,
		LightAttackAction, HeavyAttackAction
	};

	for (UInputAction* Action : AttackActions)
		EnhancedInputComponent->BindActionValueLambda(Action, ETriggerEvent::Triggered, [this, Action](const FInputActionValue& InputActionValue) { OnAttackInputReceived.ExecuteIfBound(Action); });
}

void UInputManagerComponent::HandleOnJumpStarted(const FInputActionValue& Value)
{
	OnSpaceKeyStarted.ExecuteIfBound();
}

void UInputManagerComponent::HandleOnJumpCompleted(const FInputActionValue& Value)
{
	OnSpaceKeyCompleted.ExecuteIfBound();
}

void UInputManagerComponent::HandleOnMoveTriggered(const FInputActionValue& Value)
{
	MoveInputAxis = Value.Get<FVector2D>();
	bHasMovementInput = MoveInputAxis.SquaredLength() > 0.01f;
	OnMoveInput.ExecuteIfBound(MoveInputAxis);
}

void UInputManagerComponent::HandleOnMoveCompleted(const FInputActionValue& Value)
{
	bHasMovementInput = false;
	MoveInputAxis = FVector2D::ZeroVector;
}

void UInputManagerComponent::HandleOnLookTriggered(const FInputActionValue& Value)
{
	OnLookInput.ExecuteIfBound(Value.Get<FVector2D>());
}

void UInputManagerComponent::HandleOnDashTriggered(const FInputActionValue& Value)
{
	if (IsInputActionTriggered(DashPierceAction) || IsInputActionTriggered(DashSpinningSlashAction)
		|| IsInputActionTriggered(DashDoubleSlashAction) || IsInputActionTriggered(DashBackSlashAction)
  		|| IsInputActionTriggered(JumpForwardSlamAttackAction))
		return;

	OnLShiftKeyTriggered.ExecuteIfBound();
}

void UInputManagerComponent::HandleOnGuardTriggered(const FInputActionValue& Value)
{
	if (IsInputActionTriggered(LauncherAttackAction) || IsInputActionTriggered(PowerLauncherAttackAction)
		|| IsInputActionTriggered(JumpForwardSlamAttackAction))
		return;

	OnGuardActionTriggered.ExecuteIfBound();
}

void UInputManagerComponent::HandleOnHealTriggered(const FInputActionValue& Value)
{
	OnHealActionTriggered.ExecuteIfBound();
}

void UInputManagerComponent::HandleOnMiddleMouseButtonStarted(const FInputActionInstance& Value)
{
	OnMiddleMouseButtonClicked.ExecuteIfBound();
}

void UInputManagerComponent::HandleOnPauseActionStarted(const FInputActionInstance& Value)
{
	OnPauseActionStarted.ExecuteIfBound();
}

bool UInputManagerComponent::GetInputKeyState(FKey inputKey) const
{
	if (!PlayerController)
		return false;

	return PlayerController->WasInputKeyJustPressed(inputKey) || PlayerController->IsInputKeyDown(inputKey);
}

bool UInputManagerComponent::IsInputActionTriggered(const UInputAction* inputAction) const
{
	if (!Subsystem)
		return false;

	const FInputActionInstance* instanceData{ Subsystem->GetPlayerInput()->FindActionInstanceData(inputAction) };
	if (!instanceData)
		return false;

	ETriggerEvent triggerEvent{ instanceData->GetTriggerEvent() };

	return triggerEvent == ETriggerEvent::Triggered || triggerEvent == ETriggerEvent::Started;
}

void UInputManagerComponent::DisableAllInputs()
{
	if (!PlayerController)
		return;

	if (Subsystem && DefaultMappingContext)
		Subsystem->RemoveMappingContext(DefaultMappingContext);

	OwningPlayer->DisableInput(PlayerController);
}

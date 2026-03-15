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

	UEnhancedInputLocalPlayerSubsystem* Subsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()) };
	if (PlayerController && Subsystem)
		Subsystem->AddMappingContext(DefaultMappingContext, 0);

	EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	checkf(EnhancedInputComponent, TEXT("FATAL: EnhancedInputComponent is NULL or INVALID !"));

	// Jumping action - SPACE
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, OwningPlayer, &ACharacter::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, OwningPlayer, &ACharacter::StopJumping);

	// Moving action
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::Move);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &UInputManagerComponent::StopMoving);

	// Looking action
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::Look);

	// Dashing action - LSHIFT
	EnhancedInputComponent->BindAction(ShiftAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::OnShiftTriggered);

	// Launcher attacks - A + LMB/RMB
	EnhancedInputComponent->BindAction(LauncherAttackAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::OnLauncherActionStarted);

	// Spin attacks - E + LMB/RMB
	EnhancedInputComponent->BindAction(SpinAttackAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::OnSpinAttackActionStarted);

	// Forward power attacks - F + Z/S
	EnhancedInputComponent->BindAction(ForwardPowerAttackAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::OnForwardPowerActionStarted);

	// Light attack - LMB
	EnhancedInputComponent->BindAction(LeftClickAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::OnLeftClickStarted);

	// Heavy attack - RMB
	EnhancedInputComponent->BindAction(RightClickAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::OnRightClickStarted);

	// Lock-on - Middle mouse button
	EnhancedInputComponent->BindAction(MiddleMouseAction, ETriggerEvent::Started, this, &UInputManagerComponent::HandleOnMiddleMouseButtonStarted);
}

void UInputManagerComponent::Move(const FInputActionValue& Value)
{
	MoveInputAxis = Value.Get<FVector2D>();
	bHasMovementInput = MoveInputAxis.SquaredLength() > 0.01f;

	// === MODE LOCK-ON ===
	if (bLockOnActive)
	{
		const FVector Forward{ OwningPlayer->GetActorForwardVector() };
		const FVector Right{ OwningPlayer->GetActorRightVector() };

		OwningPlayer->AddMovementInput(Forward, MoveInputAxis.Y);
		OwningPlayer->AddMovementInput(Right, MoveInputAxis.X);

		return;
	}

	// === MODE LIBRE (hors lock-on) ===
	const FRotator ControlRot{ OwningPlayer->Controller->GetControlRotation() };
	const FRotator YawRotation{ 0, ControlRot.Yaw, 0 };

	const FVector ForwardDirection{ FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X) };
	const FVector RightDirection{ FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y) };

	OwningPlayer->AddMovementInput(ForwardDirection, MoveInputAxis.Y);
	OwningPlayer->AddMovementInput(RightDirection, MoveInputAxis.X);
}

void UInputManagerComponent::StopMoving(const FInputActionValue& Value)
{
	bHasMovementInput = false;
	MoveInputAxis = FVector2D::ZeroVector;
}

void UInputManagerComponent::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector{ Value.Get<FVector2D>() };

	if (!OwningPlayer->Controller)
		return;

	OwningPlayer->AddControllerYawInput(LookAxisVector.X);
	OwningPlayer->AddControllerPitchInput(LookAxisVector.Y);

	// Switch lock-on target si mouvement de souris suffisant
	if (FMath::Abs(LookAxisVector.X) > 1.0f && bLockOnActive)
		OnSwitchLockOnTargetKeyTriggered.ExecuteIfBound(LookAxisVector.X);
}

void UInputManagerComponent::OnShiftTriggered(const FInputActionValue& Value)
{
	OnShiftKeyTriggered.ExecuteIfBound();
}

void UInputManagerComponent::OnLeftClickStarted(const FInputActionInstance& Value)
{
	OnLeftClickTriggered.ExecuteIfBound();
}

void UInputManagerComponent::OnRightClickStarted(const FInputActionInstance& Value)
{
	OnRightClickTriggered.ExecuteIfBound();
}

void UInputManagerComponent::OnLauncherActionStarted(const FInputActionInstance& Value)
{
	OnLauncherKeyTriggered.ExecuteIfBound();
}

void UInputManagerComponent::OnSpinAttackActionStarted(const FInputActionInstance& Value)
{
	OnSpinKeyTriggered.ExecuteIfBound();
}

void UInputManagerComponent::OnForwardPowerActionStarted(const FInputActionInstance& Value)
{
	OnForwardPowerKeyTriggered.ExecuteIfBound();
}

void UInputManagerComponent::HandleOnMiddleMouseButtonStarted(const FInputActionInstance& Value)
{
	OnMiddleMouseButtonClicked.ExecuteIfBound();
}

TPair<bool, bool> UInputManagerComponent::GetMouseButtonStates() const
{
	if (!PlayerController)
		return TPair<bool, bool>(false, false);

	bool isLMBPressed{ PlayerController->IsInputKeyDown(EKeys::LeftMouseButton) };
	bool isRMBPressed{ PlayerController->IsInputKeyDown(EKeys::RightMouseButton) };

	return TPair<bool, bool>{ isLMBPressed, isRMBPressed };
}

bool UInputManagerComponent::GetInputKeyState(FKey inputKey) const
{
	if (!PlayerController)
		return false;

	return PlayerController->WasInputKeyJustPressed(inputKey) || PlayerController->IsInputKeyDown(inputKey);
}

void UInputManagerComponent::DisableAllInputs()
{
	if (!PlayerController)
		return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()) };
	if (Subsystem && DefaultMappingContext)
		Subsystem->RemoveMappingContext(DefaultMappingContext);

	OwningPlayer->DisableInput(PlayerController);
}

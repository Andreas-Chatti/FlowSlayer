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
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &UInputManagerComponent::OnSpaceKeyActionStarted);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &UInputManagerComponent::OnSpaceKeyActionCompleted);

	// Moving action
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::Move);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &UInputManagerComponent::StopMoving);

	// Looking action
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::Look);

	// Dashing action - LSHIFT
	EnhancedInputComponent->BindAction(LShiftAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::OnLShiftTriggered);

	// Launcher attacks - A + LMB/RMB
	EnhancedInputComponent->BindAction(A_KeyAction, ETriggerEvent::Started, this, &UInputManagerComponent::OnAKeyActionStarted);

	// Spin attacks - E + LMB/RMB
	EnhancedInputComponent->BindAction(E_KeyAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::OnEKeyActionStarted);

	// Forward power attacks - F + Z/S
	EnhancedInputComponent->BindAction(F_KeyAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::OnFKeyActionStarted);

	// Light attack - LMB
	EnhancedInputComponent->BindAction(LMBAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::OnLMBActionStarted);

	// Heavy attack - RMB
	EnhancedInputComponent->BindAction(RMBAction, ETriggerEvent::Triggered, this, &UInputManagerComponent::OnRMBActionStarted);

	// Lock-on - Middle mouse button
	EnhancedInputComponent->BindAction(MiddleMouseAction, ETriggerEvent::Started, this, &UInputManagerComponent::OnMiddleMouseButtonStarted);
}

void UInputManagerComponent::OnSpaceKeyActionStarted(const FInputActionValue& Value)
{
	OnSpaceKeyStarted.ExecuteIfBound();
}

void UInputManagerComponent::OnSpaceKeyActionCompleted(const FInputActionValue& Value)
{
	OnSpaceKeyCompleted.ExecuteIfBound();
}

void UInputManagerComponent::Move(const FInputActionValue& Value)
{
	MoveInputAxis = Value.Get<FVector2D>();
	bHasMovementInput = MoveInputAxis.SquaredLength() > 0.01f;
	OnMoveInput.ExecuteIfBound(MoveInputAxis);
}

void UInputManagerComponent::StopMoving(const FInputActionValue& Value)
{
	bHasMovementInput = false;
	MoveInputAxis = FVector2D::ZeroVector;
}

void UInputManagerComponent::Look(const FInputActionValue& Value)
{
	OnLookInput.ExecuteIfBound(Value.Get<FVector2D>());
}

void UInputManagerComponent::OnLShiftTriggered(const FInputActionValue& Value)
{
	/*
	if (bLShiftBufferActive)
		return;

	bLShiftBufferActive = true;

	GetWorld()->GetTimerManager().SetTimer(
		LShiftBufferTimer,
		[this]()
		{
			bLShiftBufferActive = false;
			OnLShiftKeyTriggered.ExecuteIfBound();
		},
		0.05f,
		false
	);
	*/
	OnLShiftKeyTriggered.ExecuteIfBound();
}

void UInputManagerComponent::OnLMBActionStarted(const FInputActionInstance& Value)
{
	OnLMBTriggered.ExecuteIfBound();
}

void UInputManagerComponent::OnRMBActionStarted(const FInputActionInstance& Value)
{
	OnRMBTriggered.ExecuteIfBound();
}

void UInputManagerComponent::OnAKeyActionStarted(const FInputActionInstance& Value)
{
	OnAKeyTriggered.ExecuteIfBound();
}

void UInputManagerComponent::OnEKeyActionStarted(const FInputActionInstance& Value)
{
	OnEKeyTriggered.ExecuteIfBound();
}

void UInputManagerComponent::OnFKeyActionStarted(const FInputActionInstance& Value)
{
	OnFKeyTriggered.ExecuteIfBound();
}

void UInputManagerComponent::OnMiddleMouseButtonStarted(const FInputActionInstance& Value)
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

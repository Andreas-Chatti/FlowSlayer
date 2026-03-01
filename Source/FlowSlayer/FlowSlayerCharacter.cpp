#include "FlowSlayerCharacter.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AFlowSlayerCharacter::AFlowSlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	USkeletalMeshComponent* mesh = GetMesh();
	if (mesh)
	{
		mesh->SetRelativeLocation(FVector(0.f, 0.f, -96.f)); // Offset capsule
		mesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f)); // Face forward
		mesh->SetUsingAbsoluteLocation(false);
		mesh->SetUsingAbsoluteRotation(false);
	}

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator{ 0.f, 500.f, 0.f };

	GetCharacterMovement()->JumpZVelocity = 900.0f;
	GetCharacterMovement()->AirControl = 0.8f;
	GetCharacterMovement()->BrakingDecelerationFalling = 5000.f;
	GetCharacterMovement()->GravityScale = 2.5f;

	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetRelativeLocation(FVector{ 0, 0, 80 });
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.f;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 10.f;
	CameraBoom->TargetArmLength = 550.0f;
	CameraBoom->SocketOffset = FVector{ 0, 40, 70 };
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	LockOnComponent = CreateDefaultSubobject<UFSLockOnComponent>(TEXT("LockOnComponent"));
	checkf(LockOnComponent, TEXT("FATAL: LockOnComponent is NULL or INVALID !"));

	CombatComponent = CreateDefaultSubobject<UFSCombatComponent>(TEXT("CombatComponent"));
	checkf(CombatComponent, TEXT("FATAL: CombatComponent is NULL or INVALID !"));

	FlowComponent = CreateDefaultSubobject<UFSFlowComponent>(TEXT("FlowComponent"));
	checkf(FlowComponent, TEXT("FATAL: FlowComponent is NULL or INVALID !"));

	JumpMaxCount = 2;
	CurrentHealth = MaxHealth;
}

void AFlowSlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	LockOnComponent->OnLockOnStarted.BindUObject(this, &AFlowSlayerCharacter::HandleOnLockOnStarted);
	LockOnComponent->OnLockOnStopped.AddUObject(this, &AFlowSlayerCharacter::HandleOnLockOnStopped);
	OnAnimationCanceled.AddUObject(this, &AFlowSlayerCharacter::HandleOnAnimationCanceled);

	AnimInstance = GetMesh()->GetAnimInstance();
	checkf(AnimInstance, TEXT("AnimInstance is NULL"));

	CombatComponent->OnHitLandedNotify.AddUniqueDynamic(FlowComponent, &UFSFlowComponent::OnHitLanded);
	AnimInstance->OnMontageEnded.AddDynamic(this, &AFlowSlayerCharacter::OnMontageEnded);
	OnDamageTaken.AddUniqueDynamic(FlowComponent, &UFSFlowComponent::OnPlayerHit);

	/** Tag used when other classes trying to avoid direct dependance to this class */
	Tags.Add("Player");

	InitializeHUD();
}

void AFlowSlayerCharacter::InitializeHUD()
{
	if (!HUDWidgetClass || !PlayerController)
		return;

	HUDWidgetInstance = CreateWidget<UUserWidget>(PlayerController, HUDWidgetClass);
	if (HUDWidgetInstance)
		HUDWidgetInstance->AddToViewport();
}

void AFlowSlayerCharacter::ReceiveDamage(float DamageAmount, AActor* DamageDealer)
{
	if (bIsDead)
		return;

	OnDamageTaken.Broadcast(DamageAmount, DamageDealer);

	if (bInvincibility)
		return;

    CurrentHealth -= DamageAmount;

    if (CurrentHealth <= 0.f)
        Die();
}

bool AFlowSlayerCharacter::CanJumpInternal_Implementation() const
{
	if (CombatComponent->isAttacking())
		return false;
	
	return Super::CanJumpInternal_Implementation();
}

void AFlowSlayerCharacter::Die()
{
	bIsDead = true;

	DisableAllInputs();

}

void AFlowSlayerCharacter::Ragdoll()
{
	USkeletalMeshComponent* playerMesh{ GetMesh() };
	if (playerMesh)
	{
		playerMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		playerMesh->SetSimulatePhysics(true);
		playerMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		playerMesh->SetCollisionProfileName(TEXT("Ragdoll"));

		FVector ImpulseDirection = GetActorForwardVector() * -500.f + FVector(0, 0, 300.f);
		playerMesh->AddImpulse(ImpulseDirection, NAME_None, true);
	}
}

void AFlowSlayerCharacter::DisableAllInputs()
{
	if (!PlayerController)
		return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem{ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())};
	if (Subsystem && DefaultMappingContext)
		Subsystem->RemoveMappingContext(DefaultMappingContext);

	DisableInput(PlayerController);
}

void AFlowSlayerCharacter::ToggleLockOn(const FInputActionInstance& Value)
{
	if (!LockOnComponent->IsLockedOnTarget())
		LockOnComponent->EngageLockOn();

	else if (LockOnComponent->IsLockedOnTarget())
		LockOnComponent->DisengageLockOn();

	GetCharacterMovement()->MaxWalkSpeed = LockOnComponent->IsLockedOnTarget() ? RunSpeedThreshold : SprintSpeedThreshold;
}

void AFlowSlayerCharacter::HandleOnAnimationCanceled(FlowSlayerInput::EActionType actionType)
{
	CombatComponent->CancelAttack();

	switch (actionType)
	{
	case FlowSlayerInput::EActionType::NONE: return;
	case FlowSlayerInput::EActionType::Jump:
		Jump();
		break;
	case FlowSlayerInput::EActionType::Dash:
		Dash(FInputActionValue::Axis1D(1.f));
		break;
	}
}

void AFlowSlayerCharacter::HandleOnLockOnStarted(AActor* lockedOnTarget)
{
	CombatComponent->SetLockedOnTargetRef(lockedOnTarget);
}

void AFlowSlayerCharacter::HandleOnLockOnStopped()
{
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeedThreshold;
	CombatComponent->SetLockedOnTargetRef(nullptr);
}

void AFlowSlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	PlayerController = Cast<APlayerController>(GetController());
	UEnhancedInputLocalPlayerSubsystem* Subsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()) };
	if (PlayerController && Subsystem)
		Subsystem->AddMappingContext(DefaultMappingContext, 0);

	// Set up action bindings
	UEnhancedInputComponent* EnhancedInputComponent{ Cast<UEnhancedInputComponent>(PlayerInputComponent) };
	if (EnhancedInputComponent)
	{
		// Jumping action - SPACE
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving action
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFlowSlayerCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AFlowSlayerCharacter::StopMoving);

		// Looking action
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFlowSlayerCharacter::Look);

		// Dashing action - LSHIFT
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &AFlowSlayerCharacter::Dash);

		// Launcher attacks -  A + LMB/RMB
		EnhancedInputComponent->BindAction(LauncherAttackAction, ETriggerEvent::Triggered, this, &AFlowSlayerCharacter::OnLauncherActionStarted);

		// Spin attacks - E + LMB/RMB
		EnhancedInputComponent->BindAction(SpinAttackAction, ETriggerEvent::Triggered, this, &AFlowSlayerCharacter::OnSpinAttackActionStarted);

		// Forward power attacks - F + Z/S
		EnhancedInputComponent->BindAction(ForwardPowerAttackAction, ETriggerEvent::Triggered, this, &AFlowSlayerCharacter::OnForwardPowerActionStarted);

		// Light attack - LMB
		EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Triggered, this, &AFlowSlayerCharacter::OnLeftClickStarted);

		// Heavy attack - RMB
		EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Triggered, this, &AFlowSlayerCharacter::OnRightClickStarted);

		// Lock-on MODE - Middle mouse button
		EnhancedInputComponent->BindAction(ToggleLockOnAction, ETriggerEvent::Started, this, &AFlowSlayerCharacter::ToggleLockOn);
	}

	else
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
}

void AFlowSlayerCharacter::Move(const FInputActionValue& Value)
{
	if (CombatComponent->isAttacking())
		return;

	FVector2D MovementVector = Value.Get<FVector2D>();
	bHasMovementInput = MovementVector.SquaredLength() > 0.01f;

	if (!Controller)
		return;

	// === MODE LOCK-ON ===
	if (LockOnComponent->IsLockedOnTarget())
	{
		// Ignore la caméra
		const FVector Forward{ GetActorForwardVector() };
		const FVector Right{ GetActorRightVector() };

		AddMovementInput(Forward, MovementVector.Y);
		AddMovementInput(Right, MovementVector.X);

		MoveInputAxis = MovementVector;
		return;
	}

	// === MODE LIBRE (hors lock-on) ===
	const FRotator ControlRot = Controller->GetControlRotation();
	const FRotator YawRotation(0, ControlRot.Yaw, 0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);

	MoveInputAxis = MovementVector;
}

void AFlowSlayerCharacter::StopMoving(const FInputActionValue& Value)
{
	bHasMovementInput = false;
	MoveInputAxis = FVector2D::ZeroVector;
}

void AFlowSlayerCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector{ Value.Get<FVector2D>() };

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);

		// Switch lock-on target si mouvement de souris suffisant
		if (FMath::Abs(LookAxisVector.X) > LockOnComponent->XAxisSwitchSensibility && LockOnComponent->GetCurrentLockedOnTarget())
			LockOnComponent->SwitchLockOnTarget(LookAxisVector.X);
	}
}

void AFlowSlayerCharacter::Dash(const FInputActionValue& Value)
{
	bWantsToDash = true;

	GetWorld()->GetTimerManager().ClearTimer(DashInputWindowTimer);
	GetWorld()->GetTimerManager().SetTimer(
		DashInputWindowTimer,
		this,
		&AFlowSlayerCharacter::ClearDashInput,
		DashInputWindowDuration,
		false
	);

	auto [isLMBPressed, isRMBPressed] { GetMouseButtonStates() };
	if (isLMBPressed || isRMBPressed)
	{
		OnDashAttackActionStarted();
		return;
	}

	if (CombatComponent->isAttacking() || GetCharacterMovement()->IsFalling() || !bCanDash || bIsDashing || MoveInputAxis.IsNearlyZero())
		return;

	if (MoveInputAxis.Y >= 0.1 && FwdDashAnim)
		AnimInstance->Montage_Play(FwdDashAnim, 1.0f);
	else if (MoveInputAxis.Y <= -0.1 && BwdDashAnim)
		AnimInstance->Montage_Play(BwdDashAnim, 1.0f);

	bCanDash = false;
	bIsDashing = true;

	TWeakObjectPtr<AFlowSlayerCharacter> WeakThis{ this };
	float dashDuration{ FwdDashAnim->GetPlayLength() };
	FTimerHandle dashingStateTimer;
	GetWorldTimerManager().SetTimer(
		dashingStateTimer,
		[WeakThis]()
		{
			if (WeakThis.IsValid())
				WeakThis->bIsDashing = false;
		},
		dashDuration,
		false
	);

	float totalDashCooldown{ dashDuration + dashCooldown };
	FTimerHandle dashCooldownTimer;
	GetWorldTimerManager().SetTimer(
		dashCooldownTimer,
		[WeakThis]()
		{
			if (WeakThis.IsValid())
				WeakThis->bCanDash = true;
		},
		totalDashCooldown,
		false
	);
}

TPair<bool, bool> AFlowSlayerCharacter::GetMouseButtonStates() const
{
	if (!PlayerController)
		return TPair<bool, bool>(false, false);

	bool isLMBPressed{ PlayerController->IsInputKeyDown(EKeys::LeftMouseButton) };
	bool isRMBPressed{ PlayerController->IsInputKeyDown(EKeys::RightMouseButton) };

	return TPair<bool, bool>{ isLMBPressed, isRMBPressed };
}

bool AFlowSlayerCharacter::GetInputKeyState(FKey inputKey) const
{
	if (!PlayerController)
		return false;

	return PlayerController->WasInputKeyJustPressed(inputKey) || PlayerController->IsInputKeyDown(inputKey);
}

void AFlowSlayerCharacter::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!CombatComponent->isAttacking())
		GetCharacterMovement()->bOrientRotationToMovement = true;
}

void AFlowSlayerCharacter::Jump()
{
	bWantsToJump = true;

	GetWorld()->GetTimerManager().ClearTimer(JumpInputWindowTimer);
	GetWorld()->GetTimerManager().SetTimer(
		JumpInputWindowTimer,
		this,
		&AFlowSlayerCharacter::ClearJumpInput,
		JumpInputWindowDuration,
		false
	);

	if (!CanJumpInternal_Implementation())
		return;

	bHasPressedJump = true;

	Super::Jump();
}

void AFlowSlayerCharacter::Falling()
{
	bWasJumpFall = bHasPressedJump;
	bHasPressedJump = false;
}

void AFlowSlayerCharacter::OnAttackTriggered(EAttackType attackType)
{
	CombatComponent->Attack(attackType, IsMoving(), GetCharacterMovement()->IsFalling());
}

void AFlowSlayerCharacter::OnLeftClickStarted(const FInputActionInstance& Value)
{
	EAttackType attackType{ EAttackType::StandingLight };

	if (GetCharacterMovement()->IsFalling() || GetInputKeyState(EKeys::SpaceBar) || GetCharacterMovement()->IsFlying())
	{
		OnJumpAttackActionStarted();
		return;
	}

	else if (GetInputKeyState(EKeys::LeftShift))
	{
		OnDashAttackActionStarted();
		return;
	}

	else if (GetInputKeyState(EKeys::S))
	{
		OnSlamActionStarted();
		return;
	}

	else if (GetInputKeyState(EKeys::Z))
		attackType = EAttackType::RunningLight;

	else
		attackType = EAttackType::StandingLight;

	OnAttackTriggered(attackType);

}

void AFlowSlayerCharacter::OnRightClickStarted(const FInputActionInstance& Value)
{
	EAttackType attackType{ EAttackType::StandingHeavy };

	if (GetCharacterMovement()->IsFalling() || GetCharacterMovement()->IsFlying())
	{
		OnJumpAttackActionStarted();
		return;
	}

	else if (GetInputKeyState(EKeys::LeftShift))
	{
		OnDashAttackActionStarted();
		return;
	}

	else if (GetInputKeyState(EKeys::S))
	{
		OnSlamActionStarted();
		return;
	}

	else if (GetInputKeyState(EKeys::Z))
		attackType = EAttackType::RunningHeavy;

	else
		attackType = EAttackType::StandingHeavy;

	OnAttackTriggered(attackType);
}

void AFlowSlayerCharacter::OnDashAttackActionStarted()
{
	EAttackType attackType{ EAttackType::None };

	auto [isLMBPressed, isRMBPressed] { GetMouseButtonStates() };
	// LMB: DashPierce (forward) or DashSpinningSlash (sideways)
	if (isLMBPressed)
	{
		if (GetInputKeyState(EKeys::Q) || GetInputKeyState(EKeys::D))
			attackType = EAttackType::DashSpinningSlash;
		else if (GetInputKeyState(EKeys::Z))
			attackType = EAttackType::DashPierce;
	}

	// RMB: DashDoubleSlash (forward) or DashBackSlash (backward)
	else if (isRMBPressed)
	{
		if (GetInputKeyState(EKeys::S))
			attackType = EAttackType::DashBackSlash;
		else if (GetInputKeyState(EKeys::Z))
			attackType = EAttackType::DashDoubleSlash;
	}

	if (attackType == EAttackType::None)
		return;

	if (bIsDashing)
		StopAnimMontage(GetCurrentMontage() == FwdDashAnim ? FwdDashAnim : BwdDashAnim);

	//TriggerAttackWithBuffer(attackType);
	OnAttackTriggered(attackType);
}

void AFlowSlayerCharacter::OnJumpAttackActionStarted()
{
	EAttackType attackType{ EAttackType::None };

	auto [isLMBPressed, isRMBPressed] { GetMouseButtonStates() };

	// LMB: JumpSlam (no direction) or JumpForwardSlam (forward)
	if (isLMBPressed)
	{
		if (GetInputKeyState(EKeys::S))
			attackType = EAttackType::JumpSlam;
		else if (GetInputKeyState(EKeys::Z))
			attackType = EAttackType::JumpForwardSlam;
		else
			attackType = EAttackType::AirCombo;
	}

	// RMB: JumpUpperSlam combo
	else if (isRMBPressed)
	{
		if (GetInputKeyState(EKeys::Z))
			attackType = EAttackType::JumpUpperSlam;
		else
			attackType = EAttackType::AerialSlam;
	}

	OnAttackTriggered(attackType);
}

void AFlowSlayerCharacter::OnLauncherActionStarted(const FInputActionInstance& Value)
{
	EAttackType attackType{ EAttackType::None };

	auto [isLMBPressed, isRMBPressed] { GetMouseButtonStates() };

	// LMB: Normal Launcher
	if (isLMBPressed)
		attackType = EAttackType::Launcher;

	// RMB: Power Launcher
	else if (isRMBPressed)
		attackType = EAttackType::PowerLauncher;

	OnAttackTriggered(attackType);
}

void AFlowSlayerCharacter::OnSpinAttackActionStarted(const FInputActionInstance& Value)
{
	EAttackType attackType{ EAttackType::None };

	auto [isLMBPressed, isRMBPressed] { GetMouseButtonStates() };

	// LMB or no button: SpinAttack
	if (isLMBPressed || (!isLMBPressed && !isRMBPressed))
		attackType = EAttackType::SpinAttack;

	// RMB: HorizontalSweep
	else if (isRMBPressed)
		attackType = EAttackType::HorizontalSweep;

	OnAttackTriggered(attackType);
}

void AFlowSlayerCharacter::OnForwardPowerActionStarted(const FInputActionInstance& Value)
{
	if (!PlayerController)
		return;

	EAttackType attackType{ EAttackType::None };

	// F + Z: PierceThrust
	if (GetInputKeyState(EKeys::Z))
		attackType = EAttackType::PierceThrust;

	// F + S: PowerSlash
	else if (GetInputKeyState(EKeys::S))
		attackType = EAttackType::PowerSlash;

	OnAttackTriggered(attackType);
}

void AFlowSlayerCharacter::OnSlamActionStarted()
{
	EAttackType attackType{ EAttackType::None };

	auto [isLMBPressed, isRMBPressed] { GetMouseButtonStates() };

	// LMB: DiagonalRetourne
	if (isLMBPressed)
		attackType = EAttackType::DiagonalRetourne;

	// RMB: GroundSlam
	else if (isRMBPressed)
		attackType = EAttackType::GroundSlam;

	OnAttackTriggered(attackType);
}

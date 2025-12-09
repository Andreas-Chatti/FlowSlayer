#include "FlowSlayerCharacter.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AFlowSlayerCharacter::AFlowSlayerCharacter()
{
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

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 900.0f;
	GetCharacterMovement()->AirControl = 0.8f;
	GetCharacterMovement()->BrakingDecelerationFalling = 5000.f;
	GetCharacterMovement()->GravityScale = 2.5f;

	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;


	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetRelativeLocation(FVector{ 0, 0, 80 });
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.f;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 10.f;
	CameraBoom->TargetArmLength = 550.0f; // The camera follows at this distance behind the character	
	CameraBoom->SocketOffset = FVector{ 0, 40, 70 };
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	CombatComponent = CreateDefaultSubobject<UFSCombatComponent>(TEXT("CombatComponent"));
	checkf(CombatComponent, TEXT("FATAL: CombatComponent is NULL or INVALID !"));

	JumpMaxCount = 2;
	CurrentHealth = MaxHealth;
}

void AFlowSlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CombatComponent->OnLockOnStopped.AddUObject(this, &AFlowSlayerCharacter::HandleOnLockOnStopped);
	OnAnimationCanceled.AddUObject(this, &AFlowSlayerCharacter::HandleOnAnimationCanceled);

	AnimInstance = GetMesh()->GetAnimInstance();
	checkf(AnimInstance, TEXT("AnimInstance is NULL"));

	/** Tag used when other classes trying to avoid direct dependance to this class */
	Tags.Add("Player");
}

void AFlowSlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector lastUpdateVelocity{ GetCharacterMovement()->GetLastUpdateVelocity() };
	float direction{ UKismetAnimationLibrary::CalculateDirection(lastUpdateVelocity, GetActorRotation()) };
	//if (direction != 0)
		//UE_LOG(LogTemp, Warning, TEXT("%f"), direction);
}

void AFlowSlayerCharacter::ReceiveDamage(float DamageAmount, AActor* DamageDealer)
{
	if (bIsDead)
		return;

    CurrentHealth -= DamageAmount;
    UE_LOG(LogTemp, Warning, TEXT("[%s] Received %.1f damage from %s - Health: %.1f/%.1f"),
        *GetName(), DamageAmount, *DamageDealer->GetName(), CurrentHealth, MaxHealth);

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
	APlayerController* PlayerController{ Cast<APlayerController>(GetController()) };
	if (PlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem{ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer())};
		if (Subsystem && DefaultMappingContext)
			Subsystem->RemoveMappingContext(DefaultMappingContext);

		DisableInput(PlayerController);
	}
}

void AFlowSlayerCharacter::ToggleLockOn(const FInputActionInstance& Value)
{
	if (!CombatComponent->IsLockedOnTarget())
		CombatComponent->EngageLockOn();

	else if (CombatComponent->IsLockedOnTarget())
		CombatComponent->DisengageLockOn();

	GetCharacterMovement()->MaxWalkSpeed = CombatComponent->IsLockedOnTarget() ? RunSpeedThreshold : SprintSpeedThreshold;
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

void AFlowSlayerCharacter::HandleOnLockOnStopped()
{
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeedThreshold;
}

void AFlowSlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	APlayerController* PlayerController{ Cast<APlayerController>(GetController()) };
	UEnhancedInputLocalPlayerSubsystem* Subsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()) };
	if (PlayerController && Subsystem)
		Subsystem->AddMappingContext(DefaultMappingContext, 0);

	// Set up action bindings
	UEnhancedInputComponent* EnhancedInputComponent{ Cast<UEnhancedInputComponent>(PlayerInputComponent) };
	if (EnhancedInputComponent)
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFlowSlayerCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AFlowSlayerCharacter::StopMoving);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFlowSlayerCharacter::Look);

		// Dashing
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AFlowSlayerCharacter::Dash);

		// LMB - Light attack
		EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Started, this, &AFlowSlayerCharacter::OnLeftClickStarted);
		EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Completed, this, &AFlowSlayerCharacter::OnLeftClickReleased);

		// RMB - Heavy attack
		EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &AFlowSlayerCharacter::OnRightClickStarted);
		EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Completed, this, &AFlowSlayerCharacter::OnRightClickReleased);

		// Switch Movement mode
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
	if (CombatComponent->IsLockedOnTarget())
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
		if (FMath::Abs(LookAxisVector.X) > CombatComponent->XAxisSwitchSensibility && CombatComponent->GetCurrentLockedOnTarget())
			CombatComponent->SwitchLockOnTarget(LookAxisVector.X);
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

	if (CombatComponent->isAttacking() || GetCharacterMovement()->IsFalling() || !bCanDash || bIsDashing)
		return;

	if (DashSound)
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DashSound, GetActorLocation());

	double charVelocity{ GetCharacterMovement()->Velocity.Length() };
	bool hasEnoughVelocity{ charVelocity > MIN_DASH_VELOCITY };
	if (!hasEnoughVelocity)
		return;

	bCanDash = false;
	bIsDashing = true;
	FVector launchVelocity{ GetCharacterMovement()->GetLastInputVector().GetSafeNormal() * dashDistance };
	LaunchCharacter(launchVelocity, false, false);
	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	FTimerHandle dashCooldownTimerHandle;
	GetWorldTimerManager().SetTimer(
		dashCooldownTimerHandle, 
		[this]() 
		{ 
		bCanDash = true; 
		bIsDashing = false; 
		}, 
		dashCooldown, 
		false
	);
}

void AFlowSlayerCharacter::RotatePlayerToCameraDirection()
{
	if (!Controller)
		return;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	
	const float rotationDuration{ 0.3f };
	FTimerHandle myTimer;
	GetWorld()->GetTimerManager().SetTimer(
		myTimer,
		[this]()
		{
			GetCharacterMovement()->bUseControllerDesiredRotation = false;
			GetCharacterMovement()->bOrientRotationToMovement = true;
		},
		rotationDuration,
		false);
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

void AFlowSlayerCharacter::OnAttackTriggered(UInputAction* inputAction)
{
	//if (bLeftClickPressed && bRightClickPressed)
		//attackType = UFSCombatComponent::EAttackType::HeavySpecial;

	if (!CombatComponent->isAttacking() && !AnimInstance->IsAnyMontagePlaying())
		RotatePlayerToCameraDirection();

	CombatComponent->Attack(inputAction, IsMoving(), GetCharacterMovement()->IsFalling());
}

void AFlowSlayerCharacter::OnLeftClickStarted(const FInputActionInstance& Value)
{
	bLeftClickPressed = true;

	GetWorld()->GetTimerManager().ClearTimer(InputBufferTimer);
	GetWorld()->GetTimerManager().SetTimer(
		InputBufferTimer,
		[this]() { OnAttackTriggered(LightAttackAction); },
		InputBufferDelay,
		false
	);
}

void AFlowSlayerCharacter::OnLeftClickReleased(const FInputActionInstance& Value)
{
	bLeftClickPressed = false;
}

void AFlowSlayerCharacter::OnRightClickStarted(const FInputActionInstance& Value)
{
	bRightClickPressed = true;

	GetWorld()->GetTimerManager().ClearTimer(InputBufferTimer);
	GetWorld()->GetTimerManager().SetTimer(
		InputBufferTimer,
		[this]() { OnAttackTriggered(HeavyAttackAction); },
		InputBufferDelay,
		false
	);
}

void AFlowSlayerCharacter::OnRightClickReleased(const FInputActionInstance& Value)
{
	bRightClickPressed = false;
}

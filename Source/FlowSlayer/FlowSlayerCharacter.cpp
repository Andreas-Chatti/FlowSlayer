#include "FlowSlayerCharacter.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AFlowSlayerCharacter::AFlowSlayerCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;	
	GetCharacterMovement()->RotationRate = FRotator{ 0.f, 500.f, 0.f };

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.0f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetRelativeLocation(FVector{ 0, 0, 60 });
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.f;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 10.f;
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->SocketOffset = FVector{ 0, 40, 60 };
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	CombatComponent = CreateDefaultSubobject<UFSCombatComponent>(TEXT("CombatComponent"));
	checkf(CombatComponent, TEXT("FATAL: CombatComponent is NULL or INVALID !"));

	CurrentHealth = MaxHealth;
}

void AFlowSlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	AnimInstance = GetMesh()->GetAnimInstance();
	checkf(AnimInstance, TEXT("AnimInstance is NULL"));

	/** Tag used when other classes trying to avoid direct dependance to this class */
	Tags.Add("Player");
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
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (deathMontage)
	{
		PlayAnimMontage(deathMontage);

		float MontageLength{ deathMontage->GetPlayLength() };
		float BlendOutTime{ deathMontage->BlendOut.GetBlendTime() };
		float TimerDelay{ MontageLength - BlendOutTime };  

		FTimerHandle DeathTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(
			DeathTimerHandle,
			[this]()
			{
				USkeletalMeshComponent* Mesh{ GetMesh() };
				if (Mesh && Mesh->GetAnimInstance())
					Mesh->bPauseAnims = true;
			},
			TimerDelay,
			false
		);
	}
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
		UEnhancedInputLocalPlayerSubsystem* Subsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()) };
		if (Subsystem && DefaultMappingContext)
			Subsystem->RemoveMappingContext(DefaultMappingContext);

		DisableInput(PlayerController);
	}
	GetCharacterMovement()->DisableMovement();
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

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFlowSlayerCharacter::Look);

		// Dashing
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &AFlowSlayerCharacter::Dash);

		// Attacking
		EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Started, this, &AFlowSlayerCharacter::OnAttackTriggered);
		EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &AFlowSlayerCharacter::OnAttackTriggered);
	}

	else
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
}

void AFlowSlayerCharacter::Move(const FInputActionValue& Value)
{
	if (CombatComponent->isAttacking())
		return;

	// input is a Vector2D
	FVector2D MovementVector{ Value.Get<FVector2D>() };
	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
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
	}
}

void AFlowSlayerCharacter::Dash(const FInputActionValue& Value)
{
	if (CombatComponent->isAttacking())
		return;

	double charVelocity{ GetCharacterMovement()->Velocity.Length() };
	bool hasEnoughVelocity{ charVelocity > MIN_DASH_VELOCITY };
	if (!bCanDash || !hasEnoughVelocity)
		return;

	bCanDash = false;
	FVector launchVelocity{ GetCharacterMovement()->GetLastInputVector().GetSafeNormal() * dashDistance };
	LaunchCharacter(launchVelocity, false, false);
	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	FTimerHandle dashCooldownTimerHandle;
	GetWorldTimerManager().SetTimer(dashCooldownTimerHandle, [this]() { bCanDash = true; }, dashCooldown, false);
}

void AFlowSlayerCharacter::RotatePlayerToCameraDirection()
{
	if (!Controller)
		return;

	FRotator ControlRotation{ Controller->GetControlRotation() };
	FRotator NewRotation{ FRotator(0.0f, ControlRotation.Yaw, 0.0f) };
	SetActorRotation(NewRotation);
}

void AFlowSlayerCharacter::Jump()
{
	if (!CanJumpInternal_Implementation())
		return;

	Super::Jump();

	if (IdleJumpMontage && !IsMoving())
		PlayAnimMontage(IdleJumpMontage);

	else if (ForwardJumpMontage && IsMoving())
		PlayAnimMontage(ForwardJumpMontage);
}

void AFlowSlayerCharacter::OnAttackTriggered(const FInputActionInstance& Value)
{
	if (!CombatComponent->isAttacking() && !AnimInstance->IsAnyMontagePlaying())
		RotatePlayerToCameraDirection();

	UFSCombatComponent::EAttackType attackTypeInput{ Value.GetSourceAction() == LightAttackAction ? UFSCombatComponent::EAttackType::Light : UFSCombatComponent::EAttackType::Heavy };
	CombatComponent->Attack(attackTypeInput, IsMoving(), GetCharacterMovement()->IsFalling());
}

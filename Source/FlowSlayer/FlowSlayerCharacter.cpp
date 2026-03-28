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

	FlowComponent = CreateDefaultSubobject<UFSFlowComponent>(TEXT("FlowComponent"));
	checkf(FlowComponent, TEXT("FATAL: FlowComponent is NULL or INVALID !"));

	DashComponent = CreateDefaultSubobject<UDashComponent>(TEXT("DashComponent"));
	checkf(DashComponent, TEXT("FATAL: DashComponent is NULL or INVALID !"));
	DashComponent->OnDashStarted.AddUObject(FlowComponent, &UFSFlowComponent::RemoveFlow);
	DashComponent->OnDashStarted.AddUObject(this, &AFlowSlayerCharacter::HandleOnDashStarted);
	DashComponent->CanAffordDash.BindUObject(FlowComponent, &UFSFlowComponent::HasEnoughFlow);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	checkf(HealthComponent, TEXT("FATAL: HealthComponent is NULL or INVALID !"));
	HealthComponent->OnDamageReceived.AddUniqueDynamic(FlowComponent, &UFSFlowComponent::OnPlayerHit);

	CombatComponent = CreateDefaultSubobject<UFSCombatComponent>(TEXT("CombatComponent"));
	checkf(CombatComponent, TEXT("FATAL: CombatComponent is NULL or INVALID !"));
	CombatComponent->OnHitLanded.AddUniqueDynamic(this, &AFlowSlayerCharacter::HandleOnHitLanded);
	CombatComponent->OnAttackingStarted.AddUniqueDynamic(DashComponent, &UDashComponent::OnAttackingStarted);
	CombatComponent->OnAttackingEnded.AddUniqueDynamic(DashComponent, &UDashComponent::OnAttackingEnded);
	OnHitReceived.AddUniqueDynamic(this, &AFlowSlayerCharacter::HandleOnHitReceived);

	InputManagerComponent = CreateDefaultSubobject<UInputManagerComponent>(TEXT("InputManagerComponent"));
	checkf(InputManagerComponent, TEXT("FATAL: InputManagerComponent is NULL or INVALID !"));

	ProgressionComponent = CreateDefaultSubobject<UProgressionComponent>(TEXT("ProgressionComponent"));
	checkf(ProgressionComponent, TEXT("FATAL: ProgressionComponent is NULL or INVALID !"));
	InputManagerComponent->OnMiddleMouseButtonClicked.BindUObject(this, &AFlowSlayerCharacter::ToggleLockOn);
	InputManagerComponent->OnLShiftKeyTriggered.BindUObject(this, &AFlowSlayerCharacter::OnDashAction);
	InputManagerComponent->OnSpaceKeyStarted.BindUObject(this, &AFlowSlayerCharacter::HandleOnSpaceKeyStarted);
	InputManagerComponent->OnSpaceKeyCompleted.BindUObject(this, &AFlowSlayerCharacter::HandleOnSpaceKeyCompleted);
	InputManagerComponent->OnMoveInput.BindUObject(this, &AFlowSlayerCharacter::HandleMoveInput);
	InputManagerComponent->OnLookInput.BindUObject(this, &AFlowSlayerCharacter::HandleLookInput);
	InputManagerComponent->OnAttackInputReceived.BindUObject(this, &AFlowSlayerCharacter::OnAttackInputActionReceived);
	InputManagerComponent->OnGuardActionTriggered.BindUObject(this, &AFlowSlayerCharacter::HandleGuardInput);
	InputManagerComponent->OnHealActionTriggered.BindUObject(this, &AFlowSlayerCharacter::HandleHealInput);
	
	JumpMaxCount = 2;
}

void AFlowSlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	LockOnComponent->OnLockOnStarted.BindUObject(this, &AFlowSlayerCharacter::HandleOnLockOnStarted);
	LockOnComponent->OnLockOnStopped.AddUObject(this, &AFlowSlayerCharacter::HandleOnLockOnStopped);
	OnAnimationCanceled.AddUniqueDynamic(this, &AFlowSlayerCharacter::HandleOnAnimationCanceled);

	AnimInstance = GetMesh()->GetAnimInstance();
	checkf(AnimInstance, TEXT("FATAL: AnimInstance is NULL or INVALID !"));

	InitializeInputActionMap();

	HealthComponent->OnDeath.BindUObject(this, &AFlowSlayerCharacter::HandleOnDeath);

	/** Tag used when other classes trying to avoid direct dependance to this class */
	Tags.Add("Player");

	InitializeHUD();

	APlayerController* pc{ InputManagerComponent->GetPlayerController() };
	if (pc)
	{
		FInputModeGameOnly inputMode;
		pc->SetInputMode(inputMode);
		pc->SetShowMouseCursor(false);
	}
}

void AFlowSlayerCharacter::InitializeHUD()
{
	APlayerController* playerController{ Cast<APlayerController>(GetController()) };

	if (!HUDWidgetClass || !playerController)
		return;
	HUDWidgetInstance = CreateWidget<UUserWidget>(playerController, HUDWidgetClass);
	if (HUDWidgetInstance)
		HUDWidgetInstance->AddToViewport();
}

bool AFlowSlayerCharacter::CanJumpInternal_Implementation() const
{
	if (CombatComponent->IsAttacking())
		return false;
	
	return Super::CanJumpInternal_Implementation();
}

void AFlowSlayerCharacter::HandleOnDeath()
{
	AnimInstance->StopAllMontages(0.3f);
	InputManagerComponent->DisableAllInputs();
	OnPlayerDeath.Broadcast(this);
}

void AFlowSlayerCharacter::HandleOnAnimationCanceled()
{
	CombatComponent->CancelAttack();
}

void AFlowSlayerCharacter::HandleOnLockOnStarted(AActor* lockedOnTarget)
{
	CombatComponent->SetLockedOnTargetRef(lockedOnTarget);
	GetCharacterMovement()->MaxWalkSpeed = RunSpeedThreshold;
}

void AFlowSlayerCharacter::HandleOnLockOnStopped()
{
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeedThreshold;
	CombatComponent->SetLockedOnTargetRef(nullptr);
}

void AFlowSlayerCharacter::HandleOnHitLanded(AActor* hitActor, const FVector& hitLocation, const FAttackData& usedAttack)
{
	FlowComponent->HandleOnHitLanded(hitActor, hitLocation, usedAttack.Damage, usedAttack.FlowReward);
}

void AFlowSlayerCharacter::ToggleLockOn() const
{
	if (!LockOnComponent->IsLockedOnTarget())
		LockOnComponent->EngageLockOn();

	else
		LockOnComponent->DisengageLockOn();
}

void AFlowSlayerCharacter::HandleOnSpaceKeyStarted()
{
	Jump();

	if (CombatComponent->IsGuarding())
		CombatComponent->ToggleGuard();
}

void AFlowSlayerCharacter::HandleOnSpaceKeyCompleted()
{
	StopJumping();
}

void AFlowSlayerCharacter::HandleOnDashStarted(float flowCost)
{
	if (CombatComponent->IsGuarding())
		CombatComponent->ToggleGuard();
}

void AFlowSlayerCharacter::OnDashAction()
{
	DashComponent->StartDash(InputManagerComponent->GetMoveInputAxis());
}

void AFlowSlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	InputManagerComponent->SetupInputBindings(PlayerInputComponent);
}

void AFlowSlayerCharacter::HandleMoveInput(FVector2D moveAxis)
{
	if (CombatComponent->IsGuarding())
		CombatComponent->ToggleGuard();

	if (LockOnComponent->IsLockedOnTarget())
	{
		AddMovementInput(GetActorForwardVector(), moveAxis.Y);
		AddMovementInput(GetActorRightVector(), moveAxis.X);
		return;
	}

	const FRotator ControlRot{ Controller->GetControlRotation() };
	const FRotator YawRotation{ 0, ControlRot.Yaw, 0 };

	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), moveAxis.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), moveAxis.X);
}

void AFlowSlayerCharacter::HandleLookInput(FVector2D lookAxis)
{
	if (!Controller)
		return;

	AddControllerYawInput(lookAxis.X);
	AddControllerPitchInput(lookAxis.Y);

	if (FMath::Abs(lookAxis.X) > 1.0f && LockOnComponent->IsLockedOnTarget())
		LockOnComponent->SwitchLockOnTarget(lookAxis.X);
}

void AFlowSlayerCharacter::HandleGuardInput()
{
	CombatComponent->ToggleGuard();
}

void AFlowSlayerCharacter::HandleHealInput()
{
	float healCost{ HealthComponent->GetHealFlowCost() };
	if (FlowComponent->HasEnoughFlow(healCost) && !HealthComponent->IsHealOnCooldown())
	{
		HealthComponent->Heal();
		FlowComponent->ConsumeFlow(healCost);
	}
}

void AFlowSlayerCharacter::NotifyHitReceived(AActor* instigatorActor, const FAttackData& usedAttack)
{
	OnHitReceived.Broadcast(instigatorActor, usedAttack);
}

void AFlowSlayerCharacter::InitializeInputActionMap()
{
	InputActionToAttackType.Add(InputManagerComponent->GetHeavyAttackAction(),			 EAttackType::StandingHeavy);
	InputActionToAttackType.Add(InputManagerComponent->GetLightAttackAction(),			 EAttackType::StandingLight);
	InputActionToAttackType.Add(InputManagerComponent->GetDashPierceAction(),            EAttackType::DashPierce);
	InputActionToAttackType.Add(InputManagerComponent->GetDashSpinningSlashAction(),     EAttackType::DashSpinningSlash);
	InputActionToAttackType.Add(InputManagerComponent->GetDashDoubleSlashAction(),       EAttackType::DashDoubleSlash);
	InputActionToAttackType.Add(InputManagerComponent->GetDashBackSlashAction(),         EAttackType::DashBackSlash);
	InputActionToAttackType.Add(InputManagerComponent->GetJumpSlamAttackAction(),        EAttackType::JumpSlam);
	InputActionToAttackType.Add(InputManagerComponent->GetJumpForwardSlamAttackAction(), EAttackType::JumpForwardSlam);
	InputActionToAttackType.Add(InputManagerComponent->GetJumpUpperSlamAttackAction(),   EAttackType::JumpUpperSlam);
	InputActionToAttackType.Add(InputManagerComponent->GetLauncherAttackAction(),        EAttackType::Launcher);
	InputActionToAttackType.Add(InputManagerComponent->GetPowerLauncherAttackAction(),   EAttackType::PowerLauncher);
	InputActionToAttackType.Add(InputManagerComponent->GetSpinAttackAction(),            EAttackType::SpinAttack);
	InputActionToAttackType.Add(InputManagerComponent->GetHorizontalSweepAttackAction(), EAttackType::HorizontalSweep);
	InputActionToAttackType.Add(InputManagerComponent->GetPowerSlashAttackAction(),      EAttackType::PowerSlash);
	InputActionToAttackType.Add(InputManagerComponent->GetPierceThrustAttackAction(),    EAttackType::PierceThrust);
	InputActionToAttackType.Add(InputManagerComponent->GetGroundSlamAttackAction(),      EAttackType::GroundSlam);
	InputActionToAttackType.Add(InputManagerComponent->GetDiagonalRetourneAttackAction(), EAttackType::DiagonalRetourne);
}

void AFlowSlayerCharacter::OnAttackInputActionReceived(const UInputAction* inputAction)
{
	const EAttackType* foundType{ InputActionToAttackType.Find(inputAction) };
	if (!foundType)
		return;

	EAttackType attackType{ *foundType };

	// Handle Light/Heavy standing/running attacks
	if (inputAction == InputManagerComponent->GetLightAttackAction())
	{
		if (GetCharacterMovement()->IsFalling() || GetCharacterMovement()->IsFlying())
			attackType = EAttackType::AirCombo;

		else
			attackType = GetSpeed() > RunSpeedThreshold ? EAttackType::RunningLight : EAttackType::StandingLight;
	}

	else if (inputAction == InputManagerComponent->GetHeavyAttackAction())
	{
		if (GetCharacterMovement()->IsFalling() || GetCharacterMovement()->IsFlying())
			attackType = EAttackType::AerialSlam;

		else
			attackType = GetSpeed() > RunSpeedThreshold ? EAttackType::RunningHeavy : EAttackType::StandingHeavy;
	}

	CombatComponent->OnAttackInputReceived(attackType);
}

void AFlowSlayerCharacter::HandleOnHitReceived(AActor* instigatorActor, const FAttackData& usedAttack)
{
	if (CombatComponent->IsGuarding())
		return;

	CombatComponent->GetHitFeedbackComponent()->OnReceiveHit(instigatorActor->GetActorLocation(), usedAttack.KnockbackForce, usedAttack.KnockbackUpForce);
	HealthComponent->ReceiveDamage(usedAttack.Damage, instigatorActor);
}

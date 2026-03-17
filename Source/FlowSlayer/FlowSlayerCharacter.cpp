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
	InputManagerComponent->OnMiddleMouseButtonClicked.BindUObject(this, &AFlowSlayerCharacter::ToggleLockOn);
	InputManagerComponent->OnLShiftKeyTriggered.BindUObject(this, &AFlowSlayerCharacter::OnDashAction);
	InputManagerComponent->OnLMBTriggered.BindUObject(this, &AFlowSlayerCharacter::OnLeftClickAction);
	InputManagerComponent->OnRMBTriggered.BindUObject(this, &AFlowSlayerCharacter::OnRightClickAction);
	InputManagerComponent->OnAKeyTriggered.BindUObject(this, &AFlowSlayerCharacter::OnLauncherAction);
	InputManagerComponent->OnEKeyTriggered.BindUObject(this, &AFlowSlayerCharacter::OnSpinAttackAction);
	InputManagerComponent->OnFKeyTriggered.BindUObject(this, &AFlowSlayerCharacter::OnForwardPowerAction);
	InputManagerComponent->OnSwitchLockOnTargetKeyTriggered.BindUObject(LockOnComponent, &UFSLockOnComponent::SwitchLockOnTarget);
	
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

	HealthComponent->OnDeath.BindUObject(this, &AFlowSlayerCharacter::HandleOnDeath);

	/** Tag used when other classes trying to avoid direct dependance to this class */
	Tags.Add("Player");

	InitializeHUD();
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
	InputManagerComponent->SetIsLockedOn(static_cast<bool>(lockedOnTarget));
}

void AFlowSlayerCharacter::HandleOnLockOnStopped()
{
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeedThreshold;
	CombatComponent->SetLockedOnTargetRef(nullptr);
	InputManagerComponent->SetIsLockedOn(false);
}

void AFlowSlayerCharacter::HandleOnHitLanded(AActor* hitActor, const FVector& hitLocation, const FAttackData& usedAttack)
{
	FlowComponent->HandleOnHitLanded(hitActor, hitLocation, usedAttack.Damage, usedAttack.FlowReward);
}

void AFlowSlayerCharacter::ToggleLockOn() const
{
	if (!LockOnComponent->IsLockedOnTarget())
	{
		LockOnComponent->EngageLockOn();
		GetCharacterMovement()->MaxWalkSpeed = RunSpeedThreshold;
	}

	else
	{
		LockOnComponent->DisengageLockOn();
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeedThreshold;
	}
}

void AFlowSlayerCharacter::OnDashAction()
{
	EAttackType attackType{ GetDashAttackFromInput() };

	if (attackType == EAttackType::None)
		DashComponent->StartDash(InputManagerComponent->GetMoveInputAxis());
	else
		OnAttackTriggered(attackType);
}

EAttackType AFlowSlayerCharacter::GetDashAttackFromInput()
{
	auto [isLMBPressed, isRMBPressed] { InputManagerComponent->GetMouseButtonStates() };
	if (isLMBPressed)
	{
		// LSHIFT + LMB + 'Q' / 'D'
		if (InputManagerComponent->GetInputKeyState(EKeys::Q) || InputManagerComponent->GetInputKeyState(EKeys::D))
			return EAttackType::DashSpinningSlash;

		// LSHIFT + LMB + 'Z'
		else if (InputManagerComponent->GetInputKeyState(EKeys::Z))
			return EAttackType::DashPierce;
	}

	// LSHIFT + E
	else if (InputManagerComponent->GetInputKeyState(EKeys::E))
		return EAttackType::DashBackSlash;

	// LSHIFT + F
	else if (InputManagerComponent->GetInputKeyState(EKeys::F))
		return EAttackType::DashDoubleSlash;

	return EAttackType::None;
}

void AFlowSlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	InputManagerComponent->SetupInputBindings(PlayerInputComponent);
}

void AFlowSlayerCharacter::NotifyHitReceived(AActor* instigatorActor, const FAttackData& usedAttack)
{
	OnHitReceived.Broadcast(instigatorActor, usedAttack);
}

void AFlowSlayerCharacter::OnLeftClickAction()
{
	// When Shift is held, OnDashAction handles the attack
	if (InputManagerComponent->GetInputKeyState(EKeys::LeftShift))
		return;

	if (GetCharacterMovement()->IsFalling() || InputManagerComponent->GetInputKeyState(EKeys::SpaceBar) || GetCharacterMovement()->IsFlying())
	{
		OnAttackTriggered(GetJumpAttackFromInput());
		return;
	}

	else if (InputManagerComponent->GetInputKeyState(EKeys::S))
	{
		OnAttackTriggered(GetSlamAttackFromInput());
		return;
	}

	EAttackType attackType{ GetSpeed() > RunSpeedThreshold ? EAttackType::RunningLight : EAttackType::StandingLight };
	OnAttackTriggered(attackType);
}

void AFlowSlayerCharacter::OnRightClickAction()
{
	// When Shift is held, OnDashAction handles the attack
	if (InputManagerComponent->GetInputKeyState(EKeys::LeftShift))
		return;

	if (GetCharacterMovement()->IsFalling() || GetCharacterMovement()->IsFlying())
	{
		OnAttackTriggered(GetJumpAttackFromInput());
		return;
	}

	if (InputManagerComponent->GetInputKeyState(EKeys::S))
	{
		OnAttackTriggered(GetSlamAttackFromInput());
		return;
	}

	EAttackType attackType{ GetSpeed() > RunSpeedThreshold ? EAttackType::RunningHeavy : EAttackType::StandingHeavy };
	OnAttackTriggered(attackType);
}

void AFlowSlayerCharacter::OnLauncherAction()
{
	OnAttackTriggered(GetLauncherAttackFromInput());
}

void AFlowSlayerCharacter::OnSpinAttackAction()
{
	// When Shift is held, OnDashAction handles the attack
	if (InputManagerComponent->GetInputKeyState(EKeys::LeftShift))
		return;

	OnAttackTriggered(GetSpinAttackFromInput());
}

void AFlowSlayerCharacter::OnForwardPowerAction()
{
	// When Shift is held, OnDashAction handles the attack
	if (InputManagerComponent->GetInputKeyState(EKeys::LeftShift))
		return;

	OnAttackTriggered(GetForwardPowerAttackFromInput());
}

EAttackType AFlowSlayerCharacter::GetJumpAttackFromInput()
{
	auto [isLMBPressed, isRMBPressed] { InputManagerComponent->GetMouseButtonStates() };

	if (isLMBPressed)
	{
		if (InputManagerComponent->GetInputKeyState(EKeys::S))
			return EAttackType::JumpSlam;
		else if (InputManagerComponent->GetInputKeyState(EKeys::Z))
			return EAttackType::JumpForwardSlam;
		else if (CombatComponent->CanAirAttack())
			return EAttackType::AirCombo;
	}
	else if (isRMBPressed)
	{
		if (InputManagerComponent->GetInputKeyState(EKeys::Z))
			return EAttackType::JumpUpperSlam;
		else
			return EAttackType::AerialSlam;
	}

	return EAttackType::None;
}

EAttackType AFlowSlayerCharacter::GetSlamAttackFromInput()
{
	auto [isLMBPressed, isRMBPressed] { InputManagerComponent->GetMouseButtonStates() };

	if (isLMBPressed)
		return EAttackType::DiagonalRetourne;
	else if (isRMBPressed)
		return EAttackType::GroundSlam;

	return EAttackType::None;
}

EAttackType AFlowSlayerCharacter::GetLauncherAttackFromInput()
{
	auto [isLMBPressed, isRMBPressed] { InputManagerComponent->GetMouseButtonStates() };

	if (isLMBPressed)
		return EAttackType::Launcher;
	else if (isRMBPressed)
		return EAttackType::PowerLauncher;

	return EAttackType::None;
}

EAttackType AFlowSlayerCharacter::GetSpinAttackFromInput()
{
	auto [isLMBPressed, isRMBPressed] { InputManagerComponent->GetMouseButtonStates() };

	if (isRMBPressed)
		return EAttackType::HorizontalSweep;

	return EAttackType::SpinAttack;
}

EAttackType AFlowSlayerCharacter::GetForwardPowerAttackFromInput()
{
	if (InputManagerComponent->GetInputKeyState(EKeys::Z))
		return EAttackType::PierceThrust;
	else if (InputManagerComponent->GetInputKeyState(EKeys::S))
		return EAttackType::PowerSlash;

	return EAttackType::None;
}

void AFlowSlayerCharacter::OnAttackTriggered(EAttackType attackType)
{
	CombatComponent->OnAttackInputReceived(attackType);
}

void AFlowSlayerCharacter::HandleOnHitReceived(AActor* instigatorActor, const FAttackData& usedAttack)
{
	CombatComponent->GetHitFeedbackComponent()->OnReceiveHit(instigatorActor->GetActorLocation(), usedAttack.KnockbackForce, usedAttack.KnockbackUpForce);
	HealthComponent->ReceiveDamage(usedAttack.Damage, instigatorActor);
}

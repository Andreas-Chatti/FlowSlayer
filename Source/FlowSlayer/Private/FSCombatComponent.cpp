#include "FSCombatComponent.h"

UFSCombatComponent::UFSCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    HitboxComponent = CreateDefaultSubobject<UHitboxComponent>(TEXT("HitboxComponent"));
    checkf(HitboxComponent, TEXT("FATAL: HitboxComponent is NULL or INVALID !"));

    HitFeedBackComponent = CreateDefaultSubobject<UHitFeedbackComponent>(TEXT("HitFeedBackComponent"));
    checkf(HitFeedBackComponent, TEXT("FATAL: HitFeedBackComponent is NULL or INVALID !"));
}

void UFSCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bComboCounterActive)
        return;

    ComboTimeRemaining -= DeltaTime;

    if (ComboTimeRemaining <= 0)
        ResetComboCounter();
}

void UFSCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    PlayerOwner = Cast<ACharacter>(GetOwner());
    AnimInstance = PlayerOwner->GetMesh()->GetAnimInstance();
    InitializeAndAttachWeapon();

    checkf(PlayerOwner && AnimInstance && equippedWeapon && HitboxComponent, TEXT("FATAL: One or more Core CombatComponent variables are NULL"));

    HitboxComponent->OnHitboxHitLanded.BindUObject(this, &UFSCombatComponent::HandleOnHitLanded);
    HitboxComponent->SetOwnerWeaponRef(equippedWeapon);

    OnComboInputWindowOpened.BindUObject(this, &UFSCombatComponent::HandleComboInputWindowOpened);
    OnComboInputWindowClosed.BindUObject(this, &UFSCombatComponent::HandleComboInputWindowClosed);

    // Initialize combo attack data (damage, knockback, ChainableAttacks)
    InitializeComboAttackData();

    // Initialize combo lookup table for fast attack selection
    InitializeComboLookupTable();

    PlayerOwner->LandedDelegate.AddDynamic(this, &UFSCombatComponent::HandleOnLanded);
}

bool UFSCombatComponent::InitializeAndAttachWeapon()
{
    if (!weaponClass)
        return false;

    FActorSpawnParameters spawnParams;
    spawnParams.Owner = PlayerOwner;
    equippedWeapon = GetWorld()->SpawnActor<AFSWeapon>(weaponClass, spawnParams);
    if (!equippedWeapon || !equippedWeapon->AttachToComponent(PlayerOwner->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("WeaponSocket")))
        return false;

    return true;
}

void UFSCombatComponent::InitializeComboLookupTable()
{
    // Basic attacks (movement-dependent)
    ComboLookupTable.Add(EAttackType::StandingLight, &StandingLightCombo);
    ComboLookupTable.Add(EAttackType::StandingHeavy, &StandingHeavyCombo);
    ComboLookupTable.Add(EAttackType::RunningLight, &RunningLightCombo);
    ComboLookupTable.Add(EAttackType::RunningHeavy, &RunningHeavyCombo);

    // Dash attacks
    ComboLookupTable.Add(EAttackType::DashPierce, &DashPierceAttack);
    ComboLookupTable.Add(EAttackType::DashSpinningSlash, &DashSpinningSlashAttack);
    ComboLookupTable.Add(EAttackType::DashDoubleSlash, &DashDoubleSlashAttack);
    ComboLookupTable.Add(EAttackType::DashBackSlash, &DashBackSlashAttack);

    // Jump attacks
    ComboLookupTable.Add(EAttackType::JumpSlam, &JumpSlamAttack);
    ComboLookupTable.Add(EAttackType::JumpForwardSlam, &JumpForwardSlamAttack);
    ComboLookupTable.Add(EAttackType::JumpUpperSlam, &JumpUpperSlamComboAttack);
    ComboLookupTable.Add(EAttackType::AirCombo, &AirCombo);
    ComboLookupTable.Add(EAttackType::AerialSlam, &AerialSlamAttack);

    // Launcher attacks
    ComboLookupTable.Add(EAttackType::Launcher, &LauncherAttack);
    ComboLookupTable.Add(EAttackType::PowerLauncher, &PowerLauncherAttack);

    // Spin attacks
    ComboLookupTable.Add(EAttackType::SpinAttack, &SpinAttack);
    ComboLookupTable.Add(EAttackType::HorizontalSweep, &HorizontalSweepAttack);

    // Power attacks
    ComboLookupTable.Add(EAttackType::PowerSlash, &PowerSlashAttack);
    ComboLookupTable.Add(EAttackType::PierceThrust, &PierceThrustAttack);

    // Slam attacks
    ComboLookupTable.Add(EAttackType::GroundSlam, &GroundSlamAttack);
    ComboLookupTable.Add(EAttackType::DiagonalRetourne, &DiagonalRetourneAttack);
}

void UFSCombatComponent::InitializeComboAttackData()
{
    // === STANDING LIGHT COMBO (7 attacks) ===
    StandingLightCombo.Attacks.SetNum(7);
    StandingLightCombo.Attacks[0] = *GetAttackData("StandingLight_0");
    StandingLightCombo.Attacks[1] = *GetAttackData("StandingLight_1");
    StandingLightCombo.Attacks[2] = *GetAttackData("StandingLight_2");
    StandingLightCombo.Attacks[3] = *GetAttackData("StandingLight_3");
    StandingLightCombo.Attacks[4] = *GetAttackData("StandingLight_4");
    StandingLightCombo.Attacks[5] = *GetAttackData("StandingLight_5");
    StandingLightCombo.Attacks[6] = *GetAttackData("StandingLight_6");

    // === STANDING HEAVY COMBO (4 attacks) ===
    StandingHeavyCombo.Attacks.SetNum(4);
    StandingHeavyCombo.Attacks[0] = *GetAttackData("StandingHeavy_0");
    StandingHeavyCombo.Attacks[1] = *GetAttackData("StandingHeavy_1");
    StandingHeavyCombo.Attacks[2] = *GetAttackData("StandingHeavy_2");
    StandingHeavyCombo.Attacks[3] = *GetAttackData("StandingHeavy_3");

    // === RUNNING LIGHT COMBO (3 attacks) ===
    RunningLightCombo.Attacks.SetNum(3);
    RunningLightCombo.Attacks[0] = *GetAttackData("RunningLight_0");
    RunningLightCombo.Attacks[1] = *GetAttackData("RunningLight_1");
    RunningLightCombo.Attacks[2] = *GetAttackData("RunningLight_2");

    // === RUNNING HEAVY COMBO (3 attacks) ===
    RunningHeavyCombo.Attacks.SetNum(3);
    RunningHeavyCombo.Attacks[0] = *GetAttackData("RunningHeavy_0");
    RunningHeavyCombo.Attacks[1] = *GetAttackData("RunningHeavy_1");
    RunningHeavyCombo.Attacks[2] = *GetAttackData("RunningHeavy_2");

    // === DASH PIERCE ===
    DashPierceAttack.Attacks.SetNum(1);
    DashPierceAttack.Attacks[0] = *GetAttackData("DashPierce");

    // === DASH SPINNING SLASH ===
    DashSpinningSlashAttack.Attacks.SetNum(1);
    DashSpinningSlashAttack.Attacks[0] = *GetAttackData("DashSpinningSlash");

    // === DASH DOUBLE SLASH ===
    DashDoubleSlashAttack.Attacks.SetNum(1);
    DashDoubleSlashAttack.Attacks[0] = *GetAttackData("DashDoubleSlash");

    // === DASH BACK SLASH ===
    DashBackSlashAttack.Attacks.SetNum(1);
    DashBackSlashAttack.Attacks[0] = *GetAttackData("DashBackSlash");

    // === JUMP SLAM ===
    JumpSlamAttack.Attacks.SetNum(1);
    JumpSlamAttack.Attacks[0] = *GetAttackData("JumpSlam");
    JumpSlamAttack.Attacks[0].OnAttackExecuted.BindLambda([this]()
        {
            if (PlayerOwner->GetCharacterMovement()->IsFalling() || PlayerOwner->GetCharacterMovement()->IsFlying())
                AnimInstance->Montage_JumpToSection(FName("AirStart"), AnimInstance->GetCurrentActiveMontage());
            else
                AnimInstance->Montage_JumpToSection(FName("ComboStart"), AnimInstance->GetCurrentActiveMontage());
        });

    // === JUMP FORWARD SLAM ===
    JumpForwardSlamAttack.Attacks.SetNum(1);
    JumpForwardSlamAttack.Attacks[0] = *GetAttackData("JumpForwardSlam");
    JumpForwardSlamAttack.Attacks[0].OnAttackExecuted.BindLambda([this]()
        {
            if (PlayerOwner->GetCharacterMovement()->IsFalling() || PlayerOwner->GetCharacterMovement()->IsFlying())
                AnimInstance->Montage_JumpToSection(FName("AirStart"), AnimInstance->GetCurrentActiveMontage());
            else
                AnimInstance->Montage_JumpToSection(FName("ComboStart"), AnimInstance->GetCurrentActiveMontage());
        });

    // === JUMP UPPER SLAM ===
    JumpUpperSlamComboAttack.Attacks.SetNum(1);
    JumpUpperSlamComboAttack.Attacks[0] = *GetAttackData("JumpUpperSlam");
    JumpUpperSlamComboAttack.Attacks[0].OnAttackExecuted.BindLambda([this]()
        {
            if (PlayerOwner->GetCharacterMovement()->IsFalling() || PlayerOwner->GetCharacterMovement()->IsFlying())
                AnimInstance->Montage_JumpToSection(FName("AirStart"), AnimInstance->GetCurrentActiveMontage());
            else
                AnimInstance->Montage_JumpToSection(FName("ComboStart"), AnimInstance->GetCurrentActiveMontage());
        });

    // === LAUNCHER ===
    LauncherAttack.Attacks.SetNum(1);
    LauncherAttack.Attacks[0] = *GetAttackData("Launcher");

    // === POWER LAUNCHER ===
    PowerLauncherAttack.Attacks.SetNum(1);
    PowerLauncherAttack.Attacks[0] = *GetAttackData("PowerLauncher");

    // === SPIN ATTACK ===
    SpinAttack.Attacks.SetNum(1);
    SpinAttack.Attacks[0] = *GetAttackData("SpinAttack");

    // === HORIZONTAL SWEEP ===
    HorizontalSweepAttack.Attacks.SetNum(1);
    HorizontalSweepAttack.Attacks[0] = *GetAttackData("HorizontalSweep");

    // === PIERCE THRUST ===
    PierceThrustAttack.Attacks.SetNum(1);
    PierceThrustAttack.Attacks[0] = *GetAttackData("PierceThrust");

    // === POWER SLASH ===
    PowerSlashAttack.Attacks.SetNum(1);
    PowerSlashAttack.Attacks[0] = *GetAttackData("PowerSlash");

    // === DIAGONAL RETOURNE ===
    DiagonalRetourneAttack.Attacks.SetNum(1);
    DiagonalRetourneAttack.Attacks[0] = *GetAttackData("DiagonalRetourne");

    // === GROUND SLAM ===
    GroundSlamAttack.Attacks.SetNum(1);
    GroundSlamAttack.Attacks[0] = *GetAttackData("GroundSlam");

    // === AIR COMBO (3 attacks) ===
    AirCombo.Attacks.SetNum(3);
    AirCombo.Attacks[0] = *GetAttackData("AirCombo_0");
    AirCombo.Attacks[1] = *GetAttackData("AirCombo_1");
    AirCombo.Attacks[2] = *GetAttackData("AirCombo_2");
    AirCombo.Attacks[2].OnAttackExecuted.BindLambda([this]() { bCanAirAttack = false; });

    // === AERIAL SLAM ===
    AerialSlamAttack.Attacks.SetNum(1);
    AerialSlamAttack.Attacks[0] = *GetAttackData("AerialSlam");
}


////////////////////////////////////////////////
/*
* Core combo-related methods 
*/
////////////////////////////////////////////////
void UFSCombatComponent::OnAttackInputReceived(EAttackType attackType)
{
    if (bIsAttacking && !bComboInputWindowOpen)
        return;

    else if (bIsAttacking && bComboInputWindowOpen)
    {
        OnComboWindowInputReceived(attackType);
        return;
    }

    if (AnimInstance && AnimInstance->IsAnyMontagePlaying())
        return;

    OngoingCombo = GetComboFromContext(attackType);
    if (!OngoingCombo || !OngoingCombo->IsValid())
        return;
    
    const FAttackData* ongoingAttack{ GetOngoingAttack() };
    if (!ongoingAttack || (ongoingAttack && ongoingAttack->bOnCooldown))
        return;

    UAnimMontage* animAttack{ ongoingAttack->Montage };
    if (!animAttack)
        return;

    bIsAttacking = true;
    bGuardActivated = false;

    ExecuteAttack(animAttack);

    OnAttackingStarted.Broadcast();
}

FCombo* UFSCombatComponent::GetComboFromContext(EAttackType attackType)
{
    if (attackType == EAttackType::None)
        return nullptr;

    // Lookup combo from table
    FCombo** foundCombo{ ComboLookupTable.Find(attackType) };
    if (!foundCombo || !(*foundCombo)->IsValid())
        return nullptr;

    bool bIsFalling{ PlayerOwner->GetCharacterMovement()->IsFalling() };
    bool bIsFlying{ PlayerOwner->GetCharacterMovement()->IsFlying() };
    bool bIsAirAttack{ (*foundCombo)->GetFirstAttack() && (*foundCombo)->GetFirstAttack()->AttackContext == EAttackDataContext::Air };

    // First attack of that combo that can be performed in air and ground
    bool bIsAirAndGroundAttack{ (*foundCombo)->GetFirstAttack() && (*foundCombo)->GetFirstAttack()->AttackContext == EAttackDataContext::Any };
    if (bIsAirAndGroundAttack)
        return *foundCombo;

    // Reject air-only attacks on ground
    else if (bIsAirAttack && !bIsFalling && !bIsFlying)
        return nullptr;

    // Reject ground-only attacks in air
    else if (!bIsAirAttack && (bIsFalling || bIsFlying))
        return nullptr;

    return *foundCombo;
}

void UFSCombatComponent::ExecuteAttack(UAnimMontage* attackMontage)
{
    PlayerOwner->PlayAnimMontage(attackMontage);

    FAttackData* ongoingAttack{ &OngoingCombo->Attacks[ComboIndex] };
    ongoingAttack->OnAttackExecuted.ExecuteIfBound();
    ongoingAttack->StartCooldown(GetWorld());
}

void UFSCombatComponent::CancelAttack(float blendOutTime)
{
    AnimInstance->StopAllMontages(blendOutTime);
    ResetComboState();
    HitboxComponent->OnActiveFrameStopped.ExecuteIfBound();
}

FAttackData* UFSCombatComponent::GetAttackData(FName rowName) const
{
    if (!AttackDataTable)
        return nullptr;

    return AttackDataTable->FindRow<FAttackData>(rowName, TEXT("GetAttackData"));
}

void UFSCombatComponent::HandleComboInputWindowOpened()
{
    bComboInputWindowOpen = true;
}

void UFSCombatComponent::HandleComboInputWindowClosed()
{
    bComboInputWindowOpen = false;

    if (!OngoingCombo || !bContinueCombo)
    {
        ResetComboState();
        return;
    }

    ContinueCombo();
}

void UFSCombatComponent::HandleOnLanded(const FHitResult& Hit)
{
    bCanAirAttack = true;
}

void UFSCombatComponent::ChainingToNextCombo()
{
    bChainingToNewCombo = false;

    if (!PendingCombo || !PendingCombo->IsValid())
        return;

    OngoingCombo = PendingCombo;
    ComboIndex = 0;
    PendingCombo = nullptr;

    const FAttackData* ongoingAttack{ GetOngoingAttack() };
    if (!ongoingAttack)
        return;

    UAnimMontage* attackMontage{ ongoingAttack->Montage };
    if (!attackMontage)
        return;

    ExecuteAttack(attackMontage);
}

void UFSCombatComponent::ContinueCombo()
{
    if (!bContinueCombo)
        return;

    bContinueCombo = false;
    ++ComboIndex;

    if (bChainingToNewCombo)
    {
        ChainingToNextCombo();
        return;
    }

    if (!OngoingCombo || !OngoingCombo->IsValid())
        return;

    const FAttackData* ongoingAttack{ GetOngoingAttack() };
    if (!ongoingAttack)
        return;

    UAnimMontage* attackMontage{ ongoingAttack->Montage };
    if (!attackMontage)
        return;

    ExecuteAttack(attackMontage);
}

void UFSCombatComponent::OnComboWindowInputReceived(EAttackType attackType)
{
    if (!OngoingCombo)
        return;

    // Chaining to a new combo
    const FAttackData* lastAttack{ OngoingCombo->GetAttackAt(OngoingCombo->GetMaxComboIndex()) };
    bool bIsLastAttack{ ComboIndex >= OngoingCombo->GetMaxComboIndex() };
    bool bIsLastAttackChainableWithCurrentType{ static_cast<bool>(lastAttack) && lastAttack->ChainableAttacks.Contains(attackType) };
    if (bIsLastAttack && lastAttack && bIsLastAttackChainableWithCurrentType)
    {
        FCombo* nextCombo{ GetComboFromContext(attackType) };
        if (nextCombo && nextCombo->IsValid() && !nextCombo->GetFirstAttack()->bOnCooldown)
        {
            bComboInputWindowOpen = false;
            bContinueCombo = true;
            bChainingToNewCombo = true;
            PendingCombo = nextCombo;
            return;
        }
    }

    // Continuing in the same combo
    const FAttackData* nextAttack{ OngoingCombo->GetAttackAt(ComboIndex + 1) };
    if (!nextAttack || (nextAttack && nextAttack->bOnCooldown))
        return;

    bool bIsNextAttackSameType{ nextAttack->AttackType == attackType };
    if (!bIsLastAttack && bIsNextAttackSameType)
    {
        bComboInputWindowOpen = false;
        bContinueCombo = true;
        return;
    }
}

void UFSCombatComponent::ResetComboState()
{
    // If the player is not falling or flying during a reset, that means he was ground attacking
    // so there's no need to prevent player from air attacking
    if (PlayerOwner->GetCharacterMovement()->IsFalling() || PlayerOwner->GetCharacterMovement()->IsFlying())
        bCanAirAttack = false;

    ComboIndex = 0;
    bIsAttacking = false;
    bContinueCombo = false;
    bComboInputWindowOpen = false;
    bChainingToNewCombo = false;
    OngoingCombo = nullptr;
    PendingCombo = nullptr;

    OnAttackingEnded.Broadcast();
}

////////////////////////////////////////////////
/*
* Below are all methods related to OnHitLanded
*/
////////////////////////////////////////////////
void UFSCombatComponent::HandleOnHitLanded(AActor* hitActor, const FVector& hitLocation)
{
    IFSDamageable* hitActorDamageable{ Cast<IFSDamageable>(hitActor) };
    if (!hitActor || !hitActorDamageable || (hitActorDamageable && hitActorDamageable->GetHealthComponent()->IsDead()))
        return;

    ++ComboHitCount;
    // Activate the decreasing combo time remaining in Tick()
    if (!bComboCounterActive)
        bComboCounterActive = true;

    // Render combo counter to screen
    if (ComboHitCount >= 2)
        OnComboCounterStarted.Broadcast();

    OnComboCountChanged.Broadcast(ComboHitCount);

    if (!OngoingCombo)
        return;

    const FAttackData* currentAttack{ OngoingCombo->GetAttackAt(ComboIndex) };
    if (!currentAttack)
        return;

    OngoingAttackComboWindowDuration = currentAttack->ComboWindowDuration;
    ComboTimeRemaining = OngoingAttackComboWindowDuration;

    OnHitLanded.Broadcast(hitActor, hitLocation, *currentAttack);

    HitFeedBackComponent->OnLandHit(hitLocation);

    hitActorDamageable->NotifyHitReceived(PlayerOwner, *currentAttack);
}

void UFSCombatComponent::ResetComboCounter()
{
    ComboHitCount = 0;
    bComboCounterActive = false;
    ComboTimeRemaining = 0.f;
    OnComboCounterEnded.Broadcast();
}

float UFSCombatComponent::GetComboTimeRatio() const
{
    if (OngoingAttackComboWindowDuration <= 0.f)
        return 0.f;

    return FMath::Clamp(ComboTimeRemaining / OngoingAttackComboWindowDuration, 0.f, 1.f);
}

void UFSCombatComponent::ToggleGuard()
{
    bool bInAir{ PlayerOwner->GetCharacterMovement()->IsFalling() || PlayerOwner->GetCharacterMovement()->IsFlying() };

    if (bIsAttacking || bInAir)
    {
        bGuardActivated = false;
        return;
    }

    bGuardActivated = !bGuardActivated;

    if (bGuardActivated)
        RotatePlayerTowardControlRotation();
}

void UFSCombatComponent::RotatePlayerTowardControlRotation()
{
    AController* controller{ PlayerOwner->GetController() };
    if (!controller)
        return;

    float controlYaw{ static_cast<float>(controller->GetControlRotation().Yaw) };
    PlayerOwner->SetActorRotation(FRotator(0.f, controlYaw, 0.f));
}
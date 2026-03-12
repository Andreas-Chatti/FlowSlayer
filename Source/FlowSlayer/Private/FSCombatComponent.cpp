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

    OnComboWindowOpened.AddUObject(this, &UFSCombatComponent::HandleComboWindowOpened);
    OnComboWindowClosed.AddUObject(this, &UFSCombatComponent::HandleComboWindowClosed);

    OnAirStallStarted.AddUObject(this, &UFSCombatComponent::HandleAirStallStarted);
    OnAirStallFinished.AddUObject(this, &UFSCombatComponent::HandleAirStallFinished);

    AnimInstance->OnMontageEnded.AddDynamic(this, &UFSCombatComponent::OnMontageEnded);

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

    for (auto& attack : StandingLightCombo.Attacks)
        attack.OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });

    // === STANDING HEAVY COMBO (4 attacks) ===
    StandingHeavyCombo.Attacks.SetNum(4);
    StandingHeavyCombo.Attacks[0] = *GetAttackData("StandingHeavy_0");
    StandingHeavyCombo.Attacks[1] = *GetAttackData("StandingHeavy_1");
    StandingHeavyCombo.Attacks[2] = *GetAttackData("StandingHeavy_2");
    StandingHeavyCombo.Attacks[3] = *GetAttackData("StandingHeavy_3");

    for (auto& attack : StandingHeavyCombo.Attacks)
        attack.OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });

    // === RUNNING LIGHT COMBO (3 attacks) ===
    RunningLightCombo.Attacks.SetNum(3);
    RunningLightCombo.Attacks[0] = *GetAttackData("RunningLight_0");
    RunningLightCombo.Attacks[1] = *GetAttackData("RunningLight_1");
    RunningLightCombo.Attacks[2] = *GetAttackData("RunningLight_2");

    RunningLightCombo.Attacks[2].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });

    // === RUNNING HEAVY COMBO (3 attacks) ===
    RunningHeavyCombo.Attacks.SetNum(3);
    RunningHeavyCombo.Attacks[0] = *GetAttackData("RunningHeavy_0");
    RunningHeavyCombo.Attacks[1] = *GetAttackData("RunningHeavy_1");
    RunningHeavyCombo.Attacks[2] = *GetAttackData("RunningHeavy_2");

    RunningHeavyCombo.Attacks[2].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });

    // === DASH PIERCE ===
    DashPierceAttack.Attacks.SetNum(1);
    DashPierceAttack.Attacks[0] = *GetAttackData("DashPierce");
    DashPierceAttack.Attacks[0].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });

    // === DASH SPINNING SLASH ===
    DashSpinningSlashAttack.Attacks.SetNum(1);
    DashSpinningSlashAttack.Attacks[0] = *GetAttackData("DashSpinningSlash");
    DashSpinningSlashAttack.Attacks[0].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });

    // === DASH DOUBLE SLASH ===
    DashDoubleSlashAttack.Attacks.SetNum(1);
    DashDoubleSlashAttack.Attacks[0] = *GetAttackData("DashDoubleSlash");

    // === DASH BACK SLASH ===
    DashBackSlashAttack.Attacks.SetNum(1);
    DashBackSlashAttack.Attacks[0] = *GetAttackData("DashBackSlash");

    // === JUMP SLAM ===
    JumpSlamAttack.Attacks.SetNum(1);
    JumpSlamAttack.Attacks[0] = *GetAttackData("JumpSlam");
    JumpSlamAttack.Attacks[0].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });

    // === JUMP FORWARD SLAM ===
    JumpForwardSlamAttack.Attacks.SetNum(1);
    JumpForwardSlamAttack.Attacks[0] = *GetAttackData("JumpForwardSlam");
    JumpForwardSlamAttack.Attacks[0].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });

    // === JUMP UPPER SLAM ===
    JumpUpperSlamComboAttack.Attacks.SetNum(1);
    JumpUpperSlamComboAttack.Attacks[0] = *GetAttackData("JumpUpperSlam");
    JumpUpperSlamComboAttack.Attacks[0].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });

    // === LAUNCHER ===
    LauncherAttack.Attacks.SetNum(1);
    LauncherAttack.Attacks[0] = *GetAttackData("Launcher");
    LauncherAttack.Attacks[0].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });
    LauncherAttack.Attacks[0].OnAttackHit.BindUObject(this, &UFSCombatComponent::OnLauncherAttackHit);

    // === POWER LAUNCHER ===
    PowerLauncherAttack.Attacks.SetNum(1);
    PowerLauncherAttack.Attacks[0] = *GetAttackData("PowerLauncher");
    PowerLauncherAttack.Attacks[0].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });
    PowerLauncherAttack.Attacks[0].OnAttackExecuted.BindLambda([this]()
        {
            constexpr float notifyEnd{ 0.38f };
            constexpr double zVelocity{ 1400.0 };
            FTimerHandle launchTimer;
            GetWorld()->GetTimerManager().SetTimer(
                launchTimer,
                [this]() { PlayerOwner->LaunchCharacter(FVector{ 0.0, 0.0, zVelocity }, false, true); },
                notifyEnd + 0.1f,
                false
            );
        });
    PowerLauncherAttack.Attacks[0].OnAttackHit.BindUObject(this, &UFSCombatComponent::OnLauncherAttackHit);

    // === SPIN ATTACK ===
    SpinAttack.Attacks.SetNum(1);
    SpinAttack.Attacks[0] = *GetAttackData("SpinAttack");

    TSharedPtr<FTimerHandle> spinAttackTimer{ MakeShared<FTimerHandle>() };
    SpinAttack.Attacks[0].OnAttackExecuted.BindLambda([this, spinAttackTimer]()
        {
            constexpr float spinAttackMaxWalkSpeed{ 300.f };
            constexpr float walkSpeedDelay{ 1.09f };
            PlayerOwner->GetCharacterMovement()->MaxWalkSpeed = spinAttackMaxWalkSpeed;
            constexpr float runSpeedThreshold{ 600.f };
            constexpr float sprintSpeedThreshold{ 900.f };
            GetWorld()->GetTimerManager().SetTimer(
                *spinAttackTimer,
                [this]() { PlayerOwner->GetCharacterMovement()->MaxWalkSpeed = LockedOnTarget ? runSpeedThreshold : sprintSpeedThreshold; },
                walkSpeedDelay,
                false
            );
        });

    // === HORIZONTAL SWEEP ===
    HorizontalSweepAttack.Attacks.SetNum(1);
    HorizontalSweepAttack.Attacks[0] = *GetAttackData("HorizontalSweep");
    HorizontalSweepAttack.Attacks[0].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });

    // === PIERCE THRUST ===
    PierceThrustAttack.Attacks.SetNum(1);
    PierceThrustAttack.Attacks[0] = *GetAttackData("PierceThrust");
    PierceThrustAttack.Attacks[0].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });

    // === POWER SLASH ===
    PowerSlashAttack.Attacks.SetNum(1);
    PowerSlashAttack.Attacks[0] = *GetAttackData("PowerSlash");
    PowerSlashAttack.Attacks[0].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });

    // === DIAGONAL RETOURNE ===
    DiagonalRetourneAttack.Attacks.SetNum(1);
    DiagonalRetourneAttack.Attacks[0] = *GetAttackData("DiagonalRetourne");
    DiagonalRetourneAttack.Attacks[0].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });

    // === GROUND SLAM ===
    GroundSlamAttack.Attacks.SetNum(1);
    GroundSlamAttack.Attacks[0] = *GetAttackData("GroundSlam");
    GroundSlamAttack.Attacks[0].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });

    // === AIR COMBO (3 attacks) ===
    AirCombo.Attacks.SetNum(3);
    AirCombo.Attacks[0] = *GetAttackData("AirCombo_0");
    AirCombo.Attacks[1] = *GetAttackData("AirCombo_1");
    AirCombo.Attacks[2] = *GetAttackData("AirCombo_2");

    AirCombo.Attacks[2].OnAttackExecuted.BindLambda([this]() { bCanAirAttack = false; });

    AirCombo.Attacks[0].OnAttackHit.BindUObject(this, &UFSCombatComponent::OnAirAttackHit);
    AirCombo.Attacks[1].OnAttackHit.BindUObject(this, &UFSCombatComponent::OnAirAttackHit);
    AirCombo.Attacks[2].OnAttackHit.BindUObject(this, &UFSCombatComponent::OnAirAttackHit);

    // === AERIAL SLAM ===
    AerialSlamAttack.Attacks.SetNum(1);
    AerialSlamAttack.Attacks[0] = *GetAttackData("AerialSlam");
    AerialSlamAttack.Attacks[0].OnBeforeAttack.BindLambda([this]() { RotatePlayerToPlayerView(); });
    AerialSlamAttack.Attacks[0].OnAttackExecuted.BindLambda([this]()
        {
            constexpr float slamDownVelocity{ -2000.f };
            constexpr float startDelay{ 0.51f };
            FTimerHandle slamDownTimer;
            GetWorld()->GetTimerManager().SetTimer(
                slamDownTimer,
                [slamDownVelocity, this]() { PlayerOwner->LaunchCharacter(FVector{ 0.f, 0.f, slamDownVelocity }, false, true); },
                startDelay,
                false
            );
        });
}


////////////////////////////////////////////////
/*
* 
* Core combo-related methods 
* 
*/
////////////////////////////////////////////////
void UFSCombatComponent::Attack(EAttackType attackType, bool isMoving, bool isFalling)
{
    if (bIsAttacking && !bComboWindowOpened)
        return;

    else if (bIsAttacking && bComboWindowOpened)
    {
        // Continue dans le même combo
        if ((ComboIndex <= OngoingCombo->GetMaxComboIndex()) &&
            (OngoingCombo->GetAttackAt(ComboIndex) && OngoingCombo->GetAttackAt(ComboIndex)->AttackType == attackType))
        {
            bComboWindowOpened = false;
            bContinueCombo = true;
            return;
        }

        // Chain vers un nouveau combo depuis la dernière attaque
        else if ((ComboIndex > OngoingCombo->GetMaxComboIndex()) &&
            OngoingCombo->GetLastAttack() &&
            OngoingCombo->GetLastAttack()->ChainableAttacks.Contains(attackType))
        {
            // Vérifier si le state permet cette attaque
            FCombo* nextCombo{ SelectComboBasedOnState(attackType, isMoving, isFalling) };
            if (nextCombo && nextCombo->IsValid())
            {
                bComboWindowOpened = false;
                bContinueCombo = true;
                bChainingToNewCombo = true;
                PendingCombo = nextCombo;
                return;
            }
        }
    }

    if (AnimInstance && AnimInstance->IsAnyMontagePlaying())
        return;

    bIsAttacking = true;

    OngoingCombo = SelectComboBasedOnState(attackType, isMoving, isFalling);
    if (!OngoingCombo || !OngoingCombo->IsValid())
    {
        bIsAttacking = false;
        return;
    }

    UAnimMontage* nextAnimAttack{ GetComboNextAttack(*OngoingCombo) };
    if (!nextAnimAttack)
    {
        bIsAttacking = false;
        return;
    }

    OnAttackingStarted.Broadcast();

    if (OngoingCombo->GetAttackAt(ComboIndex - 1)->OnBeforeAttack.IsBound())
        OngoingCombo->GetAttackAt(ComboIndex - 1)->OnBeforeAttack.Execute();

    PlayerOwner->PlayAnimMontage(nextAnimAttack);

    if (OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.IsBound())
        OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.Execute();
}

void UFSCombatComponent::CancelAttack(float blendOutTime)
{
    AnimInstance->StopAllMontages(blendOutTime);
    ResetComboState();
    HitboxComponent->OnActiveFrameStopped.Execute();
}

FAttackData* UFSCombatComponent::GetAttackData(FName rowName) const
{
    if (!AttackDataTable)
        return nullptr;

    return AttackDataTable->FindRow<FAttackData>(rowName, TEXT("GetAttackData"));
}

FCombo* UFSCombatComponent::SelectComboBasedOnState(EAttackType attackType, bool isMoving, bool isFalling)
{
    // Invalid attack type
    if (attackType == EAttackType::None)
        return nullptr;

    // Lookup combo from table
    FCombo** foundCombo{ ComboLookupTable.Find(attackType) };
    if (!foundCombo)
        return nullptr;

    // Validate airborne restrictions
    // Air-only attacks: JumpSlam, JumpForwardSlam, JumpUpperSlam, AirCombo, AerialSlam
    bool isAirAttack{ (
        attackType == EAttackType::JumpSlam ||
        attackType == EAttackType::JumpForwardSlam ||
        attackType == EAttackType::JumpUpperSlam ||
        attackType == EAttackType::AirCombo ||
        attackType == EAttackType::AerialSlam
    ) };

    // Ground-only attacks: all Dash, Launcher, Spin, Power, Slam attacks and JumpForwardSlam
    bool isGroundAttack{ (
        attackType == EAttackType::JumpForwardSlam ||
        attackType == EAttackType::DashPierce ||
        attackType == EAttackType::DashSpinningSlash ||
        attackType == EAttackType::DashDoubleSlash ||
        attackType == EAttackType::DashBackSlash ||
        attackType == EAttackType::Launcher ||
        attackType == EAttackType::PowerLauncher ||
        attackType == EAttackType::SpinAttack ||
        attackType == EAttackType::HorizontalSweep ||
        attackType == EAttackType::PowerSlash ||
        attackType == EAttackType::PierceThrust ||
        attackType == EAttackType::GroundSlam ||
        attackType == EAttackType::DiagonalRetourne ||
        attackType == EAttackType::StandingLight ||
        attackType == EAttackType::RunningLight ||
        attackType == EAttackType::StandingHeavy ||
        attackType == EAttackType::RunningHeavy
    ) };

    if (isAirAttack && isGroundAttack)
        return *foundCombo;

    // Reject air-only attacks on ground
    if (isAirAttack && !isFalling && !PlayerOwner->GetCharacterMovement()->IsFlying())
        return nullptr;

    // Reject ground-only attacks in air
    if (isGroundAttack && isFalling)
        return nullptr;

    return *foundCombo;
}

UAnimMontage* UFSCombatComponent::GetComboNextAttack(const FCombo& combo)
{
    UAnimMontage* attackToPerform{ nullptr };

    if (ComboIndex >= combo.GetMaxComboIndex())
        attackToPerform = combo.GetLastAttack()->Montage;
    else
        attackToPerform = combo.GetAttackAt(ComboIndex)->Montage;

    ++ComboIndex;

    return attackToPerform;
}

void UFSCombatComponent::HandleComboWindowOpened()
{
    bComboWindowOpened = true;
}

void UFSCombatComponent::HandleComboWindowClosed()
{
    bComboWindowOpened = false;

    if (!OngoingCombo)
        return;

    else if (bContinueCombo)
        ContinueCombo();
}

void UFSCombatComponent::HandleAirStallStarted()
{
    PlayerOwner->GetCharacterMovement()->GravityScale = AirStallGravity;
}

void UFSCombatComponent::HandleAirStallFinished(float gravityScale)
{
    PlayerOwner->GetCharacterMovement()->GravityScale = gravityScale;
}

void UFSCombatComponent::HandleOnLanded(const FHitResult& Hit)
{
    bCanAirAttack = true;

    if (!AnimInstance || !OngoingCombo)
        return;
    
    const UAnimMontage* currentActiveMontage{ AnimInstance->GetCurrentActiveMontage() };
    if (!currentActiveMontage)
        return;
    
    for (const auto& airAttack : AirCombo.Attacks)
    {
        if (airAttack.Montage == currentActiveMontage)
        {
            CancelAttack(0.25f);
            bCanAirAttack = true;
            break;
        }
    }
}

void UFSCombatComponent::ChainingToNextCombo()
{
    bChainingToNewCombo = false;

    if (!PendingCombo || !PendingCombo->IsValid())
        return;

    OngoingCombo = PendingCombo;
    ComboIndex = 0;
    PendingCombo = nullptr;

    UAnimMontage* firstAttack{ GetComboNextAttack(*OngoingCombo) };
    if (!firstAttack)
        return;

    if (OngoingCombo->GetAttackAt(ComboIndex - 1)->OnBeforeAttack.IsBound())
        OngoingCombo->GetAttackAt(ComboIndex - 1)->OnBeforeAttack.Execute();

    PlayerOwner->PlayAnimMontage(firstAttack);

    if (OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.IsBound())
        OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.Execute();
}

void UFSCombatComponent::RotatePlayerToPlayerView()
{
    PlayerOwner->SetActorRotation(FRotator(0.f, PlayerOwner->GetControlRotation().Yaw, 0.f), ETeleportType::TeleportPhysics);
}

void UFSCombatComponent::ContinueCombo()
{
    if (!bContinueCombo)
        return;

    bContinueCombo = false;

    if (bChainingToNewCombo)
    {
        ChainingToNextCombo();
        return;
    }

    if (!OngoingCombo || !OngoingCombo->IsValid())
        return;

    UAnimMontage* nextAnimAttack{ GetComboNextAttack(*OngoingCombo) };
    if (!nextAnimAttack)
        return;

    if (OngoingCombo->GetAttackAt(ComboIndex - 1)->OnBeforeAttack.IsBound())
        OngoingCombo->GetAttackAt(ComboIndex - 1)->OnBeforeAttack.Execute();

    PlayerOwner->PlayAnimMontage(nextAnimAttack);

    if (OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.IsBound())
        OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.Execute();
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
    bComboWindowOpened = false;
    bChainingToNewCombo = false;
    OngoingCombo = nullptr;
    PendingCombo = nullptr;

    OnAttackingEnded.Broadcast();
}

void UFSCombatComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // Safety check: ensure we have a valid ongoing combo
    if (!OngoingCombo || !OngoingCombo->IsValid())
        return;

    /** Reset the whole combo state if :
    * It's the last attack of the combo
    * If any attack in the combo is not interrupted
    */
    if (Montage == OngoingCombo->GetLastAttack()->Montage || !bInterrupted)
        ResetComboState();
}

////////////////////////////////////////////////
/*
* Attack effect on launch
* and effect on target hit
*/
////////////////////////////////////////////////
void UFSCombatComponent::OnLauncherAttackHit(AActor* hitEnemy)
{
    if (!hitEnemy)
        return;

    UCharacterMovementComponent* enemyMovement{ hitEnemy->GetComponentByClass<UCharacterMovementComponent>() };
    if (!enemyMovement)
        return;

    constexpr float maxHeight{ 400.f };
    constexpr float freezeDuraction{ 1.2f };
    FreezeEnemyAtTrajectoryPeak(hitEnemy, enemyMovement, maxHeight, freezeDuraction);
}

void UFSCombatComponent::FreezeEnemyAtTrajectoryPeak(AActor* enemy, UCharacterMovementComponent* enemyMovement, float maxHeight, float freezeDuration)
{
    constexpr float checkDelay{ 0.05f };
    float startZ{ static_cast<float>(enemy->GetActorLocation().Z) };

    // État partagé entre toutes les exécutions du timer (nécessaire car timer en loop)
    TSharedPtr<bool> bPeakDetected{ MakeShared<bool>(false) };
    TWeakObjectPtr<AActor> weakEnemy{ enemy };
    TWeakObjectPtr<UCharacterMovementComponent> weakEnemyMovement{ enemyMovement };

    FTimerHandle peakDetectionTimer;
    GetWorld()->GetTimerManager().SetTimer(
        peakDetectionTimer,
        [this, weakEnemy, weakEnemyMovement, bPeakDetected, peakDetectionTimer, startZ, maxHeight, freezeDuration]() mutable
        {
            // Early exit si déjà détecté (évite re-freeze)
            if (*bPeakDetected)
                return;

            // Vérifier si les objets existent encore
            if (!weakEnemyMovement.IsValid() || !weakEnemy.IsValid())
            {
                GetWorld()->GetTimerManager().ClearTimer(peakDetectionTimer);
                return;
            }

            float currentZ{ static_cast<float>(weakEnemy->GetActorLocation().Z) };
            float heightGained{ currentZ - startZ };
            float velocityZ{ static_cast<float>(weakEnemyMovement->Velocity.Z) };

            // Conditions de freeze : pic atteint (vitesse <= 0) OU hauteur max dépassée
            bool bReachedPeak{ velocityZ <= 0.f };
            bool bReachedMaxHeight{ heightGained >= maxHeight };

            if (bReachedPeak || bReachedMaxHeight)
            {
                *bPeakDetected = true;  // Marquer comme détecté pour toutes les futures exécutions
                GetWorld()->GetTimerManager().ClearTimer(peakDetectionTimer);

                // Freeze immédiat
                FreezeEnemyInAir(weakEnemyMovement);

                // Unfreeze après fenêtre de combo
                ScheduleEnemyUnfreeze(weakEnemyMovement, freezeDuration);
            }
        },
        checkDelay,  // Check toutes les 50ms
        true    // Loop jusqu'à détection du pic
    );
}

void UFSCombatComponent::FreezeEnemyInAir(TWeakObjectPtr<UCharacterMovementComponent> enemyMovement)
{
    if (!enemyMovement.IsValid())
        return;

    enemyMovement->SetMovementMode(EMovementMode::MOVE_Flying);
    enemyMovement->StopMovementImmediately();
    enemyMovement->GravityScale = 0.f;
}

void UFSCombatComponent::ScheduleEnemyUnfreeze(TWeakObjectPtr<UCharacterMovementComponent> enemyMovement, float delay)
{
    FTimerHandle unfreezeTimer;
    GetWorld()->GetTimerManager().SetTimer(
        unfreezeTimer,
        [enemyMovement]()
        {
            if (enemyMovement.IsValid())
            {
                enemyMovement->SetMovementMode(EMovementMode::MOVE_Falling);
                enemyMovement->GravityScale = 1.f;
            }
        },
        delay,
        false
    );
}

void UFSCombatComponent::OnAirAttackHit(AActor* hitEnemy)
{
    PlayerOwner->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
    auto* enemyMovementComp{ hitEnemy->GetComponentByClass<UCharacterMovementComponent>() };
    if (enemyMovementComp)
        enemyMovementComp->SetMovementMode(EMovementMode::MOVE_Flying);

    TWeakObjectPtr<UCharacterMovementComponent> weakEnemyMovementComp{ enemyMovementComp };

    FTimerHandle flyTimer;
    GetWorld()->GetTimerManager().SetTimer(
        flyTimer,
        [this, weakEnemyMovementComp]() 
        { 
            PlayerOwner->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);

            if (weakEnemyMovementComp.IsValid())
                weakEnemyMovementComp->SetMovementMode(EMovementMode::MOVE_Falling);
        },
        1.f,
        false
    );
}

////////////////////////////////////////////////
/*
* Below are all methods related to OnHitLanded
* VFX, SFX, Hitstop, CameraShake, etc ...
*/
////////////////////////////////////////////////
void UFSCombatComponent::HandleOnHitLanded(AActor* hitActor, const FVector& hitLocation)
{
    IFSDamageable* hitActorDamageable{ Cast<IFSDamageable>(hitActor) };
    if (!hitActor || (hitActorDamageable && hitActorDamageable->GetHealthComponent()->IsDead()))
        return;

    ++ComboHitCount;
    // Activate the decreasing combo time remaining in Tick()
    if (ComboHitCount >= 1)
        bComboCounterActive = true;

    // Render combo counter to screen
    if (ComboHitCount >= 2)
        OnComboCounterStarted.Broadcast();

    OnComboCountChanged.Broadcast(ComboHitCount);
    
    // NOTE: ComboIndex has already been incremented at this point, 
    // so the actual ComboIndex to this attack that hit the enemy here is : ComboIndex - 1
    const FAttackData* currentAttack{ OngoingCombo->GetAttackAt(ComboIndex - 1) };
    if (!currentAttack)
        return;

    OngoingAttackComboWindowDuration = currentAttack->ComboWindowDuration;
    ComboTimeRemaining = OngoingAttackComboWindowDuration;

    OnHitLanded.Broadcast(hitActor, hitLocation, *currentAttack);

    if (currentAttack->OnAttackHit.IsBound())
        currentAttack->OnAttackHit.Execute(hitActor);

    HitFeedBackComponent->OnLandHit(hitLocation);

    hitActorDamageable->NotifyHitReceived(PlayerOwner, *currentAttack);
}

AActor* UFSCombatComponent::GetNearestEnemyFromPlayer(float distanceRadius, bool debugLines) const
{
    FVector Start{ PlayerOwner->GetActorLocation() };
    FVector End{ Start };

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(GetOwner());

    TArray<FHitResult> outHits;
    bool bHit{ UKismetSystemLibrary::SphereTraceMultiForObjects(
        GetWorld(),
        Start,
        End,
        distanceRadius,
        ObjectTypes,
        false,
        ActorsToIgnore,
        debugLines ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
        outHits,
        true
    ) };

    if (!bHit)
        return nullptr;

    TSet<AActor*> uniqueHitActors;
    float shortestDistance{ FLT_MAX };
    AActor* nearestEnemy{ nullptr };
    for (const auto& hit : outHits)
    {
        AActor* hitActor{ hit.GetActor() };
        if (uniqueHitActors.Contains(hitActor))
            continue;

        uniqueHitActors.Add(hitActor);

        if (hitActor->Implements<UFSDamageable>() && !Cast<IFSDamageable>(hitActor)->GetHealthComponent()->IsDead())
        {
            float newDistance{ static_cast<float>(FVector::Distance(hitActor->GetActorLocation(), PlayerOwner->GetActorLocation())) };
            if (newDistance < shortestDistance)
            {
                shortestDistance = newDistance;
                nearestEnemy = hitActor;
            }
        }
    }

    return nearestEnemy;
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
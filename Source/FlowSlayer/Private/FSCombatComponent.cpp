#include "FSCombatComponent.h"

UFSCombatComponent::UFSCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UFSCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    // Setting up core variables (PlayerOwner, AnimInstance and equippedWeapon)
    PlayerOwner = Cast<ACharacter>(GetOwner());
    AnimInstance = PlayerOwner->GetMesh()->GetAnimInstance();
    InitializeAndAttachWeapon();

    checkf(PlayerOwner && AnimInstance && equippedWeapon, TEXT("FATAL: One or more Core CombatComponent variables are NULL"));

    equippedWeapon->OnEnemyHit.AddUObject(this, &UFSCombatComponent::OnHitLanded);

    // Bind combo window delegates (broadcasted by AnimNotifyState_ModularCombo)
    OnComboWindowOpened.AddUObject(this, &UFSCombatComponent::HandleComboWindowOpened);
    OnComboWindowClosed.AddUObject(this, &UFSCombatComponent::HandleComboWindowClosed);

    // Bind Air stall delegates for air combos (broadcasted by AirStallNotify)
    OnAirStallStarted.AddUObject(this, &UFSCombatComponent::HandleAirStallStarted);
    OnAirStallFinished.AddUObject(this, &UFSCombatComponent::HandleAirStallFinished);

    AnimInstance->OnMontageEnded.AddDynamic(this, &UFSCombatComponent::OnMontageEnded);

    // Initialize combo attack data (damage, knockback, ChainableAttacks)
    // Must be called AFTER Blueprint has loaded montages
    InitializeComboAttackData();

    // Initialize combo lookup table for fast attack selection
    InitializeComboLookupTable();
}

void UFSCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsLockedOnEngaged && CurrentLockedOnTarget)
        UpdateLockOnCamera(DeltaTime);
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
    // === STANDING LIGHT COMBO (3 attacks) ===
    StandingLightCombo.Attacks.SetNum(3);

    // Attack 1
    StandingLightCombo.Attacks[0].Damage = 50.f;
    StandingLightCombo.Attacks[0].KnockbackForce = 100.f;
    StandingLightCombo.Attacks[0].AttackType = EAttackType::StandingLight;

    // Attack 2
    StandingLightCombo.Attacks[1].Damage = 55.f;
    StandingLightCombo.Attacks[1].KnockbackForce = 120.f;
    StandingLightCombo.Attacks[1].AttackType = EAttackType::StandingLight;

    // Attack 3 (final) - Only this one has ChainableAttacks
    StandingLightCombo.Attacks[2].Damage = 60.f;
    StandingLightCombo.Attacks[2].KnockbackForce = 150.f;
    StandingLightCombo.Attacks[2].AttackType = EAttackType::StandingLight;
    StandingLightCombo.Attacks[2].ChainableAttacks = {
        EAttackType::StandingLight,
        EAttackType::StandingHeavy,
        EAttackType::RunningLight,
        EAttackType::RunningHeavy,
        EAttackType::Launcher,
        EAttackType::PowerLauncher,
        EAttackType::SpinAttack,
        EAttackType::DashPierce
    };

    // === STANDING HEAVY COMBO (4 attacks) ===
    StandingHeavyCombo.Attacks.SetNum(4);

    // Attack 1
    StandingHeavyCombo.Attacks[0].Damage = 70.f;
    StandingHeavyCombo.Attacks[0].KnockbackForce = 250.f;
    StandingHeavyCombo.Attacks[0].AttackType = EAttackType::StandingHeavy;

    // Attack 2
    StandingHeavyCombo.Attacks[1].Damage = 75.f;
    StandingHeavyCombo.Attacks[1].KnockbackForce = 280.f;
    StandingHeavyCombo.Attacks[1].AttackType = EAttackType::StandingHeavy;

    // Attack 3
    StandingHeavyCombo.Attacks[2].Damage = 80.f;
    StandingHeavyCombo.Attacks[2].KnockbackForce = 320.f;
    StandingHeavyCombo.Attacks[2].AttackType = EAttackType::StandingHeavy;

    // Attack 4 (final) - Only this one has ChainableAttacks
    StandingHeavyCombo.Attacks[3].Damage = 90.f;
    StandingHeavyCombo.Attacks[3].KnockbackForce = 400.f;
    StandingHeavyCombo.Attacks[3].AttackType = EAttackType::StandingHeavy;
    StandingHeavyCombo.Attacks[3].ChainableAttacks = {
        EAttackType::StandingHeavy,
        EAttackType::StandingLight,
        EAttackType::RunningHeavy,
        EAttackType::RunningLight,
        EAttackType::GroundSlam,
        EAttackType::PowerSlash,
        EAttackType::Launcher,
        EAttackType::PowerLauncher
    };

    // === RUNNING LIGHT COMBO (7 attacks) ===
    RunningLightCombo.Attacks.SetNum(7);
    {
        // Attack 1
        RunningLightCombo.Attacks[0].Damage = 50.f;
        RunningLightCombo.Attacks[0].KnockbackForce = 120.f;
        RunningLightCombo.Attacks[0].AttackType = EAttackType::RunningLight;

        // Attack 2
        RunningLightCombo.Attacks[1].Damage = 52.f;
        RunningLightCombo.Attacks[1].KnockbackForce = 130.f;
        RunningLightCombo.Attacks[1].AttackType = EAttackType::RunningLight;

        // Attack 3
        RunningLightCombo.Attacks[2].Damage = 55.f;
        RunningLightCombo.Attacks[2].KnockbackForce = 140.f;
        RunningLightCombo.Attacks[2].AttackType = EAttackType::RunningLight;

        // Attack 4
        RunningLightCombo.Attacks[3].Damage = 58.f;
        RunningLightCombo.Attacks[3].KnockbackForce = 150.f;
        RunningLightCombo.Attacks[3].AttackType = EAttackType::RunningLight;

        // Attack 5
        RunningLightCombo.Attacks[4].Damage = 60.f;
        RunningLightCombo.Attacks[4].KnockbackForce = 160.f;
        RunningLightCombo.Attacks[4].AttackType = EAttackType::RunningLight;

        // Attack 6
        RunningLightCombo.Attacks[5].Damage = 65.f;
        RunningLightCombo.Attacks[5].KnockbackForce = 180.f;
        RunningLightCombo.Attacks[5].AttackType = EAttackType::RunningLight;

        // Attack 7 (final) - Only this one has ChainableAttacks
        RunningLightCombo.Attacks[6].Damage = 70.f;
        RunningLightCombo.Attacks[6].KnockbackForce = 200.f;
        RunningLightCombo.Attacks[6].AttackType = EAttackType::RunningLight;
        RunningLightCombo.Attacks[6].ChainableAttacks = {
            EAttackType::RunningLight,
            EAttackType::RunningHeavy,
            EAttackType::StandingHeavy,
            EAttackType::Launcher,
            EAttackType::PowerLauncher,
            EAttackType::PowerSlash,
            EAttackType::PierceThrust,
            EAttackType::DashPierce,
            EAttackType::DashDoubleSlash
        };
    }

    // === RUNNING HEAVY COMBO (4 attacks) ===
    RunningHeavyCombo.Attacks.SetNum(4);
    {
        // Attack 1
        RunningHeavyCombo.Attacks[0].Damage = 75.f;
        RunningHeavyCombo.Attacks[0].KnockbackForce = 300.f;
        RunningHeavyCombo.Attacks[0].AttackType = EAttackType::RunningHeavy;

        // Attack 2
        RunningHeavyCombo.Attacks[1].Damage = 80.f;
        RunningHeavyCombo.Attacks[1].KnockbackForce = 320.f;
        RunningHeavyCombo.Attacks[1].AttackType = EAttackType::RunningHeavy;

        // Attack 3
        RunningHeavyCombo.Attacks[2].Damage = 85.f;
        RunningHeavyCombo.Attacks[2].KnockbackForce = 350.f;
        RunningHeavyCombo.Attacks[2].AttackType = EAttackType::RunningHeavy;

        // Attack 4 (final) - Only this one has ChainableAttacks
        RunningHeavyCombo.Attacks[3].Damage = 95.f;
        RunningHeavyCombo.Attacks[3].KnockbackForce = 420.f;
        RunningHeavyCombo.Attacks[3].AttackType = EAttackType::RunningHeavy;
        RunningHeavyCombo.Attacks[3].ChainableAttacks = {
            EAttackType::RunningHeavy,
            EAttackType::StandingHeavy,
            EAttackType::RunningLight,
            EAttackType::GroundSlam,
            EAttackType::PowerSlash,
            EAttackType::Launcher,
            EAttackType::PowerLauncher,
            EAttackType::DashDoubleSlash
        };
    }

    // === DASH PIERCE ===
    DashPierceAttack.Attacks.SetNum(1);
    {
        DashPierceAttack.Attacks[0].Damage = 70.f;
        DashPierceAttack.Attacks[0].KnockbackForce = 200.f;
        DashPierceAttack.Attacks[0].AttackType = EAttackType::DashPierce;
        DashPierceAttack.Attacks[0].ChainableAttacks = {
            EAttackType::RunningLight,
            EAttackType::StandingLight,
            EAttackType::Launcher,
            EAttackType::PowerLauncher,
            EAttackType::PierceThrust,
            EAttackType::SpinAttack
        };
    }

    // === DASH SPINNING SLASH ===
    DashSpinningSlashAttack.Attacks.SetNum(1);
    {
        DashSpinningSlashAttack.Attacks[0].Damage = 75.f;
        DashSpinningSlashAttack.Attacks[0].KnockbackForce = 250.f;
        DashSpinningSlashAttack.Attacks[0].AttackType = EAttackType::DashSpinningSlash;
        DashSpinningSlashAttack.Attacks[0].ChainableAttacks = {
            EAttackType::RunningLight,
            EAttackType::RunningHeavy,
            EAttackType::StandingLight,
            EAttackType::StandingHeavy,
            EAttackType::HorizontalSweep,
            EAttackType::SpinAttack,
            EAttackType::PowerSlash,
            EAttackType::Launcher
        };
    }

    // === DASH DOUBLE SLASH ===
    DashDoubleSlashAttack.Attacks.SetNum(1);
    {
        DashDoubleSlashAttack.Attacks[0].Damage = 90.f;
        DashDoubleSlashAttack.Attacks[0].KnockbackForce = 300.f;
        DashDoubleSlashAttack.Attacks[0].AttackType = EAttackType::DashDoubleSlash;
        DashDoubleSlashAttack.Attacks[0].ChainableAttacks = {
            EAttackType::RunningHeavy,
            EAttackType::PowerSlash,
            EAttackType::GroundSlam,
            EAttackType::Launcher
        };
    }

    // === DASH BACK SLASH ===
    DashBackSlashAttack.Attacks.SetNum(1);
    {
        DashBackSlashAttack.Attacks[0].Damage = 80.f;
        DashBackSlashAttack.Attacks[0].KnockbackForce = 400.f;
        DashBackSlashAttack.Attacks[0].AttackType = EAttackType::DashBackSlash;
        DashBackSlashAttack.Attacks[0].ChainableAttacks = {
            EAttackType::StandingLight,
            EAttackType::StandingHeavy,
            EAttackType::DiagonalRetourne,
            EAttackType::JumpSlam,
            EAttackType::JumpForwardSlam,
            EAttackType::SpinAttack
        };
    }

    // === JUMP SLAM ===
    JumpSlamAttack.Attacks.SetNum(1);
    {
        JumpSlamAttack.Attacks[0].Damage = 100.f;
        JumpSlamAttack.Attacks[0].KnockbackForce = 400.f;
        JumpSlamAttack.Attacks[0].KnockbackUpForce = 200.f;
        JumpSlamAttack.Attacks[0].AttackType = EAttackType::JumpSlam;
        JumpSlamAttack.Attacks[0].ChainableAttacks = {
            EAttackType::StandingLight,
            EAttackType::RunningLight,
            EAttackType::StandingHeavy,
            EAttackType::SpinAttack,
            EAttackType::Launcher
        };
    }

    // === JUMP FORWARD SLAM ===
    JumpForwardSlamAttack.Attacks.SetNum(1);
    {
        JumpForwardSlamAttack.Attacks[0].Damage = 105.f;
        JumpForwardSlamAttack.Attacks[0].KnockbackForce = 450.f;
        JumpForwardSlamAttack.Attacks[0].KnockbackUpForce = 250.f;
        JumpForwardSlamAttack.Attacks[0].AttackType = EAttackType::JumpForwardSlam;
        JumpForwardSlamAttack.Attacks[0].ChainableAttacks = {
            EAttackType::RunningLight,
            EAttackType::RunningHeavy,
            EAttackType::DashPierce,
            EAttackType::Launcher,
            EAttackType::PowerSlash
        };
    }

    // === JUMP UPPER SLAM ===
    JumpUpperSlamComboAttack.Attacks.SetNum(1);
    {
        JumpUpperSlamComboAttack.Attacks[0].Damage = 110.f;
        JumpUpperSlamComboAttack.Attacks[0].KnockbackForce = 500.f;
        JumpUpperSlamComboAttack.Attacks[0].KnockbackUpForce = 300.f;
        JumpUpperSlamComboAttack.Attacks[0].AttackType = EAttackType::JumpUpperSlam;
        JumpUpperSlamComboAttack.Attacks[0].ChainableAttacks = {
            EAttackType::StandingLight,
            EAttackType::StandingHeavy,
            EAttackType::RunningLight,
            EAttackType::Launcher,
            EAttackType::GroundSlam,
            EAttackType::SpinAttack
        };
    }

    // === LAUNCHER ===
    LauncherAttack.Attacks.SetNum(1);
    {
        LauncherAttack.Attacks[0].Damage = 60.f;
        LauncherAttack.Attacks[0].KnockbackForce = 200.f;
        LauncherAttack.Attacks[0].KnockbackUpForce = 800.f;
        LauncherAttack.Attacks[0].AttackType = EAttackType::Launcher;
        LauncherAttack.Attacks[0].ChainableAttacks = {
            EAttackType::AirCombo,
            EAttackType::AerialSlam
        };
        LauncherAttack.Attacks[0].OnAttackExecuted.BindUObject(this, &UFSCombatComponent::OnLauncherAttackExecuted);
        LauncherAttack.Attacks[0].OnAttackHit.BindUObject(this, &UFSCombatComponent::OnLauncherAttackHit);
    }

    // === POWER LAUNCHER ===
    PowerLauncherAttack.Attacks.SetNum(1);
    {
        PowerLauncherAttack.Attacks[0].Damage = 80.f;
        PowerLauncherAttack.Attacks[0].KnockbackForce = 250.f;
        PowerLauncherAttack.Attacks[0].KnockbackUpForce = 1000.f;
        PowerLauncherAttack.Attacks[0].AttackType = EAttackType::PowerLauncher;
        PowerLauncherAttack.Attacks[0].ChainableAttacks = {
            EAttackType::AirCombo,
            EAttackType::AerialSlam
        };
    }

    // === SPIN ATTACK ===
    SpinAttack.Attacks.SetNum(1);
    {
        SpinAttack.Attacks[0].Damage = 65.f;
        SpinAttack.Attacks[0].KnockbackForce = 200.f;
        SpinAttack.Attacks[0].AttackType = EAttackType::SpinAttack;
        SpinAttack.Attacks[0].ChainableAttacks = {
            EAttackType::StandingLight,
            EAttackType::StandingHeavy,
            EAttackType::HorizontalSweep,
            EAttackType::GroundSlam,
            EAttackType::PowerSlash,
            EAttackType::Launcher
        };
    }

    // === HORIZONTAL SWEEP ===
    HorizontalSweepAttack.Attacks.SetNum(1);
    {
        HorizontalSweepAttack.Attacks[0].Damage = 75.f;
        HorizontalSweepAttack.Attacks[0].KnockbackForce = 250.f;
        HorizontalSweepAttack.Attacks[0].AttackType = EAttackType::HorizontalSweep;
        HorizontalSweepAttack.Attacks[0].ChainableAttacks = {
            EAttackType::StandingHeavy,
            EAttackType::RunningHeavy,
            EAttackType::GroundSlam,
            EAttackType::SpinAttack,
            EAttackType::Launcher,
            EAttackType::PowerSlash
        };
    }

    // === PIERCE THRUST ===
    PierceThrustAttack.Attacks.SetNum(1);
    {
        PierceThrustAttack.Attacks[0].Damage = 90.f;
        PierceThrustAttack.Attacks[0].KnockbackForce = 350.f;
        PierceThrustAttack.Attacks[0].AttackType = EAttackType::PierceThrust;
        PierceThrustAttack.Attacks[0].ChainableAttacks = {
            EAttackType::StandingLight,
            EAttackType::RunningLight,
            EAttackType::PowerSlash,
            EAttackType::DashPierce,
            EAttackType::Launcher,
            EAttackType::SpinAttack
        };
    }

    // === POWER SLASH ===
    PowerSlashAttack.Attacks.SetNum(1);
    {
        PowerSlashAttack.Attacks[0].Damage = 120.f;
        PowerSlashAttack.Attacks[0].KnockbackForce = 500.f;
        PowerSlashAttack.Attacks[0].AttackType = EAttackType::PowerSlash;
        PowerSlashAttack.Attacks[0].ChainableAttacks = {
            EAttackType::RunningHeavy,
            EAttackType::GroundSlam,
            EAttackType::Launcher,
            EAttackType::DiagonalRetourne
        };
    }

    // === DIAGONAL RETOURNE ===
    DiagonalRetourneAttack.Attacks.SetNum(1);
    {
        DiagonalRetourneAttack.Attacks[0].Damage = 85.f;
        DiagonalRetourneAttack.Attacks[0].KnockbackForce = 400.f;
        DiagonalRetourneAttack.Attacks[0].AttackType = EAttackType::DiagonalRetourne;
        DiagonalRetourneAttack.Attacks[0].ChainableAttacks = {
            EAttackType::StandingLight,
            EAttackType::StandingHeavy,
            EAttackType::RunningHeavy,
            EAttackType::DashBackSlash,
            EAttackType::SpinAttack,
            EAttackType::HorizontalSweep,
            EAttackType::JumpSlam
        };
    }

    // === GROUND SLAM ===
    GroundSlamAttack.Attacks.SetNum(1);
    {
        GroundSlamAttack.Attacks[0].Damage = 130.f;
        GroundSlamAttack.Attacks[0].KnockbackForce = 600.f;
        GroundSlamAttack.Attacks[0].KnockbackUpForce = 300.f;
        GroundSlamAttack.Attacks[0].AttackType = EAttackType::GroundSlam;
        GroundSlamAttack.Attacks[0].ChainableAttacks = {
            EAttackType::StandingHeavy,
            EAttackType::Launcher,
            EAttackType::PowerSlash
        };
    }

    // === AIR COMBO (3 attacks) ===
    AirCombo.Attacks.SetNum(3);
    {
        // Attack 1
        AirCombo.Attacks[0].Damage = 50.f;
        AirCombo.Attacks[0].KnockbackForce = 80.f;
        AirCombo.Attacks[0].AttackType = EAttackType::AirCombo;
        AirCombo.Attacks[0].OnAttackExecuted.BindLambda([this]() {OnAirAttackExecuted(0.15f, 0.34f);});

        // Attack 2
        AirCombo.Attacks[1].Damage = 55.f;
        AirCombo.Attacks[1].KnockbackForce = 100.f;
        AirCombo.Attacks[1].AttackType = EAttackType::AirCombo;
        AirCombo.Attacks[0].OnAttackExecuted.BindLambda([this]() {OnAirAttackExecuted(0.33f, 0.52f);});

        // Attack 3 (final) - Only this one has ChainableAttacks
        AirCombo.Attacks[2].Damage = 60.f;
        AirCombo.Attacks[2].KnockbackForce = 120.f;
        AirCombo.Attacks[2].AttackType = EAttackType::AirCombo;
        AirCombo.Attacks[2].ChainableAttacks = { EAttackType::AerialSlam };
        AirCombo.Attacks[0].OnAttackExecuted.BindLambda([this]() {OnAirAttackExecuted(0.14f, 0.28f);});

        for (auto& attack : AirCombo.Attacks)
            attack.OnAttackHit.BindUObject(this, &UFSCombatComponent::OnAirAttackHit);
    }

    // === AERIAL SLAM ===
    AerialSlamAttack.Attacks.SetNum(1);
    {
        AerialSlamAttack.Attacks[0].Damage = 110.f;
        AerialSlamAttack.Attacks[0].KnockbackForce = 500.f;
        AerialSlamAttack.Attacks[0].KnockbackUpForce = 200.f;
        AerialSlamAttack.Attacks[0].AttackType = EAttackType::AerialSlam;
        AerialSlamAttack.Attacks[0].ChainableAttacks = {
            EAttackType::StandingLight,
            EAttackType::RunningLight,
            EAttackType::StandingHeavy,
            EAttackType::Launcher,
            EAttackType::GroundSlam,
            EAttackType::SpinAttack
        };
    }
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

    PlayerOwner->PlayAnimMontage(nextAnimAttack);

    if (OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.IsBound())
        OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.Execute();
}

void UFSCombatComponent::CancelAttack()
{
    AnimInstance->StopAllMontages(0.2f);
    StopCombo();
    equippedWeapon->DeactivateHitbox();
}

FCombo* UFSCombatComponent::SelectComboBasedOnState(EAttackType attackType, bool isMoving, bool isFalling)
{
    // Invalid attack type
    if (attackType == EAttackType::None)
        return nullptr;

    // Lookup combo from table
    FCombo** foundCombo = ComboLookupTable.Find(attackType);
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

void UFSCombatComponent::ContinueCombo()
{
    if (!bContinueCombo)
        return;

    bContinueCombo = false;

    // Si on chain vers un nouveau combo
    if (bChainingToNewCombo)
    {
        bChainingToNewCombo = false;
        if (!PendingCombo || !PendingCombo->IsValid())
            return;

        // Démarrer le nouveau combo
        OngoingCombo = PendingCombo;
        ComboIndex = 0;
        PendingCombo = nullptr;

        // Utiliser GetComboNextAttack pour récupérer le montage ET incrémenter ComboIndex
        UAnimMontage* firstAttack{ GetComboNextAttack(*OngoingCombo) };
        if (!firstAttack)
            return;

        PlayerOwner->PlayAnimMontage(firstAttack);

        if (OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.IsBound())
            OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.Execute();

        return;
    }

    // Continuer dans le combo actuel
    if (!OngoingCombo || !OngoingCombo->IsValid())
        return;

    UAnimMontage* nextAnimAttack{ GetComboNextAttack(*OngoingCombo) };
    if (!nextAnimAttack)
        return;

    PlayerOwner->PlayAnimMontage(nextAnimAttack);

    if (OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.IsBound())
        OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.Execute();
}

void UFSCombatComponent::StopCombo()
{
    ComboIndex = 0;
    bIsAttacking = false;
    bContinueCombo = false;
    bComboWindowOpened = false;
    bChainingToNewCombo = false;
    OngoingCombo = nullptr;
    PendingCombo = nullptr;
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
        StopCombo();
}

////////////////////////////////////////////////
/*
* Attack effect on launch
* and effect on target hit
*/
////////////////////////////////////////////////
  void UFSCombatComponent::OnAirAttackExecuted(float start, float end)
  {
      float distanceRadius{ 250.f };
      AActor* nearestEnemy{ GetNearestEnemyFromPlayer(distanceRadius) };

      UMotionWarpingComponent* MotionWarpingComp{ PlayerOwner->FindComponentByClass<UMotionWarpingComponent>() };
      if (!MotionWarpingComp || !nearestEnemy)
          return;

      FVector playerLocation{ PlayerOwner->GetActorLocation() };
      FVector enemyLocation{ nearestEnemy->GetActorLocation() };
      FRotator lookAtRotation{ UKismetMathLibrary::FindLookAtRotation(playerLocation, enemyLocation) };

      FMotionWarpingTarget target;
      target.Name = FName("AttackTarget");
      target.Location = enemyLocation;
      target.Rotation = lookAtRotation;

      MotionWarpingComp->AddOrUpdateWarpTarget(target);

      FTimerHandle flyingStartTimer;
      GetWorld()->GetTimerManager().SetTimer(
          flyingStartTimer,
          [this]()
          {
              PlayerOwner->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
          },
          start,
          false
      );

      FTimerHandle flyingEndTimer;
      GetWorld()->GetTimerManager().SetTimer(
          flyingEndTimer,
          [this, MotionWarpingComp]()
          {
              PlayerOwner->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
              MotionWarpingComp->RemoveAllWarpTargets();
          },
          end,
          false
      );
  }

void UFSCombatComponent::OnAirAttackHit(AActor* hitEnemy)
{
}

void UFSCombatComponent::OnLauncherAttackExecuted()
{
    float distanceRadius{ 250.f };
    AActor* nearestEnemy{ GetNearestEnemyFromPlayer(distanceRadius) };

    UMotionWarpingComponent* MotionWarpingComp{ PlayerOwner->FindComponentByClass<UMotionWarpingComponent>() };
    if (!MotionWarpingComp || !nearestEnemy)
        return;

    FVector targetLocation{ nearestEnemy->GetActorLocation() };
    targetLocation.Z += 150.f;

    //FVector forwardOffset{ PlayerOwner->GetActorForwardVector() * 100.f };
    //targetLocation += forwardOffset;

    FVector playerLocation{ PlayerOwner->GetActorLocation() };
    FVector enemyLocation{ nearestEnemy->GetActorLocation() };
    FRotator lookAtRotation{ UKismetMathLibrary::FindLookAtRotation(playerLocation, enemyLocation) };

    FMotionWarpingTarget target;
    target.Name = FName("AttackTarget");
    target.Location = targetLocation;
    target.Rotation = lookAtRotation;

    MotionWarpingComp->AddOrUpdateWarpTarget(target);

    FTimerHandle flyingStartTimer;
    GetWorld()->GetTimerManager().SetTimer(
        flyingStartTimer,
        [this]()
        {
            PlayerOwner->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
        },
        0.40f,
        false
    );

    FTimerHandle flyingEndTimer;
    GetWorld()->GetTimerManager().SetTimer(
        flyingEndTimer,
        [this, MotionWarpingComp]()
        {
            PlayerOwner->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
            MotionWarpingComp->RemoveAllWarpTargets();
        },
        0.82f,
        false
    );
}

void UFSCombatComponent::OnLauncherAttackHit(AActor* hitEnemy)
{
    if (!hitEnemy)
        return;

    // === DÉTECTION HAUTEUR MAX OU PEAK ===
    UCharacterMovementComponent* enemyMovement{ hitEnemy->GetComponentByClass<UCharacterMovementComponent>() };
    if (!enemyMovement)
        return;

    float startZ{ static_cast<float>(hitEnemy->GetActorLocation().Z) };
    const float MAX_HEIGHT_OFFSET{ 400.f }; // Hauteur max au-dessus de la position initiale

    TSharedPtr<bool> bPeakDetected = MakeShared<bool>(false);

    FTimerHandle peakDetectionTimer;
    TWeakObjectPtr<AActor> weakEnemy{ hitEnemy };
    TWeakObjectPtr<UCharacterMovementComponent> weakEnemyMovement{ enemyMovement };

    GetWorld()->GetTimerManager().SetTimer(
        peakDetectionTimer,
        [this, weakEnemy, weakEnemyMovement, bPeakDetected, peakDetectionTimer, startZ, MAX_HEIGHT_OFFSET]() mutable
        {
            if (*bPeakDetected)
                return;

            if (!weakEnemyMovement.IsValid() || !weakEnemy.IsValid())
            {
                GetWorld()->GetTimerManager().ClearTimer(peakDetectionTimer);
                return;
            }

            float currentZ{ static_cast<float>(weakEnemy->GetActorLocation().Z) };
            float heightGained{ currentZ - startZ };
            float currentVelocityZ{ static_cast<float>(weakEnemyMovement->Velocity.Z) };

            // FREEZE si : Peak atteint OU hauteur max dépassée
            bool bShouldFreeze{ currentVelocityZ <= 0.f || heightGained >= MAX_HEIGHT_OFFSET };

            if (bShouldFreeze)
            {
                *bPeakDetected = true;
                GetWorld()->GetTimerManager().ClearTimer(peakDetectionTimer);

                UE_LOG(LogTemp, Warning, TEXT("Freezing enemy at Z = %f (gained %f units)"),
                    currentZ, heightGained);

                // FREEZE L'ENNEMI AU PEAK
                weakEnemyMovement->SetMovementMode(EMovementMode::MOVE_Flying);
                weakEnemyMovement->StopMovementImmediately();
                weakEnemyMovement->GravityScale = 0.f;

                // UNFREEZE APRÈS LA FENÊTRE DE COMBO AÉRIEN
                FTimerHandle unfreezeTimer;
                GetWorld()->GetTimerManager().SetTimer(
                    unfreezeTimer,
                    [weakEnemyMovement]()
                    {
                        if (weakEnemyMovement.IsValid())
                        {
                            weakEnemyMovement->SetMovementMode(EMovementMode::MOVE_Falling);
                            weakEnemyMovement->GravityScale = 1.f;
                            UE_LOG(LogTemp, Warning, TEXT("Enemy released from air freeze"));
                        }
                    },
                    1.2f, // Durée de la fenêtre de combo aérien
                    false
                );
            }
        },
        0.05f, // Check toutes les 50ms
        true   // Loop jusqu'au freeze
    );
}

////////////////////////////////////////////////
/*
* Below are all methods related to OnHitLanded
* VFX, SFX, Hitstop, CameraShake, etc ...
*/
////////////////////////////////////////////////
void UFSCombatComponent::OnHitLanded(AActor* hitActor, const FVector& hitLocation)
{
    if (!hitActor)
        return;

    // NOTE: ComboIndex has already been incremented at this point, 
    // so the actual ComboIndex to this attack that hit the enemy here is : ComboIndex - 1
    const FAttackData* currentAttack{ OngoingCombo->GetAttackAt(ComboIndex - 1) };
    if (!currentAttack)
        return;

    if (currentAttack->OnAttackHit.IsBound())
        currentAttack->OnAttackHit.Execute(hitActor);

    ApplyDamage(hitActor, equippedWeapon, currentAttack->Damage);
    ApplyKnockback(hitActor, currentAttack->KnockbackForce, currentAttack->KnockbackUpForce);
    ApplyHitstop();
    SpawnHitVFX(hitLocation);
    PlayHitSound(hitLocation);
    ApplyCameraShake();
    ApplyHitFlash(hitActor);
}

void UFSCombatComponent::ApplyDamage(AActor* target, AActor* instigator, float damageAmount)
{
    IFSDamageable* damageableActor{ Cast<IFSDamageable>(target) };
    if (damageableActor)
        damageableActor->ReceiveDamage(damageAmount, instigator);
}

void UFSCombatComponent::ApplyKnockback(AActor* target, float KnockbackForce, float UpKnockbackForce)
{
    ACharacter* HitCharacter{ Cast<ACharacter>(target) };
    if (!HitCharacter)
        return;

    FVector PlayerLocation{ PlayerOwner->GetActorLocation() };
    FVector EnemyLocation{ target->GetActorLocation() };
    FVector KnockbackDirection{ (EnemyLocation - PlayerLocation).GetSafeNormal() };
    FVector KnockbackVelocity{ KnockbackDirection * KnockbackForce + FVector(0.f, 0.f, UpKnockbackForce) };

    HitCharacter->LaunchCharacter(KnockbackVelocity, true, true);
}

void UFSCombatComponent::ApplyHitstop()
{
    // Ralentit le temps globalement
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), hitstopTimeDilation);

    // Reset après la durée
    GetWorld()->GetTimerManager().SetTimer(
        hitstopTimerHandle,
        this,
        &UFSCombatComponent::ResetTimeDilation,
        hitstopDuration * hitstopTimeDilation, // Ajusté pour le slow-mo
        false
    );
}

void UFSCombatComponent::ResetTimeDilation()
{
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
}

void UFSCombatComponent::SpawnHitVFX(const FVector& location)
{
    if (hitParticlesSystemArray.IsEmpty())
        return;

    uint32 randIndex{ static_cast<uint32>(FMath::RandRange(0, hitParticlesSystemArray.Num() - 1)) };
    if (!hitParticlesSystemArray.IsValidIndex(randIndex))
        return;

    UNiagaraSystem* hitParticulesSystem{ hitParticlesSystemArray[randIndex] };
    if (hitParticulesSystem)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            hitParticulesSystem,
            location,
            FRotator::ZeroRotator,
            FVector(1.0f),
            true,
            true,
            ENCPoolMethod::None
        );
    }
}

void UFSCombatComponent::PlayHitSound(const FVector& location)
{
    if (hitSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            GetWorld(),
            hitSound,
            location
        );
    }
}

void UFSCombatComponent::ApplyCameraShake()
{
    if (hitCameraShake)
    {
        APlayerController* pc{ UGameplayStatics::GetPlayerController(GetWorld(), 0) };
        if (pc)
            pc->ClientStartCameraShake(hitCameraShake);
    }
}

void UFSCombatComponent::ApplyHitFlash(AActor* hitActor)
{
    if (!hitActor)
        return;

    USkeletalMeshComponent* enemyMesh{ hitActor->FindComponentByClass<USkeletalMeshComponent>() };
    if (!enemyMesh)
        return;

    TArray<UMaterialInterface*> ogMats;
    for (int32 i{}; i < enemyMesh->GetNumMaterials(); i++)
    {
        ogMats.Add(enemyMesh->GetMaterials()[i]);
        enemyMesh->SetMaterial(i, HitFlashMaterial);
    }

    FTimerHandle flashTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        flashTimerHandle,
        [enemyMesh, ogMats]()
        {
            for (int32 i{}; i < enemyMesh->GetNumMaterials(); i++)
                if (ogMats.IsValidIndex(i))
                    enemyMesh->SetMaterial(i, ogMats[i]);
        },
        hitFlashDuration,
        false
    );
}

////////////////////////////////////////////////
/*
* 
* Lock-on system and methods
* 
*/
////////////////////////////////////////////////
void UFSCombatComponent::LockOnValidCheck()
{
    if (!CurrentLockedOnTarget || !CachedDamageableLockOnTarget)
        return;

    FVector targetLocation{ CurrentLockedOnTarget->GetActorLocation() };
    FVector playerLocation{ PlayerOwner->GetActorLocation() };

    double distanceSq{ FVector::DistSquared(playerLocation, targetLocation) };
    if (distanceSq >= FMath::Square(LockOnDetectionRadius) || CachedDamageableLockOnTarget->IsDead())
        DisengageLockOn();
}

void UFSCombatComponent::UpdateLockOnCamera(float deltaTime)
{
    FVector PlayerLocation{ PlayerOwner->GetActorLocation() };
    FVector TargetLocation{ CurrentLockedOnTarget->GetActorLocation() };
    FRotator PlayerLookAtRotation{ UKismetMathLibrary::FindLookAtRotation(PlayerLocation, TargetLocation) };
    PlayerLookAtRotation.Pitch = 0.0f;

    FRotator CurrentPlayerRotation{ PlayerOwner->GetActorRotation() };
    FRotator SmoothedPlayerRotation{ FMath::RInterpTo(CurrentPlayerRotation, PlayerLookAtRotation, deltaTime, CameraRotationInterpSpeed) };
    PlayerOwner->SetActorRotation(SmoothedPlayerRotation);

    FRotator CameraLookAtRotation{ UKismetMathLibrary::FindLookAtRotation(PlayerLocation, TargetLocation) };
    FVector DirectionToTarget{ (TargetLocation - PlayerLocation).GetSafeNormal() };
    FVector PlayerRight{ PlayerOwner->GetActorRightVector() };
    float DotRight{ static_cast<float>(FVector::DotProduct(DirectionToTarget, PlayerRight)) };

    double Distance{ FVector::Dist(PlayerLocation, TargetLocation) };
    double maxClampDistance{ LockOnDetectionRadius / 2 };
    double DistanceRatio{ FMath::Clamp(Distance / maxClampDistance, 0.0, 1.0) };
    double CurrentYawOffset{ FMath::Lerp(CloseCameraYawOffset, FarCameraYawOffset, DistanceRatio) };

    double CurrentPitchOffset{ FMath::Lerp(CloseCameraPitchOffset, FarCameraPitchOffset, DistanceRatio) };

    float dotRightTolerance{ bIsAttacking ? -0.15f : 0.f };
    CameraLookAtRotation.Yaw += DotRight > dotRightTolerance ? -CurrentYawOffset : CurrentYawOffset;
    CameraLookAtRotation.Pitch += CurrentPitchOffset;

    FRotator CurrentCameraRotation{ PlayerOwner->GetController()->GetControlRotation() };
    FRotator SmoothedCameraRotation{ FMath::RInterpTo(CurrentCameraRotation, CameraLookAtRotation, deltaTime, CameraRotationInterpSpeed) };

    PlayerOwner->GetController()->SetControlRotation(SmoothedCameraRotation);
}

bool UFSCombatComponent::EngageLockOn() 
{
    if (!PlayerOwner)
        return false;

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
        LockOnDetectionRadius,
        ObjectTypes,
        false,
        ActorsToIgnore,
        EDrawDebugTrace::None,
        outHits,
        true
    ) };

    if (!bHit)
        return false;

    AController* Controller{ PlayerOwner->GetController() };
    if (!Controller)
        return false;

    FRotator ControlRotation{ Controller->GetControlRotation() };
    FVector CameraForward{ ControlRotation.Vector() };
    CameraForward.Z = 0;
    CameraForward.Normalize();

    TSet<AActor*> uniqueHitActors;
    for (const FHitResult& hit : outHits)
    {
        AActor* HitActor{ hit.GetActor() };
        if (uniqueHitActors.Contains(HitActor))
            continue;

        uniqueHitActors.Add(HitActor);

        if (!HitActor->Implements<UFSFocusable>() ||
            TargetsInLockOnRadius.Contains(HitActor) ||
            !HitActor->Implements<UFSDamageable>())
            continue;

        IFSDamageable* damageableTarget = Cast<IFSDamageable>(HitActor);
        if (!damageableTarget || damageableTarget->IsDead())
            continue;

        FVector TargetLocation{ HitActor->GetActorLocation() };
        FVector PlayerLocation{ PlayerOwner->GetActorLocation() };
        FVector DirectionToTarget{ TargetLocation - PlayerLocation };
        DirectionToTarget.Z = 0;

        if (DirectionToTarget.IsNearlyZero())
            continue;

        DirectionToTarget.Normalize();

        double DotForward{ FVector::DotProduct(DirectionToTarget, CameraForward) };

        if (DotForward > 0.0) // 120°
            TargetsInLockOnRadius.Add(HitActor);
    }

    if (TargetsInLockOnRadius.IsEmpty())
        return false;

    float distance{ LockOnDetectionRadius };
    for (AActor* target : TargetsInLockOnRadius)
    {
        FVector targetLocation{ target->GetActorLocation() };
        FVector playerLocation{ PlayerOwner->GetActorLocation() };
        double newDistance{ UKismetMathLibrary::Vector_Distance(playerLocation, targetLocation) };

        if (newDistance < distance)
        {
            distance = newDistance;
            CurrentLockedOnTarget = target;
            CachedDamageableLockOnTarget = Cast<IFSDamageable>(CurrentLockedOnTarget);
        }
    }

    if (!CurrentLockedOnTarget)
        return false;

    bIsLockedOnEngaged = true;
    PlayerOwner->GetController()->SetIgnoreLookInput(true);
    PlayerOwner->GetCharacterMovement()->bOrientRotationToMovement = false;
    PlayerOwner->GetCharacterMovement()->bUseControllerDesiredRotation = false; // remettre à TRUE après
    PrimaryComponentTick.SetTickFunctionEnable(true);
    Cast<IFSFocusable>(CurrentLockedOnTarget)->DisplayLockedOnWidget(true);

    GetWorld()->GetTimerManager().SetTimer(
        LockOnValidCheckTimer,
        this, &UFSCombatComponent::LockOnValidCheck,
        LockOnDistanceCheckDelay, 
        true
    );

    return true;
}

bool UFSCombatComponent::SwitchLockOnTarget(float axisValueX)
{
    if (!CurrentLockedOnTarget || GetWorld()->GetTimerManager().IsTimerActive(delaySwitchLockOnTimer))
        return false;

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
        LockOnDetectionRadius,
        ObjectTypes,
        false,
        ActorsToIgnore,
        EDrawDebugTrace::None,
        outHits,
        true
    ) };

    if (!bHit)
        return false;

    FRotator ControlRotation{ PlayerOwner->GetController()->GetControlRotation() };
    FVector CameraForward{ ControlRotation.Vector() };
    CameraForward.Z = 0;
    CameraForward.Normalize();

    FVector CameraRight{ FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::Y) };
    CameraRight.Z = 0;
    CameraRight.Normalize();

    FVector PlayerLocation{ PlayerOwner->GetActorLocation() };
    bool bLookingRight{ axisValueX > 0 };

    AActor* BestTarget{ nullptr };
    float SmallestAngle{ FLT_MAX };

    TSet<AActor*> ProcessedActors;
    for (const FHitResult& hit : outHits)
    {
        AActor* HitActor{ hit.GetActor() };

        if (ProcessedActors.Contains(HitActor) ||
            !HitActor->Implements<UFSFocusable>() ||
            HitActor == CurrentLockedOnTarget ||
            !HitActor->Implements<UFSDamageable>())
            continue;

        IFSDamageable* damageableTarget = Cast<IFSDamageable>(HitActor);
        if (!damageableTarget || damageableTarget->IsDead())
            continue;

        ProcessedActors.Add(HitActor);

        FVector TargetLocation{ HitActor->GetActorLocation() };
        FVector DirectionToTarget{ TargetLocation - PlayerLocation };
        DirectionToTarget.Z = 0;

        if (DirectionToTarget.IsNearlyZero())
            continue;

        DirectionToTarget.Normalize();

        double DotForward{ FVector::DotProduct(DirectionToTarget, CameraForward) };
        if (DotForward < 0.f) // FOV de 180°
            continue;

        double DotRight{ FVector::DotProduct(DirectionToTarget, CameraRight) };

        if (bLookingRight && DotRight <= 0)
            continue;

        else if (!bLookingRight && DotRight >= 0)
            continue;

        double AngleRadians{ FMath::Atan2(DotRight, DotForward) };
        double AngleDegrees{ FMath::RadiansToDegrees(AngleRadians) };
        double AbsAngle{ FMath::Abs(AngleDegrees) };

        if (AbsAngle < SmallestAngle)
        {
            SmallestAngle = AbsAngle;
            BestTarget = HitActor;
        }
    }

    if (!BestTarget)
        return false;

    // Remove UI Lock-on from previous target
    Cast<IFSFocusable>(CurrentLockedOnTarget)->DisplayLockedOnWidget(false);

    CurrentLockedOnTarget = BestTarget;
    CachedDamageableLockOnTarget = Cast<IFSDamageable>(CurrentLockedOnTarget);
    Cast<IFSFocusable>(CurrentLockedOnTarget)->DisplayLockedOnWidget(true);

    GetWorld()->GetTimerManager().SetTimer(delaySwitchLockOnTimer, targetSwitchDelay, false);

    return true;
}

void UFSCombatComponent::DisengageLockOn()
{
    TargetsInLockOnRadius.Empty();
    Cast<IFSFocusable>(CurrentLockedOnTarget)->DisplayLockedOnWidget(false);
    CurrentLockedOnTarget = nullptr;
    CachedDamageableLockOnTarget = nullptr;

    bIsLockedOnEngaged = false;

    PrimaryComponentTick.SetTickFunctionEnable(false);

    PlayerOwner->GetController()->ResetIgnoreLookInput();
    PlayerOwner->GetCharacterMovement()->bOrientRotationToMovement = true;
    PlayerOwner->GetCharacterMovement()->bUseControllerDesiredRotation = false;

    LockOnValidCheckTimer.Invalidate();
    OnLockOnStopped.Broadcast();
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

        if (hitActor->Implements<UFSDamageable>() || hitActor->Implements<UFSFocusable>())
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

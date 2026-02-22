#include "FSCombatComponent.h"

UFSCombatComponent::UFSCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
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

    // Cache MotionWarpingComponent reference
    MotionWarpingComponent = PlayerOwner->FindComponentByClass<UMotionWarpingComponent>();
    checkf(MotionWarpingComponent, TEXT("FATAL: MotionWarpingComponent not found on player!"));
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
    StandingLightCombo.Attacks[0].FlowReward = 4.f;
    StandingLightCombo.Attacks[0].AttackType = EAttackType::StandingLight;

    // Attack 2
    StandingLightCombo.Attacks[1].Damage = 55.f;
    StandingLightCombo.Attacks[1].KnockbackForce = 120.f;
    StandingLightCombo.Attacks[1].FlowReward = 5.f;
    StandingLightCombo.Attacks[1].AttackType = EAttackType::StandingLight;

    // Attack 3 (final) - Only this one has ChainableAttacks
    StandingLightCombo.Attacks[2].Damage = 60.f;
    StandingLightCombo.Attacks[2].KnockbackForce = 150.f;
    StandingLightCombo.Attacks[2].FlowReward = 6.f;
    StandingLightCombo.Attacks[2].AttackType = EAttackType::StandingLight;
    StandingLightCombo.Attacks[2].ChainableAttacks = {
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
    StandingHeavyCombo.Attacks[0].FlowReward = 10.f;
    StandingHeavyCombo.Attacks[0].AttackType = EAttackType::StandingHeavy;

    // Attack 2
    StandingHeavyCombo.Attacks[1].Damage = 75.f;
    StandingHeavyCombo.Attacks[1].KnockbackForce = 280.f;
    StandingHeavyCombo.Attacks[1].FlowReward = 12.f;
    StandingHeavyCombo.Attacks[1].AttackType = EAttackType::StandingHeavy;

    // Attack 3
    StandingHeavyCombo.Attacks[2].Damage = 80.f;
    StandingHeavyCombo.Attacks[2].KnockbackForce = 320.f;
    StandingHeavyCombo.Attacks[2].FlowReward = 14.f;
    StandingHeavyCombo.Attacks[2].AttackType = EAttackType::StandingHeavy;

    // Attack 4 (final) - Only this one has ChainableAttacks
    StandingHeavyCombo.Attacks[3].Damage = 90.f;
    StandingHeavyCombo.Attacks[3].KnockbackForce = 400.f;
    StandingHeavyCombo.Attacks[3].FlowReward = 18.f;
    StandingHeavyCombo.Attacks[3].AttackType = EAttackType::StandingHeavy;
    StandingHeavyCombo.Attacks[3].ChainableAttacks = {
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
        RunningLightCombo.Attacks[0].FlowReward = 3.f;
        RunningLightCombo.Attacks[0].AttackType = EAttackType::RunningLight;

        // Attack 2
        RunningLightCombo.Attacks[1].Damage = 52.f;
        RunningLightCombo.Attacks[1].KnockbackForce = 130.f;
        RunningLightCombo.Attacks[1].FlowReward = 3.f;
        RunningLightCombo.Attacks[1].AttackType = EAttackType::RunningLight;

        // Attack 3
        RunningLightCombo.Attacks[2].Damage = 55.f;
        RunningLightCombo.Attacks[2].KnockbackForce = 140.f;
        RunningLightCombo.Attacks[2].FlowReward = 3.f;
        RunningLightCombo.Attacks[2].AttackType = EAttackType::RunningLight;

        // Attack 4
        RunningLightCombo.Attacks[3].Damage = 58.f;
        RunningLightCombo.Attacks[3].KnockbackForce = 150.f;
        RunningLightCombo.Attacks[3].FlowReward = 3.f;
        RunningLightCombo.Attacks[3].AttackType = EAttackType::RunningLight;

        // Attack 5
        RunningLightCombo.Attacks[4].Damage = 60.f;
        RunningLightCombo.Attacks[4].KnockbackForce = 160.f;
        RunningLightCombo.Attacks[4].FlowReward = 4.f;
        RunningLightCombo.Attacks[4].AttackType = EAttackType::RunningLight;

        // Attack 6
        RunningLightCombo.Attacks[5].Damage = 65.f;
        RunningLightCombo.Attacks[5].KnockbackForce = 180.f;
        RunningLightCombo.Attacks[5].FlowReward = 4.f;
        RunningLightCombo.Attacks[5].AttackType = EAttackType::RunningLight;

        // Attack 7 (final) - Only this one has ChainableAttacks
        RunningLightCombo.Attacks[6].Damage = 70.f;
        RunningLightCombo.Attacks[6].KnockbackForce = 200.f;
        RunningLightCombo.Attacks[6].FlowReward = 5.f;
        RunningLightCombo.Attacks[6].AttackType = EAttackType::RunningLight;
        RunningLightCombo.Attacks[6].ChainableAttacks = {
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
        RunningHeavyCombo.Attacks[0].FlowReward = 10.f;
        RunningHeavyCombo.Attacks[0].AttackType = EAttackType::RunningHeavy;

        // Attack 2
        RunningHeavyCombo.Attacks[1].Damage = 80.f;
        RunningHeavyCombo.Attacks[1].KnockbackForce = 320.f;
        RunningHeavyCombo.Attacks[1].FlowReward = 12.f;
        RunningHeavyCombo.Attacks[1].AttackType = EAttackType::RunningHeavy;

        // Attack 3
        RunningHeavyCombo.Attacks[2].Damage = 85.f;
        RunningHeavyCombo.Attacks[2].KnockbackForce = 350.f;
        RunningHeavyCombo.Attacks[2].FlowReward = 14.f;
        RunningHeavyCombo.Attacks[2].AttackType = EAttackType::RunningHeavy;

        // Attack 4 (final) - Only this one has ChainableAttacks
        RunningHeavyCombo.Attacks[3].Damage = 95.f;
        RunningHeavyCombo.Attacks[3].KnockbackForce = 420.f;
        RunningHeavyCombo.Attacks[3].FlowReward = 18.f;
        RunningHeavyCombo.Attacks[3].AttackType = EAttackType::RunningHeavy;
        RunningHeavyCombo.Attacks[3].ChainableAttacks = {
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
        DashPierceAttack.Attacks[0].FlowReward = 8.f;
        DashPierceAttack.Attacks[0].AttackType = EAttackType::DashPierce;
        DashPierceAttack.Attacks[0].ChainableAttacks = {
            EAttackType::RunningLight,
            EAttackType::StandingLight,
            EAttackType::Launcher,
            EAttackType::PowerLauncher,
            EAttackType::PierceThrust,
            EAttackType::SpinAttack
        };
        DashPierceAttack.Attacks[0].OnAttackExecuted.BindLambda([this]() 
            { 
                const FName warpTargetName{ "DashIn" };
                constexpr float notifyStart{ 0.09f };
                constexpr float notifyEnd{ 0.52f };
                constexpr float searchRadius{ 600.f };
                SetupGroundAttackMotionWarp(warpTargetName, notifyStart, notifyEnd, searchRadius);
            });
    }

    // === DASH SPINNING SLASH ===
    DashSpinningSlashAttack.Attacks.SetNum(1);
    {
        DashSpinningSlashAttack.Attacks[0].Damage = 75.f;
        DashSpinningSlashAttack.Attacks[0].KnockbackForce = 250.f;
        DashSpinningSlashAttack.Attacks[0].FlowReward = 8.f;
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
        DashSpinningSlashAttack.Attacks[0].OnAttackExecuted.BindLambda([this]()
            {
                const FName warpTargetName{ "DashIn" };
                constexpr float notifyStart{ 0.16f };
                constexpr float notifyEnd{ 0.51f };
                constexpr float searchRadius{ 600.f };
                SetupGroundAttackMotionWarp(warpTargetName, notifyStart, notifyEnd, searchRadius);
            });
    }

    // === DASH DOUBLE SLASH ===
    DashDoubleSlashAttack.Attacks.SetNum(1);
    {
        DashDoubleSlashAttack.Attacks[0].Damage = 90.f;
        DashDoubleSlashAttack.Attacks[0].KnockbackForce = 300.f;
        DashDoubleSlashAttack.Attacks[0].FlowReward = 10.f;
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
        DashBackSlashAttack.Attacks[0].FlowReward = 7.f;
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
        JumpSlamAttack.Attacks[0].FlowReward = 12.f;
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
        JumpForwardSlamAttack.Attacks[0].FlowReward = 13.f;
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
        JumpUpperSlamComboAttack.Attacks[0].FlowReward = 14.f;
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
        LauncherAttack.Attacks[0].FlowReward = 8.f;
        LauncherAttack.Attacks[0].AttackType = EAttackType::Launcher;
        LauncherAttack.Attacks[0].ChainableAttacks = {
            EAttackType::AirCombo,
            EAttackType::AerialSlam
        };
        LauncherAttack.Attacks[0].OnAttackExecuted.BindLambda([this]() 
            { 
            const FName warpTargetName{ "DashIn" };
            constexpr float notifyStart{ 0.1f };
            constexpr float notifyEnd{ 0.35f };
            constexpr float searchRadius{ 400.f };
            SetupGroundAttackMotionWarp(warpTargetName, notifyStart, notifyEnd, searchRadius);


            const FName airWarpTargetName{ "AttackTarget" };
            constexpr float airNotifyStart{ 0.4f };
            constexpr float airNotifyEnd{ 0.82f };
            constexpr float airSearchRadius{ 400.f };
            constexpr float zOffset{ 150.f };
            SetupAirAttackMotionWarp(airWarpTargetName, airNotifyStart, airNotifyEnd, airSearchRadius, false, zOffset); 
            });
        LauncherAttack.Attacks[0].OnAttackHit.BindUObject(this, &UFSCombatComponent::OnLauncherAttackHit);
    }

    // === POWER LAUNCHER ===
    PowerLauncherAttack.Attacks.SetNum(1);
    {
        PowerLauncherAttack.Attacks[0].Damage = 80.f;
        PowerLauncherAttack.Attacks[0].KnockbackForce = 250.f;
        PowerLauncherAttack.Attacks[0].KnockbackUpForce = 1000.f;
        PowerLauncherAttack.Attacks[0].FlowReward = 10.f;
        PowerLauncherAttack.Attacks[0].AttackType = EAttackType::PowerLauncher;
        PowerLauncherAttack.Attacks[0].ChainableAttacks = {
            EAttackType::AirCombo,
            EAttackType::AerialSlam
        };
        PowerLauncherAttack.Attacks[0].OnAttackExecuted.BindLambda([this]() 
            { 
            const FName warpTargetName{ "DashIn" };
            constexpr float notifyStart{ 0.3f };
            constexpr float notifyEnd{ 0.42f };
            constexpr float searchRadius{ 400.f };
            SetupGroundAttackMotionWarp(warpTargetName, notifyStart, notifyEnd, searchRadius);

            const FName airWarpTargetName{ "AttackTarget" };
            constexpr float airNotifyStart{ 0.47f };
            constexpr float airNotifyEnd{ 0.86f };
            constexpr float airSearchRadius{ 400.f };
            constexpr float zOffset{ 250.f };
            SetupAirAttackMotionWarp(airWarpTargetName, airNotifyStart, airNotifyEnd, airSearchRadius, false, zOffset);
            });
        PowerLauncherAttack.Attacks[0].OnAttackHit.BindUObject(this, &UFSCombatComponent::OnLauncherAttackHit);
    }

    // === SPIN ATTACK ===
    SpinAttack.Attacks.SetNum(1);
    {
        SpinAttack.Attacks[0].Damage = 65.f;
        SpinAttack.Attacks[0].KnockbackForce = 200.f;
        SpinAttack.Attacks[0].FlowReward = 8.f;
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
        HorizontalSweepAttack.Attacks[0].FlowReward = 10.f;
        HorizontalSweepAttack.Attacks[0].AttackType = EAttackType::HorizontalSweep;
        HorizontalSweepAttack.Attacks[0].ChainableAttacks = {
            EAttackType::StandingHeavy,
            EAttackType::RunningHeavy,
            EAttackType::GroundSlam,
            EAttackType::SpinAttack,
            EAttackType::Launcher,
            EAttackType::PowerSlash
        };
        HorizontalSweepAttack.Attacks[0].OnAttackExecuted.BindLambda([this]()
            {
                const FName warpTargetName{ "Rotate" };
                constexpr float notifyStart{ 0.f };
                constexpr float notifyEnd{ 0.24f };
                constexpr float searchRadius{ 300.f };
                SetupGroundAttackMotionWarp(warpTargetName, notifyStart, notifyEnd, searchRadius);
            });
    }

    // === PIERCE THRUST ===
    PierceThrustAttack.Attacks.SetNum(1);
    {
        PierceThrustAttack.Attacks[0].Damage = 90.f;
        PierceThrustAttack.Attacks[0].KnockbackForce = 350.f;
        PierceThrustAttack.Attacks[0].FlowReward = 10.f;
        PierceThrustAttack.Attacks[0].AttackType = EAttackType::PierceThrust;
        PierceThrustAttack.Attacks[0].ChainableAttacks = {
            EAttackType::StandingLight,
            EAttackType::RunningLight,
            EAttackType::PowerSlash,
            EAttackType::DashPierce,
            EAttackType::Launcher,
            EAttackType::SpinAttack
        };
        PierceThrustAttack.Attacks[0].OnAttackExecuted.BindLambda([this]()
            {
                const FName warpTargetName{ "Rotate" };
                constexpr float notifyStart{ 0.f };
                constexpr float notifyEnd{ 0.36f };
                constexpr float searchRadius{ 300.f };
                SetupGroundAttackMotionWarp(warpTargetName, notifyStart, notifyEnd, searchRadius);
            });
    }

    // === POWER SLASH ===
    PowerSlashAttack.Attacks.SetNum(1);
    {
        PowerSlashAttack.Attacks[0].Damage = 120.f;
        PowerSlashAttack.Attacks[0].KnockbackForce = 500.f;
        PowerSlashAttack.Attacks[0].FlowReward = 18.f;
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
        DiagonalRetourneAttack.Attacks[0].FlowReward = 10.f;
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
        DiagonalRetourneAttack.Attacks[0].OnAttackExecuted.BindLambda([this]()
            {
                const FName warpTargetName{ "Rotate" };
                constexpr float notifyStart{ 0.f };
                constexpr float notifyEnd{ 0.62f };
                constexpr float searchRadius{ 300.f };
                SetupGroundAttackMotionWarp(warpTargetName, notifyStart, notifyEnd, searchRadius);
            });
    }

    // === GROUND SLAM ===
    GroundSlamAttack.Attacks.SetNum(1);
    {
        GroundSlamAttack.Attacks[0].Damage = 130.f;
        GroundSlamAttack.Attacks[0].KnockbackForce = 600.f;
        GroundSlamAttack.Attacks[0].KnockbackUpForce = 300.f;
        GroundSlamAttack.Attacks[0].FlowReward = 20.f;
        GroundSlamAttack.Attacks[0].AttackType = EAttackType::GroundSlam;
        GroundSlamAttack.Attacks[0].ChainableAttacks = {
            EAttackType::StandingHeavy,
            EAttackType::Launcher,
            EAttackType::PowerSlash
        };
        GroundSlamAttack.Attacks[0].OnAttackExecuted.BindLambda([this]()
            {
                const FName warpTargetName{ "Rotate" };
                constexpr float notifyStart{ 0.f };
                constexpr float notifyEnd{ 0.37f };
                constexpr float searchRadius{ 250.f };
                SetupGroundAttackMotionWarp(warpTargetName, notifyStart, notifyEnd, searchRadius);
            });
    }

    // === AIR COMBO (3 attacks) ===
    AirCombo.Attacks.SetNum(3);
    {
        // Attack 1
        AirCombo.Attacks[0].Damage = 50.f;
        AirCombo.Attacks[0].KnockbackForce = 80.f;
        AirCombo.Attacks[0].FlowReward = 4.f;
        AirCombo.Attacks[0].AttackType = EAttackType::AirCombo;
        AirCombo.Attacks[0].OnAttackExecuted.BindLambda([this]() 
            { 
                const FName airWarpTargetName{ "AttackTarget" };
                constexpr float airNotifyStart{ 0.15f };
                constexpr float airNotifyEnd{ 0.34f };
                constexpr float airSearchRadius{ 250.f };
                SetupAirAttackMotionWarp(airWarpTargetName, airNotifyStart, airNotifyEnd, airSearchRadius);
            });
        AirCombo.Attacks[0].OnAttackHit.BindUObject(this, &UFSCombatComponent::OnAirAttackHit);

        // Attack 2
        AirCombo.Attacks[1].Damage = 55.f;
        AirCombo.Attacks[1].KnockbackForce = 100.f;
        AirCombo.Attacks[1].FlowReward = 5.f;
        AirCombo.Attacks[1].AttackType = EAttackType::AirCombo;
        AirCombo.Attacks[1].OnAttackExecuted.BindLambda([this]() 
            { 
                const FName airWarpTargetName{ "AttackTarget" };
                constexpr float airNotifyStart{ 0.33f };
                constexpr float airNotifyEnd{ 0.52f };
                constexpr float airSearchRadius{ 250.f };
                SetupAirAttackMotionWarp(airWarpTargetName, airNotifyStart, airNotifyEnd, airSearchRadius);
            });
        AirCombo.Attacks[1].OnAttackHit.BindUObject(this, &UFSCombatComponent::OnAirAttackHit);

        // Attack 3 (final) - Only this one has ChainableAttacks
        AirCombo.Attacks[2].Damage = 60.f;
        AirCombo.Attacks[2].KnockbackForce = 120.f;
        AirCombo.Attacks[2].FlowReward = 6.f;
        AirCombo.Attacks[2].AttackType = EAttackType::AirCombo;
        AirCombo.Attacks[2].ChainableAttacks = { EAttackType::AerialSlam };
        AirCombo.Attacks[2].OnAttackExecuted.BindLambda([this]() 
            { 
                const FName airWarpTargetName{ "AttackTarget" };
                constexpr float airNotifyStart{ 0.14f };
                constexpr float airNotifyEnd{ 0.28f };
                constexpr float airSearchRadius{ 250.f };
                SetupAirAttackMotionWarp(airWarpTargetName, airNotifyStart, airNotifyEnd, airSearchRadius);
            });
        AirCombo.Attacks[2].OnAttackHit.BindUObject(this, &UFSCombatComponent::OnAirAttackHit);
    }

    // === AERIAL SLAM ===
    AerialSlamAttack.Attacks.SetNum(1);
    {
        AerialSlamAttack.Attacks[0].Damage = 110.f;
        AerialSlamAttack.Attacks[0].KnockbackForce = 500.f;
        AerialSlamAttack.Attacks[0].KnockbackUpForce = -1500.f;
        AerialSlamAttack.Attacks[0].FlowReward = 15.f;
        AerialSlamAttack.Attacks[0].AttackType = EAttackType::AerialSlam;
        AerialSlamAttack.Attacks[0].ChainableAttacks = {
            EAttackType::StandingLight,
            EAttackType::RunningLight,
            EAttackType::StandingHeavy,
            EAttackType::Launcher,
            EAttackType::GroundSlam,
            EAttackType::SpinAttack
        };
        AerialSlamAttack.Attacks[0].OnAttackExecuted.BindLambda([this]()
            {
                const FName airWarpTargetName{ "AttackTarget" };
                constexpr float airNotifyStart{ 0.12f };
                constexpr float airNotifyEnd{ 0.51f };
                constexpr float airSearchRadius{ 250.f };
                SetupAirAttackMotionWarp(airWarpTargetName, airNotifyStart, airNotifyEnd, airSearchRadius);
            });
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
    ResetComboState();
    equippedWeapon->DeactivateHitbox();
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

AActor* UFSCombatComponent::GetTargetForMotionWarp(float searchRadius, bool debugLines)
{
    if (LockedOnTarget && (FVector::DistSquared(PlayerOwner->GetActorLocation(), LockedOnTarget->GetActorLocation()) <= (searchRadius * searchRadius)))
        return LockedOnTarget;

    return GetNearestEnemyFromPlayer(searchRadius, debugLines);
}

void UFSCombatComponent::SetupAirAttackMotionWarp(FName motionWarpingTargetName, float notifyStartTime, float notifyEndTime, float searchRadius, bool debugLines, float zOffset, float forwardOffset)
{
    AActor* nearestEnemy{ GetTargetForMotionWarp(searchRadius, debugLines) };
    if (!MotionWarpingComponent || !nearestEnemy)
        return;

    TWeakObjectPtr<AActor> weakEnemy{ nearestEnemy };
    TWeakObjectPtr<UMotionWarpingComponent> weakMotionWarp{ MotionWarpingComponent };

    FTimerHandle flyingStartTimer;
    GetWorld()->GetTimerManager().SetTimer(
        flyingStartTimer,
        [this, weakEnemy, weakMotionWarp, zOffset, forwardOffset, motionWarpingTargetName]()
        {
            if (!weakEnemy.IsValid() || !weakMotionWarp.IsValid())
                return;

            FVector playerLocation{ PlayerOwner->GetActorLocation() };
            FVector enemyLocation{ weakEnemy->GetActorLocation() };

            FVector directionToEnemy{ (enemyLocation - playerLocation).GetSafeNormal() };

            FVector targetLocation{ enemyLocation };
            targetLocation.Z += zOffset;
            targetLocation -= directionToEnemy * forwardOffset; 

            FRotator lookAtRotation{ UKismetMathLibrary::FindLookAtRotation(playerLocation, enemyLocation) };

            FMotionWarpingTarget target;
            target.Name = motionWarpingTargetName;
            target.Location = targetLocation;
            target.Rotation = lookAtRotation;

            PlayerOwner->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
            weakMotionWarp->AddOrUpdateWarpTarget(target);
        },
        notifyStartTime,
        false
    );

    FTimerHandle flyingEndTimer;
    GetWorld()->GetTimerManager().SetTimer(
        flyingEndTimer,
        [this, weakMotionWarp]()
        {
            if (!weakMotionWarp.IsValid())
                return;

            PlayerOwner->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
            weakMotionWarp->RemoveAllWarpTargets();
        },
        notifyEndTime,
        false
    );
}

void UFSCombatComponent::SetupGroundAttackMotionWarp(FName motionWarpingTargetName, float notifyStartTime, float notifyEndTime, float searchRadius, bool debugLines, float forwardOffset)
{
    AActor* nearestEnemy{ GetTargetForMotionWarp(searchRadius, debugLines) };
    if (!MotionWarpingComponent || !nearestEnemy)
        return;

    FVector targetLocation{ nearestEnemy->GetActorLocation() };

    FVector forwardOffsetVector{ PlayerOwner->GetActorForwardVector() * forwardOffset };
    targetLocation += forwardOffsetVector;

    FVector playerLocation{ PlayerOwner->GetActorLocation() };
    FVector enemyLocation{ nearestEnemy->GetActorLocation() };
    FRotator lookAtRotation{ UKismetMathLibrary::FindLookAtRotation(playerLocation, enemyLocation) };

    FMotionWarpingTarget target;
    target.Name = motionWarpingTargetName;
    target.Location = targetLocation;
    target.Rotation = lookAtRotation;

    MotionWarpingComponent->AddOrUpdateWarpTarget(target);

    TWeakObjectPtr<UMotionWarpingComponent> weakMotionWarp{ MotionWarpingComponent };

    FTimerHandle notifyStartTimer;
    GetWorld()->GetTimerManager().SetTimer(
        notifyStartTimer,
        [weakMotionWarp, target]()
        {
            if (weakMotionWarp.IsValid())
                weakMotionWarp->AddOrUpdateWarpTarget(target);
        },
        notifyStartTime,
        false
    );

    FTimerHandle notifyEndTimer;
    GetWorld()->GetTimerManager().SetTimer(
        notifyEndTimer,
        [weakMotionWarp]()
        {
            if (weakMotionWarp.IsValid())
                weakMotionWarp->RemoveAllWarpTargets();
        },
        notifyEndTime,
        false
    );
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

    PlayerOwner->PlayAnimMontage(firstAttack);

    if (OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.IsBound())
        OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.Execute();
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

    PlayerOwner->PlayAnimMontage(nextAnimAttack);

    if (OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.IsBound())
        OngoingCombo->GetAttackAt(ComboIndex - 1)->OnAttackExecuted.Execute();
}

void UFSCombatComponent::ResetComboState()
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
void UFSCombatComponent::OnHitLanded(AActor* hitActor, const FVector& hitLocation)
{
    if (!hitActor || Cast<IFSDamageable>(hitActor)->IsDead())
        return;
    
    // NOTE: ComboIndex has already been incremented at this point, 
    // so the actual ComboIndex to this attack that hit the enemy here is : ComboIndex - 1
    const FAttackData* currentAttack{ OngoingCombo->GetAttackAt(ComboIndex - 1) };
    if (!currentAttack)
        return;

    OnHitLandedNotify.Broadcast(hitActor, hitLocation, currentAttack->Damage, currentAttack->FlowReward);

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
        hitstopDuration * hitstopTimeDilation,
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

    TWeakObjectPtr<USkeletalMeshComponent> weakEnemyMesh{ enemyMesh };

    FTimerHandle flashTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        flashTimerHandle,
        [weakEnemyMesh, ogMats]()
        {
            if (!weakEnemyMesh.IsValid())
                return;

            for (int32 i{}; i < weakEnemyMesh->GetNumMaterials(); i++)
                if (ogMats.IsValidIndex(i))
                    weakEnemyMesh->SetMaterial(i, ogMats[i]);
        },
        hitFlashDuration,
        false
    );
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

        if (hitActor->Implements<UFSDamageable>() && !Cast<IFSDamageable>(hitActor)->IsDead())
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

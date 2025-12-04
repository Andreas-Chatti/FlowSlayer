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

    OnHitboxActivated.AddUObject(equippedWeapon, &AFSWeapon::ActivateHitbox);
    OnHitboxDeactivated.AddUObject(equippedWeapon, &AFSWeapon::DeactivateHitbox);
    equippedWeapon->OnEnemyHit.AddUObject(this, &UFSCombatComponent::OnHitLanded);
    equippedWeapon->setDamage(Damage);

    // Bind MODULAR combo window delegates (broadcasted by AnimNotifyState_ModularCombo)
    OnModularComboWindowOpened.AddUObject(this, &UFSCombatComponent::HandleModularComboWindowOpened);
    OnModularComboWindowClosed.AddUObject(this, &UFSCombatComponent::HandleModularComboWindowClosed);

    // Bind FULL combo window delegates (broadcasted by AnimNotifyState_FullCombo)
    OnFullComboWindowOpened.AddUObject(this, &UFSCombatComponent::HandleFullComboWindowOpened);
    OnFullComboWindowClosed.AddUObject(this, &UFSCombatComponent::HandleFullComboWindowClosed);

    // Bind Air stall delegates for air combos (broadcasted by AirStallNotify)
    OnAirStallStarted.AddUObject(this, &UFSCombatComponent::HandleAirStallStarted);
    OnAirStallFinished.AddUObject(this, &UFSCombatComponent::HandleAirStallFinished);

    AnimInstance->OnMontageEnded.AddDynamic(this, &UFSCombatComponent::OnMontageEnded);
}

void UFSCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsLockedOnEngaged && CurrentLockedOnTarget)
    {
        FVector playerLocation{ PlayerOwner->GetActorLocation() };
        FVector currentLockedOnTargetLocation{ CurrentLockedOnTarget->GetActorLocation() };
        PlayerOwner->GetController()->SetControlRotation(UKismetMathLibrary::FindLookAtRotation(playerLocation, currentLockedOnTargetLocation));
    }
}

bool UFSCombatComponent::InitializeAndAttachWeapon()
{
    if (!weaponClass)
    {
        UE_LOG(LogTemp, Error, TEXT("LOADING EQUIPPED WEAPON FAILED"));
        return false;
    }

    FActorSpawnParameters spawnParams;
    spawnParams.Owner = PlayerOwner;
    equippedWeapon = GetWorld()->SpawnActor<AFSWeapon>(weaponClass, spawnParams);
    if (!equippedWeapon || !equippedWeapon->AttachToComponent(PlayerOwner->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("WeaponSocket")))
    {
        UE_LOG(LogTemp, Error, TEXT("LOADING EQUIPPED WEAPON FAILED"));
        return false;
    }

    return true;
}


////////////////////////////////////////////////
/*
* 
* Core combo-related methods 
* 
*/
////////////////////////////////////////////////
void UFSCombatComponent::Attack(EAttackType attackTypeInput, bool isMoving, bool isFalling)
{
    if (bIsAttacking && !bComboWindowOpened)
        return;

    else if (bIsAttacking && bComboWindowOpened)
    {
        // Allow combo continuation if it's a full combo OR we haven't reached the end yet
        if (OngoingCombo->IsFullCombo() || ComboIndex <= OngoingCombo->GetMaxComboIndex())
        {
            bComboWindowOpened = false;
            bContinueCombo = true;
        }

        return;
    }

    if (AnimInstance && AnimInstance->IsAnyMontagePlaying())
        return;

    bIsAttacking = true;

    OngoingCombo = SelectComboBasedOnState(attackTypeInput, isMoving, isFalling);
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
}

FCombo* UFSCombatComponent::SelectComboBasedOnState(EAttackType attackTypeInput, bool isMoving, bool isFalling)
{
    FCombo* selectedCombo{ nullptr };
    if (isMoving && !isFalling)
    {
        switch (attackTypeInput)
        {
        case EAttackType::Light:
            selectedCombo = &RunningLightCombo;
            break;
        case EAttackType::Heavy:
            selectedCombo = &RunningHeavyCombo;
            break;
        }
    }

    else if (!isMoving && !isFalling)
    {
        switch (attackTypeInput)
        {
        case EAttackType::Light:
            selectedCombo = &StandingLightCombo;
            break;
        case EAttackType::Heavy:
            selectedCombo = &StandingHeavyCombo;
            break;
        }
    }

    else if (isFalling)
    {
        switch (attackTypeInput)
        {
        case UFSCombatComponent::EAttackType::Light:
            selectedCombo = &AirCombo;
            break;
        case UFSCombatComponent::EAttackType::Heavy:
            selectedCombo = &AirCombo;
            break;
        }
    }

    return selectedCombo;
}

UAnimMontage* UFSCombatComponent::GetComboNextAttack(const FCombo& combo)
{
    UAnimMontage* attackToPerform{ nullptr };

    if (ComboIndex >= combo.GetMaxComboIndex())
        attackToPerform = combo.GetLastAttack();

    else
        attackToPerform = combo.GetAttackAt(ComboIndex);

    ++ComboIndex;

    return attackToPerform;
}

void UFSCombatComponent::HandleModularComboWindowOpened()
{
    bComboWindowOpened = true;
    UE_LOG(LogTemp, Error, TEXT("MODULAR Combo Window OPENED (via delegate)!"));
}

void UFSCombatComponent::HandleModularComboWindowClosed()
{
    bComboWindowOpened = false;
    UE_LOG(LogTemp, Error, TEXT("MODULAR Combo Window CLOSED (via delegate)!"));

    // If we've exceeded the max combo index, we're at the end of the combo chain
    // Reset and let the animation finish naturally (no Montage_Stop)
    if (ComboIndex > OngoingCombo->GetMaxComboIndex())
    {
        UE_LOG(LogTemp, Warning, TEXT("End of combo reached - Resetting"));
        return;
    }

    else if (!bContinueCombo && AnimInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player didn't continue MODULAR combo - Letting animation stops"));
        return;
    }

    ContinueCombo();
}

void UFSCombatComponent::HandleFullComboWindowOpened()
{
    UE_LOG(LogTemp, Error, TEXT("FULL Combo Window OPENED (via delegate)!"));
    bComboWindowOpened = true;
}

void UFSCombatComponent::HandleFullComboWindowClosed()
{
    bComboWindowOpened = false;
    UE_LOG(LogTemp, Error, TEXT("FULL Combo Window CLOSED (via delegate)!"));

    // If player didn't buffer a continue input, stop the combo early
    if (!bContinueCombo && AnimInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player didn't continue FULL combo - Stopping"));
        AnimInstance->Montage_Stop(0.6f, AnimInstance->GetCurrentActiveMontage());
        return;
    }
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

    if (!OngoingCombo || !OngoingCombo->IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("OngoingCombo is null or invalid - Stopping"));
        AnimInstance->Montage_Stop(0.6f, AnimInstance->GetCurrentActiveMontage());
        return;
    }

    if (OngoingCombo->IsFullCombo())
    {
        UE_LOG(LogTemp, Warning, TEXT("FullCombo detected - Letting animation continue naturally"));
        // Increment ComboIndex to track progress through combo windows
        ++ComboIndex;
        return;
    }

    // MODULAR COMBO: Multiple separate montages - switch to next attack montage
    UAnimMontage* nextAnimAttack{ GetComboNextAttack(*OngoingCombo) };
    if (!nextAnimAttack)
    {
        UE_LOG(LogTemp, Error, TEXT("nextAnimAttack is null - Stopping"));
        AnimInstance->Montage_Stop(0.6f, AnimInstance->GetCurrentActiveMontage());
        return;
    }

    // Successfully continuing the modular combo - play next attack animation
    UE_LOG(LogTemp, Warning, TEXT("Continuing modular combo - Playing next attack"));
    PlayerOwner->PlayAnimMontage(nextAnimAttack);
}

void UFSCombatComponent::StopCombo()
{
    ComboIndex = 0;
    bIsAttacking = false;
    //OngoingCombo = nullptr;
    
    UE_LOG(LogTemp, Error, TEXT("COMBO RESET !"));
}

void UFSCombatComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // Safety check: ensure we have a valid ongoing combo
    if (!OngoingCombo || !OngoingCombo->IsValid())
        return;

    // If the montage that ended is the last attack OR if a full combo was interrupted
    if (Montage == OngoingCombo->GetLastAttack() || (OngoingCombo->IsFullCombo() && bInterrupted))
        StopCombo();

    // If it's a modular combo and it finished naturally (not interrupted)
    else if (OngoingCombo->IsModularCombo() && !bInterrupted)
        StopCombo();
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

    ApplyHitstop();
    SpawnHitVFX(hitLocation);
    PlayHitSound(hitLocation);
    ApplyCameraShake();
    ApplyHitFlash(hitActor);
}

void UFSCombatComponent::ApplyHitstop()
{
    // Ralentit le temps globalement
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), hitstopTimeDilation);

    // Reset apr�s la dur�e
    GetWorld()->GetTimerManager().SetTimer(
        hitstopTimerHandle,
        this,
        &UFSCombatComponent::ResetTimeDilation,
        hitstopDuration * hitstopTimeDilation, // Ajust� pour le slow-mo
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
void UFSCombatComponent::LockOnDistanceCheck()
{
    if (!CurrentLockedOnTarget)
        return;

    FVector targetLocation{ CurrentLockedOnTarget->GetActorLocation() };
    FVector playerLocation{ PlayerOwner->GetActorLocation() };

    double distance{ UKismetMathLibrary::Vector_Distance(playerLocation, targetLocation) };
    if (distance >= LockOnDetectionRadius)
        DisengageLockOn();
}

bool UFSCombatComponent::EngageLockOn()
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
        LockOnDetectionRadius,
        ObjectTypes,
        false,              // Trace Complex
        ActorsToIgnore,     // Actors to Ignore
        EDrawDebugTrace::ForDuration,  // Draw Debug Type
        outHits,            // Out Hits
        true                // Ignore Self
    ) };

    if (!bHit)
        return false;

    TSet<AActor*> uniqueHitActors;
    for (const FHitResult& hit : outHits)
    {
        AActor* HitActor{ hit.GetActor() };
        if (uniqueHitActors.Contains(HitActor))
            continue;

        uniqueHitActors.Add(HitActor);
        if (HitActor->Implements<UFSFocusable>() && !TargetsInLockOnRadius.Contains(HitActor))
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
        }
    }

    if (!CurrentLockedOnTarget)
        return false;

    bIsLockedOnEngaged = true;
    PlayerOwner->GetController()->SetIgnoreLookInput(true);
    PlayerOwner->GetCharacterMovement()->bOrientRotationToMovement = false;
    PlayerOwner->GetCharacterMovement()->bUseControllerDesiredRotation = true;
    PrimaryComponentTick.SetTickFunctionEnable(true);

    GetWorld()->GetTimerManager().SetTimer(
        LockOnDistanceCheckTimer, 
        this, &UFSCombatComponent::LockOnDistanceCheck, 
        LockOnDistanceCheckDelay, 
        true
    );

    return true;
}

bool UFSCombatComponent::SwitchLockOnTarget(UCameraComponent* followCamera, float axisValueX)
{
    if (!CurrentLockedOnTarget || !followCamera || GetWorld()->GetTimerManager().IsTimerActive(delaySwitchLockOnTimer))
        return false;

    bool bLookingRight{ (axisValueX > 0) };

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
        EDrawDebugTrace::ForDuration,
        outHits,
        true
    ) };

    if (!bHit)
        return false;

    FVector CameraForward{ followCamera->GetForwardVector() };
    FVector CameraRight{ followCamera->GetRightVector() };
    CameraForward.Z = 0;
    CameraRight.Z = 0;
    CameraForward.Normalize();
    CameraRight.Normalize();

    AActor* BestTarget{ nullptr };
    float SmallestAngle{ FLT_MAX };

    TSet<AActor*> ProcessedActors;
    for (const FHitResult& hit : outHits)
    {
        AActor* HitActor{ hit.GetActor() };

        if (ProcessedActors.Contains(HitActor) || !HitActor->Implements<UFSFocusable>() || HitActor == CurrentLockedOnTarget)
            continue;

        ProcessedActors.Add(HitActor);

        FVector TargetLocation{ HitActor->GetActorLocation() };
        FVector PlayerLocation{ PlayerOwner->GetActorLocation() };
        FVector DirectionToTarget{ TargetLocation - PlayerLocation };
        DirectionToTarget.Z = 0;

        if (DirectionToTarget.IsNearlyZero())
            continue;

        DirectionToTarget.Normalize();

        double DotRight{ FVector::DotProduct(DirectionToTarget, CameraRight) };

        if (bLookingRight && DotRight <= 0)
            continue;

        else if (!bLookingRight && DotRight >= 0)
            continue;

        double AngleRadians{ FMath::Atan2(DotRight, FVector::DotProduct(DirectionToTarget, CameraForward)) };
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

    CurrentLockedOnTarget = BestTarget;

    GetWorld()->GetTimerManager().SetTimer(delaySwitchLockOnTimer, targetSwitchDelay, false);

    return true;
}

void UFSCombatComponent::DisengageLockOn()
{
    TargetsInLockOnRadius.Empty();
    CurrentLockedOnTarget = nullptr;

    bIsLockedOnEngaged = false;

    PrimaryComponentTick.SetTickFunctionEnable(false);

    PlayerOwner->GetController()->ResetIgnoreLookInput();
    PlayerOwner->GetCharacterMovement()->bOrientRotationToMovement = true;
    PlayerOwner->GetCharacterMovement()->bUseControllerDesiredRotation = false;

    LockOnDistanceCheckTimer.Invalidate();
}

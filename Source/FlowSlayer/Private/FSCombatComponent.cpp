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
        UpdateLockOnCamera(DeltaTime);
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

void UFSCombatComponent::CancelAttack()
{
    AnimInstance->StopAllMontages(0.2f);
    StopCombo();
    equippedWeapon->DeactivateHitbox();
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
        attackToPerform = combo.GetLastAttack()->Montage;

    else
        attackToPerform = combo.GetAttackAt(ComboIndex)->Montage;

    ++ComboIndex;

    return attackToPerform;
}

void UFSCombatComponent::HandleModularComboWindowOpened()
{
    bComboWindowOpened = true;
    //UE_LOG(LogTemp, Error, TEXT("MODULAR Combo Window OPENED (via delegate)!"));
}

void UFSCombatComponent::HandleModularComboWindowClosed()
{
    bComboWindowOpened = false;
    //UE_LOG(LogTemp, Error, TEXT("MODULAR Combo Window CLOSED (via delegate)!"));

    // If we've exceeded the max combo index, we're at the end of the combo chain
    // Reset and let the animation finish naturally (no Montage_Stop)
    if (ComboIndex > OngoingCombo->GetMaxComboIndex())
    {
        //UE_LOG(LogTemp, Warning, TEXT("End of combo reached - Resetting"));
        return;
    }

    else if (!bContinueCombo && AnimInstance)
    {
        //UE_LOG(LogTemp, Warning, TEXT("Player didn't continue MODULAR combo - Letting animation stops"));
        return;
    }

    ContinueCombo();
}

void UFSCombatComponent::HandleFullComboWindowOpened()
{
    //UE_LOG(LogTemp, Error, TEXT("FULL Combo Window OPENED (via delegate)!"));
    bComboWindowOpened = true;
}

void UFSCombatComponent::HandleFullComboWindowClosed()
{
    bComboWindowOpened = false;
    //UE_LOG(LogTemp, Error, TEXT("FULL Combo Window CLOSED (via delegate)!"));

    // If player didn't buffer a continue input, stop the combo early
    if (!bContinueCombo && AnimInstance)
    {
        //UE_LOG(LogTemp, Warning, TEXT("Player didn't continue FULL combo - Stopping"));
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
        //UE_LOG(LogTemp, Error, TEXT("OngoingCombo is null or invalid - Stopping"));
        AnimInstance->Montage_Stop(0.6f, AnimInstance->GetCurrentActiveMontage());
        return;
    }

    if (OngoingCombo->IsFullCombo())
    {
        //UE_LOG(LogTemp, Warning, TEXT("FullCombo detected - Letting animation continue naturally"));
        // Increment ComboIndex to track progress through combo windows
        ++ComboIndex;
        return;
    }

    // MODULAR COMBO: Multiple separate montages - switch to next attack montage
    UAnimMontage* nextAnimAttack{ GetComboNextAttack(*OngoingCombo) };
    if (!nextAnimAttack)
    {
        //UE_LOG(LogTemp, Error, TEXT("nextAnimAttack is null - Stopping"));
        AnimInstance->Montage_Stop(0.6f, AnimInstance->GetCurrentActiveMontage());
        return;
    }

    // Successfully continuing the modular combo - play next attack animation
    //UE_LOG(LogTemp, Warning, TEXT("Continuing modular combo - Playing next attack"));
    PlayerOwner->PlayAnimMontage(nextAnimAttack);
}

void UFSCombatComponent::StopCombo()
{
    ComboIndex = 0;
    bIsAttacking = false;
    bContinueCombo = false;
    bComboWindowOpened = false;
    //OngoingCombo = nullptr;
}

void UFSCombatComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // Safety check: ensure we have a valid ongoing combo
    if (!OngoingCombo || !OngoingCombo->IsValid())
        return;

    // If the montage that ended is the last attack OR if a full combo was interrupted
    if (Montage == OngoingCombo->GetLastAttack()->Montage || (OngoingCombo->IsFullCombo() && bInterrupted))
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

    // NOTE: ComboIndex has already been incremented, so actual ComboIndex to this attack here is ComboIndex - 1
    const FAttackData* currentAttack{ OngoingCombo->GetAttackAt(ComboIndex - 1) };
    if (!currentAttack)
        return;

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

    CameraLookAtRotation.Yaw += DotRight > 0.0f ? -CurrentYawOffset : CurrentYawOffset;
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

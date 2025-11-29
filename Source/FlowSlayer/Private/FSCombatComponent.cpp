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

    AnimInstance->OnMontageEnded.AddDynamic(this, &UFSCombatComponent::OnMontageEnded);
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
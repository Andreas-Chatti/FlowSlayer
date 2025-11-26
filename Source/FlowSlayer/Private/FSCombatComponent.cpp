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
        if (OngoingCombo->Num() == 1 || ComboIndex <= MaxComboIndex)
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
    if (!OngoingCombo)
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

TArray<UAnimMontage*>* UFSCombatComponent::SelectComboBasedOnState(EAttackType attackTypeInput, bool isMoving, bool isFalling)
{
    TArray<UAnimMontage*>* usedCombo{ nullptr };
    if (isMoving && !isFalling)
    {
        switch (attackTypeInput)
        {
        case UFSCombatComponent::EAttackType::Light:
            usedCombo = &RunningLightCombo;
            break;
        case UFSCombatComponent::EAttackType::Heavy:
            usedCombo = &RunningHeavyCombo;
            break;
        }
    }

    else if (!isMoving && !isFalling)
    {
        switch (attackTypeInput)
        {
        case UFSCombatComponent::EAttackType::Light:
            usedCombo = &StandingLightCombo;
            break;
        case UFSCombatComponent::EAttackType::Heavy:
            usedCombo = &StandingHeavyCombo;
            break;
        }
    }

    if (usedCombo)
        MaxComboIndex = usedCombo->Num() - 1;

    return usedCombo;
}

UAnimMontage* UFSCombatComponent::GetComboNextAttack(TArray<UAnimMontage*> combo)
{
    UAnimMontage* attackToPerform{ nullptr };
    if (ComboIndex >= MaxComboIndex) // Si Combo index est à la dernière attaque du combo
        attackToPerform = combo.Last(); // Retourne la dernière attaque du combo (dernier élément)

    else if (combo.IsValidIndex(ComboIndex) && combo[ComboIndex])
        attackToPerform = combo[ComboIndex];

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
    if (ComboIndex > MaxComboIndex)
    {
        UE_LOG(LogTemp, Warning, TEXT("End of combo reached - Resetting"));
        return;
    }

    // If player didn't buffer a continue input, stop the combo early
    if (!bContinueCombo && AnimInstance)
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

    if (!OngoingCombo)
    {
        UE_LOG(LogTemp, Error, TEXT("OngoingCombo is null - Stopping"));
        AnimInstance->Montage_Stop(0.6f, AnimInstance->GetCurrentActiveMontage());
        return;
    }

    // FULL COMBO DETECTION: If combo array has only 1 montage, it's a FullCombo
    // For FullCombo, we don't switch montages - we just let the current one continue playing
    if (OngoingCombo->Num() == 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("FullCombo detected - Letting animation continue naturally"));
        // Don't call PlayAnimMontage - just let the current montage keep playing
        // Increment ComboIndex to mark progress through the combo windows
        ++ComboIndex;
        return;
    }

    // MODULAR COMBO: Multiple montages - switch to next attack montage
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
    MaxComboIndex = 0;
    bIsAttacking = false;

    UE_LOG(LogTemp, Error, TEXT("COMBO RESET !"));
}

void UFSCombatComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == OngoingCombo->Last() || (OngoingCombo->Num() == 1 && bInterrupted))
        StopCombo();

    else if (OngoingCombo->Num() > 1 && !bInterrupted)
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
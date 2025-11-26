#include "FSCombatComponent.h"

UFSCombatComponent::UFSCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UFSCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    PlayerOwner = Cast<ACharacter>(GetOwner());

    AnimInstance = PlayerOwner->GetMesh()->GetAnimInstance();
    if (AnimInstance)
    {
        AnimInstance->OnMontageEnded.AddDynamic(this, &UFSCombatComponent::OnMontageEnded);
        AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &UFSCombatComponent::AnimNotify_ComboWindow);
        AnimInstance->OnPlayMontageNotifyEnd.AddDynamic(this, &UFSCombatComponent::AnimNotify_AttackEnd);
    }

    InitializeAndAttachWeapon();
    OnHitboxActivated.AddUObject(equippedWeapon, &AFSWeapon::ActivateHitbox);
    OnHitboxDeactivated.AddUObject(equippedWeapon, &AFSWeapon::DeactivateHitbox);
    equippedWeapon->OnEnemyHit.AddUObject(this, &UFSCombatComponent::OnHitLanded);
    equippedWeapon->setDamage(Damage);

    checkf(PlayerOwner && AnimInstance && equippedWeapon, TEXT("Core CombatComponent is NULL"));
}

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
    if (ComboIndex >= MaxComboIndex) // Si Combo index est � la derni�re attaque du combo
    {
        attackToPerform = combo.Last(); // Retourne la derni�re attaque du combo (dernier �l�ment)
        ContinueCombo(); // Increment to block further inputs (ComboIndex > MaxComboIndex)
    }

    else if (combo.IsValidIndex(ComboIndex) && combo[ComboIndex])
    {
        attackToPerform = combo[ComboIndex];
        ContinueCombo();
    }

    return attackToPerform;
}

void UFSCombatComponent::ContinueCombo()
{
    ++ComboIndex;
    UE_LOG(LogTemp, Error, TEXT("Combo Index incremented : %d"), ComboIndex);
}

void UFSCombatComponent::ResetCombo()
{
    ComboIndex = 0;
    MaxComboIndex = 0;
    bIsAttacking = false;

    UE_LOG(LogTemp, Error, TEXT("COMBO RESET !"));
}

void UFSCombatComponent::AnimNotify_ComboWindow(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
    UE_LOG(LogTemp, Error, TEXT("Combo Window OPENED !"));

    bComboWindowOpened = true;
}

void UFSCombatComponent::AnimNotify_AttackEnd(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
    bComboWindowOpened = false;
    UE_LOG(LogTemp, Error, TEXT("Combo Window CLOSED !"));

    // Si on a dépassé le dernier index, on est à la fin du combo
    // On reset mais on laisse l'animation se terminer naturellement (pas de Montage_Stop)
    if (ComboIndex > MaxComboIndex)
    {
        UE_LOG(LogTemp, Warning, TEXT("End of combo reached - Resetting"));
        ResetCombo();
        return;
    }

    // Si le joueur n'a pas demandé de continuer le combo, on arrête
    if (!bContinueCombo && AnimInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player didn't continue combo - Stopping"));
        ResetCombo();
        AnimInstance->Montage_Stop(0.6f, AnimInstance->GetCurrentActiveMontage());
        return;
    }

    // Le joueur veut continuer le combo
    if (bContinueCombo)
    {
        bContinueCombo = false;

        if (!OngoingCombo)
        {
            UE_LOG(LogTemp, Error, TEXT("OngoingCombo is null - Stopping"));
            ResetCombo();
            if (AnimInstance)
                AnimInstance->Montage_Stop(0.6f, AnimInstance->GetCurrentActiveMontage());
            return;
        }

        UAnimMontage* nextAnimAttack{ GetComboNextAttack(*OngoingCombo) };
        if (!nextAnimAttack)
        {
            UE_LOG(LogTemp, Error, TEXT("nextAnimAttack is null - Stopping"));
            ResetCombo();
            if (AnimInstance)
                AnimInstance->Montage_Stop(0.6f, AnimInstance->GetCurrentActiveMontage());
            return;
        }

        UE_LOG(LogTemp, Warning, TEXT("Continuing combo - Playing next attack"));
        PlayerOwner->PlayAnimMontage(nextAnimAttack);
    }
}

void UFSCombatComponent::Attack(EAttackType attackTypeInput, bool isMoving, bool isFalling)
{
    if (bIsAttacking && !bComboWindowOpened)
        return;

    else if (bIsAttacking && bComboWindowOpened)
    {
        // Block input if we've gone past the last attack
        if (ComboIndex > MaxComboIndex)
            return;

        bComboWindowOpened = false;
        bContinueCombo = true;
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

void UFSCombatComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{

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
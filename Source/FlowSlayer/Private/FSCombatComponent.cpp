#include "FSCombatComponent.h"

UFSCombatComponent::UFSCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UFSCombatComponent::BeginPlay()
{
    Super::BeginPlay();

    PlayerOwner = Cast<ACharacter>(GetOwner());

    UAnimInstance* AnimInstance{ PlayerOwner->GetMesh()->GetAnimInstance() };
    if (AnimInstance)
        AnimInstance->OnMontageEnded.AddDynamic(this, &UFSCombatComponent::OnAttackMontageEnded);

    InitializeAndAttachWeapon();
    checkf(equippedWeapon, TEXT("FATAL: EquippedWeapon is NULL or INVALID !"));

    OnHitboxActivated.AddUObject(equippedWeapon, &AFSWeapon::ActivateHitbox);
    OnHitboxDeactivated.AddUObject(equippedWeapon, &AFSWeapon::DeactivateHitbox);
    equippedWeapon->OnEnemyHit.AddUObject(this, &UFSCombatComponent::OnHitLanded);
    equippedWeapon->setDamage(Damage);
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
    if (hitParticles)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            hitParticles,
            location
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

void UFSCombatComponent::ApplyHitFlash(AActor* hitActor)
{
    if (!hitActor)
        return;

    USkeletalMeshComponent* enemyMesh{ hitActor->FindComponentByClass<USkeletalMeshComponent>() };
    if (!enemyMesh)
        return;

    UMaterialInterface* ogMat{ enemyMesh->GetMaterial(0) };
    enemyMesh->SetMaterial(0, HitFlashMaterial);

    FTimerHandle flashTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        flashTimerHandle,
        [this, enemyMesh, ogMat]() { enemyMesh->SetMaterial(0, ogMat); },
        hitFlashDuration,
        false
    );
}

void UFSCombatComponent::Attack(bool isMoving, bool isFalling)
{
    if (bIsAttacking)
        return;

    bIsAttacking = true;

    if (RunningAttackMontage && isMoving && !isFalling)
        PlayerOwner->PlayAnimMontage(RunningAttackMontage);

    else if (IdleAttackMontage && !isMoving && !isFalling)
        PlayerOwner->PlayAnimMontage(IdleAttackMontage);

    else
        bIsAttacking = false;
}

void UFSCombatComponent::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == IdleAttackMontage || Montage == RunningAttackMontage)
        bIsAttacking = false;
}
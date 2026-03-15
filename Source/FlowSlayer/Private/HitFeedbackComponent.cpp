#include "HitFeedbackComponent.h"

UHitFeedbackComponent::UHitFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHitFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();

    OwnerCharacter = Cast<ACharacter>(GetOwner());

    if (HitFlashMaterial)
        HitFlashMaterial->GetMaterial()->SetScalarParameterValueEditorOnly("Speed", HitFlashSpeed);
}

void UHitFeedbackComponent::OnLandHit(const FVector& hitLocation)
{
    ApplyHitstop();
    ApplyHitShake(LandedShakeAmplitude);
    SpawnHitVFX(hitLocation);
    PlayHitSound(hitLocation);
    ApplyCameraShake();
}

void UHitFeedbackComponent::OnReceiveHit(const FVector& attackerLocation, float knockbackForce, float upKnockbackForce)
{
    ApplyKnockback(attackerLocation, knockbackForce, upKnockbackForce);
    ApplyHitstop();
    ApplyHitShake(ReceiveShakeAmplitude);
    ApplyHitFlash();
}

void UHitFeedbackComponent::ApplyKnockback(const FVector& attackerLocation, float knockbackForce, float upKnockbackForce)
{
    if (!OwnerCharacter)
        return;

    FVector ownerLocation{ OwnerCharacter->GetActorLocation() };
    FVector knockbackDirection{ (ownerLocation - attackerLocation).GetSafeNormal() };
    FVector knockbackVelocity{ knockbackDirection * knockbackForce + FVector(0.f, 0.f, upKnockbackForce) };

    OwnerCharacter->LaunchCharacter(knockbackVelocity, true, true);
}

void UHitFeedbackComponent::ApplyHitstop()
{
    if (!OwnerCharacter)
        return;

    OwnerCharacter->CustomTimeDilation = HitstopTimeDilation;

    TWeakObjectPtr<ACharacter> weakOwner{ OwnerCharacter };
    FTimerHandle hitstopTimer;
    GetWorld()->GetTimerManager().SetTimer(
        hitstopTimer,
        [weakOwner]()
        {
            if (weakOwner.IsValid())
                weakOwner->CustomTimeDilation = 1.f;
        },
        HitstopDuration,
        false
    );
}

void UHitFeedbackComponent::ApplyHitShake(float shakeAmplitude)
{
    if (!OwnerCharacter)
        return;

    USkeletalMeshComponent* ownerMesh{ OwnerCharacter->GetMesh() };
    if (!ownerMesh)
        return;

    float speed{ 1.f / ShakeSpeed };
    FVector defaultRelativeLoc{ ownerMesh->GetRelativeLocation() };
    float offsetDirection{ 1.f };

    TWeakObjectPtr<USkeletalMeshComponent> weakMesh{ ownerMesh };
    GetWorld()->GetTimerManager().SetTimer(
        HitShakeTimer,
        [this, weakMesh, defaultRelativeLoc, offsetDirection, shakeAmplitude]() mutable
        {
            if (!weakMesh.IsValid())
                return;

            FVector cameraRight{ GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetActorRightVector() };
            FVector cameraRightLocal{ UKismetMathLibrary::InverseTransformDirection(GetOwner()->GetActorTransform(), cameraRight) };
            cameraRightLocal.Z = 0.f;
            cameraRightLocal *= shakeAmplitude * offsetDirection;

            weakMesh->SetRelativeLocation(defaultRelativeLoc + cameraRightLocal, false, nullptr, ETeleportType::TeleportPhysics);

            offsetDirection *= -1.f;
        },
        speed,
        true
    );

    FTimerHandle hitShakeStopTimer;
    GetWorld()->GetTimerManager().SetTimer(
        hitShakeStopTimer,
        [this, weakMesh, defaultRelativeLoc]()
        {
            GetWorld()->GetTimerManager().ClearTimer(HitShakeTimer);

            if (weakMesh.IsValid())
                weakMesh->SetRelativeLocation(defaultRelativeLoc);
        },
        HitstopDuration,
        false
    );
}

void UHitFeedbackComponent::ApplyHitFlash()
{
    if (!OwnerCharacter || !HitFlashMaterial)
        return;

    USkeletalMeshComponent* ownerMesh{ OwnerCharacter->GetMesh() };
    if (!ownerMesh)
        return;

    ownerMesh->SetOverlayMaterial(HitFlashMaterial);

    TWeakObjectPtr<USkeletalMeshComponent> weakMesh{ ownerMesh };
    FTimerHandle hitFlashTimer;
    GetWorld()->GetTimerManager().SetTimer(
        hitFlashTimer,
        [weakMesh]() { if (weakMesh.IsValid()) weakMesh->SetOverlayMaterial(nullptr); },
        HitstopDuration,
        false
    );
}

void UHitFeedbackComponent::SpawnHitVFX(const FVector& location)
{
    if (HitParticlesSystemArray.IsEmpty())
        return;

    uint32 randIndex{ static_cast<uint32>(FMath::RandRange(0, HitParticlesSystemArray.Num() - 1)) };
    if (!HitParticlesSystemArray.IsValidIndex(randIndex))
        return;

    UNiagaraSystem* hitParticlesSystem{ HitParticlesSystemArray[randIndex] };
    if (hitParticlesSystem)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            hitParticlesSystem,
            location,
            FRotator::ZeroRotator,
            FVector(1.0f),
            true,
            true,
            ENCPoolMethod::None
        );
    }
}

void UHitFeedbackComponent::PlayHitSound(const FVector& location)
{
    if (!HitSound)
        return;

    UGameplayStatics::PlaySoundAtLocation(
        GetWorld(),
        HitSound,
        location
    );
}

void UHitFeedbackComponent::ApplyCameraShake()
{
    if (!HitCameraShake)
        return;

    APlayerController* pc{ UGameplayStatics::GetPlayerController(GetWorld(), 0) };
    if (pc)
        pc->ClientStartCameraShake(HitCameraShake);
}

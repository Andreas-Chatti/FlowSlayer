#include "DashComponent.h"

UDashComponent::UDashComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UDashComponent::BeginPlay()
{
    Super::BeginPlay();

    OwningPlayer = Cast<ACharacter>(GetOwner());
    checkf(OwningPlayer, TEXT("FATAL: Owner is invalid or NULL !"));

    checkf(DashCurve, TEXT("FATAL: DashCurve is invalid or NULL !"));

    InitDashVFXComp();
}

bool UDashComponent::CanDash() const
{
    if (CanAffordDash.IsBound() && !CanAffordDash.Execute(FlowCost))
        return false;

    return !bIsDashing && !bIsOnCooldown && !OwningPlayer->GetCharacterMovement()->IsFalling() && !bIsAttacking;
}

void UDashComponent::OnAttackingStarted()
{
    bIsAttacking = true;
}

void UDashComponent::OnAttackingEnded()
{
    bIsAttacking = false;
}

void UDashComponent::StartDash(const FVector2D& inputDirection)
{
    if (!OwningPlayer || !DashCurve || !CanDash() || inputDirection.IsNearlyZero())
        return;

    // Snap input to 8 directions
    float inputAngle{ static_cast<float>(FMath::Atan2(inputDirection.Y, inputDirection.X)) };
    float snappedAngle{ FMath::RoundToFloat(inputAngle / HALF_PI * 4.f) * HALF_PI / 4.f };

    // Rebuild the snapped direction in 2D
    FVector2D snappedInput{ FMath::Cos(snappedAngle), FMath::Sin(snappedAngle) };

    // Convert to world direction relative to camera
    FRotator cameraYaw{ 0.f, OwningPlayer->GetControlRotation().Yaw, 0.f };
    FVector forward{ FRotationMatrix(cameraYaw).GetUnitAxis(EAxis::X) };
    FVector right{ FRotationMatrix(cameraYaw).GetUnitAxis(EAxis::Y) };

    FVector dashDirection{ (forward * snappedInput.Y + right * snappedInput.X).GetSafeNormal() };

    dashStart = OwningPlayer->GetActorLocation();
    dashEnd = dashStart + dashDirection * DashDistance;

    dashElapsed = 0.f;
    lastCurveValue = 0.f;
    bIsDashing = true;

    OnDashStarted.Broadcast(FlowCost);

    if (AfterImagesVFXComp)
        AfterImagesVFXComp->Activate();

    UGameplayStatics::PlaySoundAtLocation(
        GetWorld(),
        DashSound,
        OwningPlayer->GetActorLocation()
    );

    SetComponentTickEnabled(true);
}

void UDashComponent::TickComponent(float deltaTime, ELevelTick tickType,
    FActorComponentTickFunction* thisTickFunction)
{
    Super::TickComponent(deltaTime, tickType, thisTickFunction);

    if (!bIsDashing)
        return;

    dashElapsed += deltaTime;

    // Alpha = normalized time progression between 0 and 1
    float alpha{ FMath::Clamp(dashElapsed / DashDuration, 0.f, 1.f) };

    // The curve transforms linear time into custom movement
    float curveValue{ DashCurve->GetFloatValue(alpha) };

    // Delta displacement — only move the difference since last frame
    // Avoids error accumulation and allows proper collision handling
    float deltaCurve{ curveValue - lastCurveValue };
    FVector deltaMove{ (dashEnd - dashStart) * deltaCurve };
    lastCurveValue = curveValue;

    FHitResult hitResult;
    OwningPlayer->GetCharacterMovement()->SafeMoveUpdatedComponent(deltaMove, OwningPlayer->GetActorRotation(), true, hitResult);

    if (alpha >= 1.f)
        endDash();
}

void UDashComponent::InitDashVFXComp()
{
    if (!DashVFX)
        return;

    AfterImagesVFXComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
        DashVFX,
        OwningPlayer->GetMesh(),
        NAME_None,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        EAttachLocation::SnapToTarget,
        false,
        false
    );
}

void UDashComponent::endDash()
{
    bIsDashing = false;
    lastCurveValue = 0.f;
    SetComponentTickEnabled(false);

    OnDashEnded.Broadcast();

    if (AfterImagesVFXComp)
        AfterImagesVFXComp->Deactivate();

    // Cooldown
    bIsOnCooldown = true;
    GetWorld()->GetTimerManager().SetTimer(
        cooldownTimer,
        [this]() { bIsOnCooldown = false; },
        DashCooldown,
        false
    );
}
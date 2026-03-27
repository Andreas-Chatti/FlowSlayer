#include "DashComponent.h"

UDashComponent::UDashComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UDashComponent::BeginPlay()
{
    Super::BeginPlay();

    OwningPlayer = Cast<ACharacter>(GetOwner());
    checkf(OwningPlayer, TEXT("FATAL: Owner is invalid or NULL !"));
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

    if (bIsDashing)
        EndDash();
}

void UDashComponent::OnAttackingEnded()
{
    bIsAttacking = false;
}

void UDashComponent::StartDash(const FVector2D& inputDirection)
{
    if (!OwningPlayer || !CanDash() || inputDirection.IsNearlyZero())
        return;
    UE_LOG(LogTemp, Warning, TEXT("Start Dashing"));
    // Snap input to 8 directions
    float inputAngle{ static_cast<float>(FMath::Atan2(inputDirection.Y, inputDirection.X)) };
    float snappedAngle{ FMath::RoundToFloat(inputAngle / HALF_PI * 2.f) * HALF_PI / 2.f };

    // Rebuild the snapped direction in 2D and store it — AnimNotifyState will recompute the world direction at NotifyBegin
    FVector2D snappedInput{ FMath::Cos(snappedAngle), FMath::Sin(snappedAngle) };
    SnappedInput2D = snappedInput;

    // Compute world direction from camera yaw to derive blend weights for the ABP 8D blend space
    FRotator cameraYaw{ 0.f, OwningPlayer->GetControlRotation().Yaw, 0.f };
    FVector camForward{ FRotationMatrix(cameraYaw).GetUnitAxis(EAxis::X) };
    FVector camRight{ FRotationMatrix(cameraYaw).GetUnitAxis(EAxis::Y) };
    FVector dashDirection{ (camForward * snappedInput.Y + camRight * snappedInput.X).GetSafeNormal() };

    // Capture blend weights once so the ABP blend space doesn't drift if the character rotates during the animation
    DashForward = FVector::DotProduct(dashDirection, OwningPlayer->GetActorForwardVector());
    DashLateral = FVector::DotProduct(dashDirection, OwningPlayer->GetActorRightVector());

    bIsDashing = true;

    // Safety fallback — if NotifyEnd never fires (e.g. montage interrupted before NotifyBegin), force-end the dash
    GetWorld()->GetTimerManager().SetTimer(
        SafetyTimer,
        [this]() { if (bIsDashing) EndDash(); },
        MaxDashDuration,
        false
    );

    // Cooldown
    bIsOnCooldown = true;
    GetWorld()->GetTimerManager().SetTimer(
        cooldownTimer,
        [this]() { bIsOnCooldown = false; },
        CooldownDuration,
        false
    );

    OnDashStarted.Broadcast(FlowCost);
}

void UDashComponent::EndDash()
{
    bIsDashing = false;

    OnDashEnded.Broadcast();
}
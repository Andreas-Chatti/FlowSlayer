#include "AnimNotifyState_SafeMoveUpdated.h"

UAnimNotifyState_SafeMoveUpdated::UAnimNotifyState_SafeMoveUpdated()
{
	NotifyColor = FColorList::LimeGreen;
	bShouldFireInEditor = false;
}

void UAnimNotifyState_SafeMoveUpdated::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    OwningCharacter = Cast<ACharacter>(MeshComp->GetOwner());
	if (!OwningCharacter)
		return;

    DashComp = OwningCharacter->FindComponentByClass<UDashComponent>();
    if (!DashComp)
        return;

    if (!MoveCurve)
        verifyf(MoveCurve, TEXT("MoveCurve is NULL or INVALID !"));

    // Recompute world direction from current camera yaw at the moment movement starts
    // Avoids stale direction from the ~15 frames between StartDash() and NotifyBegin()
    FRotator cameraYaw{ 0.f, OwningCharacter->GetControlRotation().Yaw, 0.f };
    FVector forward{ FRotationMatrix(cameraYaw).GetUnitAxis(EAxis::X) };
    FVector right{ FRotationMatrix(cameraYaw).GetUnitAxis(EAxis::Y) };
    FVector2D snappedInput{ DashComp->GetSnappedInput2D() };
    FVector dashDirection{ (forward * snappedInput.Y + right * snappedInput.X).GetSafeNormal() };

    Start = OwningCharacter->GetActorLocation();
    End = Start + dashDirection * DashComp->GetDashDistance();

    TimeElapsed = 0.f;
    LastCurveValue = 0.f;
    NotifyDuration = TotalDuration;
}

void UAnimNotifyState_SafeMoveUpdated::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

    if (!OwningCharacter || !MoveCurve || bDashFinished || !DashComp)
        return;

    TimeElapsed += FrameDeltaTime;

    // Alpha = normalized time progression between 0 and 1
    float alpha{ FMath::Clamp(TimeElapsed / NotifyDuration, 0.f, 1.f) };

    // The curve transforms linear time into custom movement
    float curveValue{ MoveCurve->GetFloatValue(alpha) };

    // Delta displacement � only move the difference since last frame
    // Avoids error accumulation and allows proper collision handling
    float deltaCurve{ curveValue - LastCurveValue };
    FVector deltaMove{ (End - Start) * deltaCurve };
    LastCurveValue = curveValue;

    FHitResult hitResult;
    OwningCharacter->GetCharacterMovement()->SafeMoveUpdatedComponent(deltaMove, OwningCharacter->GetActorRotation(), true, hitResult);

    if (alpha >= 1.f)
        bDashFinished = true;
}

void UAnimNotifyState_SafeMoveUpdated::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (DashComp)
        DashComp->EndDash();

    LastCurveValue = 0.f;
    TimeElapsed = 0.f;
    bDashFinished = false;
    OwningCharacter = nullptr;
    DashComp = nullptr;
}

#include "HitboxComponent.h"

UHitboxComponent::UHitboxComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHitboxComponent::BeginPlay()
{
	Super::BeginPlay();

	OnActiveFrameStarted.BindUObject(this, &UHitboxComponent::HandleActiveFrameStarted);
	OnActiveFrameStopped.BindUObject(this, &UHitboxComponent::HandleActiveFrameStopped);
}

void UHitboxComponent::HandleActiveFrameStarted(const FHitboxProfile* hitboxProfile)
{
    if (OwnerWeapon)
        OwnerWeapon->ActivateTrail();

    if (!hitboxProfile)
        return;

    switch (hitboxProfile->Shape)
    {
    case EHitboxShape::WeaponSweep:
        DetectWeaponSweep(hitboxProfile->SweepRadius);
        break;
    case EHitboxShape::Sphere:
        DetectSphere(hitboxProfile->Range, hitboxProfile->Offset);
        break;
    case EHitboxShape::Cone:
        DetectCone(hitboxProfile->Range, hitboxProfile->ConeHalfAngle, hitboxProfile->Offset);
        break;
    case EHitboxShape::Box:
        DetectBox(hitboxProfile->BoxExtent, hitboxProfile->Range, hitboxProfile->Offset);
        break;
    }
}

void UHitboxComponent::HandleActiveFrameStopped()
{
    ActorsHitThisAttack.Empty();

    if (OwnerWeapon)
        OwnerWeapon->DeactivateTrail();
}

void UHitboxComponent::DetectWeaponSweep(float radius)
{
    if (!OwnerWeapon)
        return;

    FVector start{ OwnerWeapon->GetBaseSocketLocation() };
    FVector end{ OwnerWeapon->GetTipSocketLocation() };

    TArray<TEnumAsByte<EObjectTypeQuery>> objectsType{ EObjectTypeQuery::ObjectTypeQuery3 };
    TArray<AActor*> actorsToIgnore{ GetOwner() };
    EDrawDebugTrace::Type debugTrace{ DebugLines ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None };
    TArray<FHitResult> outHits;

    UKismetSystemLibrary::SphereTraceMultiForObjects(GetWorld(), start, end, radius, objectsType, false, actorsToIgnore, debugTrace, outHits, true,
        FLinearColor::Red, FLinearColor::Green, DebugLinesDuration);

    ProcessHits(outHits);
}

void UHitboxComponent::DetectSphere(float range, const FVector& offset)
{
    const AActor* owner{ GetOwner() };
    FVector worldOffset{ owner->GetActorTransform().TransformVector(offset) };
    FVector center{ owner->GetActorLocation() + worldOffset };

    TArray<TEnumAsByte<EObjectTypeQuery>> objectsType{ EObjectTypeQuery::ObjectTypeQuery3 };
    TArray<AActor*> actorsToIgnore{ GetOwner() };
    EDrawDebugTrace::Type debugTrace{ DebugLines ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None };
    TArray<FHitResult> outHits;

    UKismetSystemLibrary::SphereTraceMultiForObjects(GetWorld(), center, center, range, objectsType, false, actorsToIgnore, debugTrace, outHits, true,
        FLinearColor::Red, FLinearColor::Green, DebugLinesDuration);

    ProcessHits(outHits);
}

void UHitboxComponent::DetectCone(float range, float halfAngleDeg, const FVector& offset)
{
    TArray<TEnumAsByte<EObjectTypeQuery>> objectsType{ EObjectTypeQuery::ObjectTypeQuery3 };
    TArray<AActor*> actorsToIgnore{ GetOwner(), OwnerWeapon };
    TArray<FHitResult> outHits;

    const AActor* owner{ GetOwner() };
    FVector worldOffset{ owner->GetActorTransform().TransformVector(offset) };
    FVector center{ owner->GetActorLocation() + worldOffset };

    UKismetSystemLibrary::SphereTraceMultiForObjects(GetWorld(), center, center, range, objectsType, false, actorsToIgnore, EDrawDebugTrace::None, outHits, true);

    TArray<FHitResult> coneHits;
    const float cosHalfAngle{ FMath::Cos(FMath::DegreesToRadians(halfAngleDeg)) };
    FVector forward{ owner->GetActorForwardVector() };
    for (const FHitResult& hit : outHits)
    {
        if (!hit.GetActor())
            continue;

        FVector dirToTarget{ (hit.GetActor()->GetActorLocation() - center).GetSafeNormal() };
        float dot{ static_cast<float>(FVector::DotProduct(forward, dirToTarget)) };

        if (dot >= cosHalfAngle)
            coneHits.Add(hit);
    }

    if (DebugLines)
    {
        DrawDebugCone(GetWorld(), center, forward, range,
            FMath::DegreesToRadians(halfAngleDeg),
            FMath::DegreesToRadians(halfAngleDeg),
            12, FColor::Yellow, false, DebugLinesDuration);
    }

    ProcessHits(coneHits);
}

void UHitboxComponent::DetectBox(const FVector& extent, float range, const FVector& offset)
{
    AActor* owner{ GetOwner() };
    FVector worldOffset{ owner->GetActorTransform().TransformVector(offset) };
    FVector center{ owner->GetActorLocation() + worldOffset + owner->GetActorForwardVector() * range };
    FRotator rotation{ owner->GetActorRotation() };

    TArray<FHitResult> outHits;
    FCollisionQueryParams queryParams;
    queryParams.AddIgnoredActor(owner);
    queryParams.AddIgnoredActor(OwnerWeapon);

    GetWorld()->SweepMultiByObjectType(outHits, center, center, FQuat(rotation), FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
        FCollisionShape::MakeBox(extent), queryParams);

    if (DebugLines)
        DrawDebugBox(GetWorld(), center, extent, FQuat(rotation), FColor::Blue, false, DebugLinesDuration);

    ProcessHits(outHits);
}

void UHitboxComponent::ProcessHits(const TArray<FHitResult>& hits)
{
    for (const FHitResult& hitResult : hits)
    {
        AActor* hitActor{ hitResult.GetActor() };
        if (!hitActor || ActorsHitThisAttack.Contains(hitActor))
            continue;

        ActorsHitThisAttack.Add(hitActor);

        if (hitActor->Implements<UFSDamageable>())
            OnHit.Broadcast(hitActor, hitResult.ImpactPoint);
    }
}
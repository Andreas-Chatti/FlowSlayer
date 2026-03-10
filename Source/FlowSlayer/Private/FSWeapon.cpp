#include "FSWeapon.h"

AFSWeapon::AFSWeapon()
{
    PrimaryActorTick.bCanEverTick = false;
    InitializeComponents();
}

void AFSWeapon::InitializeComponents()
{
    RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootComp;

    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    checkf(WeaponMesh, TEXT("FATAL: WeaponMesh is NULL or INVALID !"));
    WeaponMesh->SetupAttachment(RootComponent);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    /** Trail VFX */
    SwordTrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SwordTrail"));
    verifyf(SwordTrailComponent, TEXT("WARNING: SwordTrailComponent is NULL or INVALID !"));
    SwordTrailComponent->SetupAttachment(WeaponMesh, "S_WeaponMid");
    SwordTrailComponent->bAutoActivate = false;
}

void AFSWeapon::BeginPlay()
{
    Super::BeginPlay();

    OnActiveFrameStarted.BindUObject(this, &AFSWeapon::HandleActiveFrameStarted);
    OnActiveFrameStopped.BindUObject(this, &AFSWeapon::HandleActiveFrameStopped);

    if (SwordTrailSystem && SwordTrailComponent)
        SwordTrailComponent->SetAsset(SwordTrailSystem);
}

void AFSWeapon::HandleActiveFrameStarted(const FAttackData* attackData)
{
    if (SwordTrailComponent)
        SwordTrailComponent->Activate();

    if (!attackData)
        return;

    switch (attackData->HitboxShape)
    {
    case EHitboxShape::WeaponSweep:
        DetectWeaponSweep(attackData->SweepSphereRadius);
        break;
    case EHitboxShape::Sphere:
        DetectSphere(attackData->HitboxRange, attackData->HitboxOffset);
        break;
    case EHitboxShape::Cone:
        DetectCone(attackData->HitboxRange, attackData->ConeHalfAngle, attackData->HitboxOffset);
        break;
    case EHitboxShape::Box:
        DetectBox(attackData->BoxExtent, attackData->HitboxRange, attackData->HitboxOffset);
        break;
    }
}

void AFSWeapon::HandleActiveFrameStopped()
{
    ActorsHitThisAttack.Empty();

    if (SwordTrailComponent)
        SwordTrailComponent->Deactivate();
}

void AFSWeapon::DetectWeaponSweep(float radius)
{
    FVector start{ WeaponMesh->GetSocketLocation(BaseSocket) };
    FVector end{ WeaponMesh->GetSocketLocation(TipSocket) };

    TArray<TEnumAsByte<EObjectTypeQuery>> objectsType{ EObjectTypeQuery::ObjectTypeQuery3 };
    TArray<AActor*> actorsToIgnore{ GetOwner() };
    EDrawDebugTrace::Type debugTrace{ DebugLines ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None };
    TArray<FHitResult> outHits;

    UKismetSystemLibrary::SphereTraceMultiForObjects(GetWorld(), start, end, radius, objectsType, false, actorsToIgnore, debugTrace, outHits, true);

    ProcessHits(outHits);
}

void AFSWeapon::DetectSphere(float range, const FVector& offset)
{
    const AActor* owner{ GetOwner() };
    FVector worldOffset{ owner->GetActorTransform().TransformVector(offset) };
    FVector center{ owner->GetActorLocation() + worldOffset };

    TArray<TEnumAsByte<EObjectTypeQuery>> objectsType{ EObjectTypeQuery::ObjectTypeQuery3 };
    TArray<AActor*> actorsToIgnore{ GetOwner() };
    EDrawDebugTrace::Type debugTrace{ DebugLines ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None };
    TArray<FHitResult> outHits;

    UKismetSystemLibrary::SphereTraceMultiForObjects(GetWorld(), center, center, range, objectsType, false, actorsToIgnore, debugTrace, outHits, true);

    ProcessHits(outHits);
}

void AFSWeapon::DetectCone(float range, float halfAngleDeg, const FVector& offset)
{
    TArray<TEnumAsByte<EObjectTypeQuery>> objectsType{ EObjectTypeQuery::ObjectTypeQuery3 };
    TArray<AActor*> actorsToIgnore{ GetOwner() };
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
            12, FColor::Yellow, false, 1.f);
    }

    ProcessHits(coneHits);
}

void AFSWeapon::DetectBox(const FVector& extent, float range, const FVector& offset)
{
    AActor* owner{ GetOwner() };
    FVector worldOffset{ owner->GetActorTransform().TransformVector(offset) };
    FVector center{ owner->GetActorLocation() + worldOffset + owner->GetActorForwardVector() * range };
    FRotator rotation{ owner->GetActorRotation() };

    TArray<FHitResult> outHits;
    FCollisionQueryParams queryParams;
    queryParams.AddIgnoredActor(owner);
    queryParams.AddIgnoredActor(this);

    GetWorld()->SweepMultiByObjectType(outHits, center, center, FQuat(rotation), FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
        FCollisionShape::MakeBox(extent), queryParams);

    if (DebugLines)
        DrawDebugBox(GetWorld(), center, extent, FQuat(rotation), FColor::Blue, false, 1.f);

    ProcessHits(outHits);
}

void AFSWeapon::ProcessHits(const TArray<FHitResult>& hits)
{
    for (const FHitResult& hitResult : hits)
    {
        AActor* hitActor{ hitResult.GetActor() };
        if (!hitActor || ActorsHitThisAttack.Contains(hitActor))
            continue;

        ActorsHitThisAttack.Add(hitActor);

        if (hitActor->Implements<UFSDamageable>())
            OnEnemyHit.Broadcast(hitActor, hitResult.ImpactPoint);
    }
}

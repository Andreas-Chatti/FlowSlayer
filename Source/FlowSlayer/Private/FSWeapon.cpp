#include "FSWeapon.h"

AFSWeapon::AFSWeapon()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    InitializeComponents();
}

void AFSWeapon::InitializeComponents()
{
    RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootComp;

    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(RootComponent);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Hitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
    Hitbox->SetupAttachment(RootComponent);
    Hitbox->SetBoxExtent(DEFAULT_HITBOX_TRANSFORM);
    Hitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    /** Trail VFX */
    SwordTrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SwordTrail"));
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

void AFSWeapon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bHitboxActive)
        UpdateDamageHitbox();
}

void AFSWeapon::HandleActiveFrameStarted(float attackRadius)
{
    /*
    bHitboxActive = true;
    SetActorTickEnabled(true);
    PreviousHitboxLocation = Hitbox->GetComponentLocation();
    */

    if (SwordTrailComponent)
        SwordTrailComponent->Activate();

    TriggerActiveFrame(attackRadius);
}

void AFSWeapon::HandleActiveFrameStopped()
{
    bHitboxActive = false;
    SetActorTickEnabled(false);
    ActorsHitThisAttack.Empty();

    if (SwordTrailComponent)
        SwordTrailComponent->Deactivate();
}

void AFSWeapon::UpdateDamageHitbox()
{
    TArray<FHitResult> sweepResults;
    FCollisionQueryParams queryParams;
    queryParams.AddIgnoredActor(GetOwner());
    queryParams.AddIgnoredActor(this);

    FVector currentLocation{ Hitbox->GetComponentLocation() };
    FVector boxExtent{ Hitbox->GetScaledBoxExtent() };
    GetWorld()->SweepMultiByObjectType(
        sweepResults,
        PreviousHitboxLocation,
        currentLocation,
        Hitbox->GetComponentQuat(),
        FCollisionObjectQueryParams::AllObjects,
        FCollisionShape::MakeBox(boxExtent),
        queryParams
    );

    if (DebugLines)
        DrawDebugLine(GetWorld(), PreviousHitboxLocation, currentLocation, FColor::Magenta, false, 2.0f, 0, 2.0f);

    for (const FHitResult& hit : sweepResults)
    {
        AActor* hitActor{ hit.GetActor() };
        if (!hitActor)
            continue;

        else if (ActorsHitThisAttack.Contains(hitActor))
            continue;

        ActorsHitThisAttack.Add(hitActor);

        if (DebugLines)
            DrawDebugSphere(GetWorld(), hit.ImpactPoint, 10.0f, 12, FColor::Red, false, 2.0f);

        if (hitActor->Implements<UFSDamageable>())
            OnEnemyHit.Broadcast(hitActor, hit.ImpactPoint);
    }
    PreviousHitboxLocation = currentLocation;
}

void AFSWeapon::TriggerActiveFrame(float attackRadius)
{
    FVector start{ WeaponMesh->GetSocketLocation(BaseSocket) };
    FVector end{ WeaponMesh->GetSocketLocation(TipSocket) };

    TArray<TEnumAsByte<EObjectTypeQuery>> objectsType{ EObjectTypeQuery::ObjectTypeQuery3 };
    TArray<AActor*> actorsToIgnore{ GetOwner() };
    EDrawDebugTrace::Type debugTrace{ DebugLines ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None };
    TArray<FHitResult> outHits;

    bool bHit{ UKismetSystemLibrary::SphereTraceMultiForObjects(GetWorld(), start, end, attackRadius, objectsType, false, actorsToIgnore, debugTrace, outHits, true) };
    
    if (!bHit)
        return;

    for (const FHitResult& hitResult : outHits)
    {
        AActor* hitActor{ hitResult.GetActor() };
        if (!hitActor)
            continue;

        else if (ActorsHitThisAttack.Contains(hitActor))
            continue;

        ActorsHitThisAttack.Add(hitActor);

        if (hitActor->Implements<UFSDamageable>())
            OnEnemyHit.Broadcast(hitActor, hitResult.ImpactPoint);
    }
}

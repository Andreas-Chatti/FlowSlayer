#include "FSWeapon.h"

AFSWeapon::AFSWeapon()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    initializeComponents();
}

void AFSWeapon::initializeComponents()
{
    rootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = rootComp;

    weaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    weaponMesh->SetupAttachment(RootComponent);
    weaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    hitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
    hitbox->SetupAttachment(RootComponent);
    hitbox->SetBoxExtent(DEFAULT_HITBOX_TRANSFORM);
    hitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    /** Trail VFX */
    SwordTrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SwordTrail"));
    SwordTrailComponent->SetupAttachment(weaponMesh, "S_WeaponTip");
    SwordTrailComponent->bAutoActivate = false;
}

void AFSWeapon::BeginPlay()
{
    Super::BeginPlay();

    if (SwordTrailSystem && SwordTrailComponent)
        SwordTrailComponent->SetAsset(SwordTrailSystem);
}

void AFSWeapon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bHitboxActive)
        UpdateDamageHitbox();
}

void AFSWeapon::ActivateHitbox()
{
    bHitboxActive = true;
    SetActorTickEnabled(true);
    previousHitboxLocation = hitbox->GetComponentLocation();

    if (SwordTrailComponent)
        SwordTrailComponent->Activate();
}

void AFSWeapon::DeactivateHitbox()
{
    bHitboxActive = false;
    SetActorTickEnabled(false);
    actorsHitThisAttack.Empty();

    if (SwordTrailComponent)
        SwordTrailComponent->Deactivate();
}

void AFSWeapon::UpdateDamageHitbox()
{
    TArray<FHitResult> sweepResults;
    FCollisionQueryParams queryParams;
    queryParams.AddIgnoredActor(GetOwner());
    queryParams.AddIgnoredActor(this);

    FVector currentLocation{ hitbox->GetComponentLocation() };
    FVector boxExtent{ hitbox->GetScaledBoxExtent() };
    GetWorld()->SweepMultiByObjectType(
        sweepResults,
        previousHitboxLocation,
        currentLocation,
        hitbox->GetComponentQuat(),
        FCollisionObjectQueryParams::AllObjects,
        FCollisionShape::MakeBox(boxExtent),
        queryParams
    );

    //DrawDebugLine(GetWorld(), previousHitboxLocation, currentLocation, FColor::Magenta, false, 2.0f, 0, 2.0f);

    for (const FHitResult& hit : sweepResults)
    {
        AActor* hitActor{ hit.GetActor() };
        if (!hitActor)
            continue;

        else if (actorsHitThisAttack.Contains(hitActor))
            continue;

        actorsHitThisAttack.Add(hitActor);

        //UE_LOG(LogTemp, Warning, TEXT("⚔️ HIT: %s"), *hitActor->GetName());
        //DrawDebugSphere(GetWorld(), hit.ImpactPoint, 10.0f, 12, FColor::Red, false, 2.0f);

        if (hitActor->Implements<UFSDamageable>())
            OnEnemyHit.Broadcast(hitActor, hit.ImpactPoint);
    }
    previousHitboxLocation = currentLocation;
}

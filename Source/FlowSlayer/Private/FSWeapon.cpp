#include "FSWeapon.h"

AFSWeapon::AFSWeapon()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    initializeComponents();
}

void AFSWeapon::setDamage(float damage)
{
    Damage = damage;
}

void AFSWeapon::ActivateHitbox()
{
    bHitboxActive = true;
    SetActorTickEnabled(true);
    previousHitboxLocation = hitbox->GetComponentLocation();

    //UE_LOG(LogTemp, Warning, TEXT("⚔️ Hitbox ACTIVATED - Continuous tracing started"));
}

void AFSWeapon::DeactivateHitbox()
{
    bHitboxActive = false;
    SetActorTickEnabled(false);
    actorsHitThisAttack.Empty();

    //int32 numHit{ actorsHitThisAttack.Num() };
    //UE_LOG(LogTemp, Warning, TEXT("⚔️ Hitbox DEACTIVATED - %d enemies hit total"), numHit);
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

        if (IFSDamageable * damageableActor{ Cast<IFSDamageable>(hitActor) })
            damageableActor->ReceiveDamage(Damage, this);
    }
    previousHitboxLocation = currentLocation;
}

void AFSWeapon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bHitboxActive)
        UpdateDamageHitbox();
}

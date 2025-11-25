#include "FSProjectile.h"

AFSProjectile::AFSProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    // Collision sphere
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    CollisionComponent->InitSphereRadius(15.0f);
    CollisionComponent->SetCollisionObjectType(ECC_GameTraceChannel1);
    CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
    CollisionComponent->OnComponentHit.AddDynamic(this, &AFSProjectile::OnHit);
    RootComponent = CollisionComponent;

    // Mesh visuel
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComponent->SetupAttachment(RootComponent);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Projectile movement
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
    ProjectileMovement->InitialSpeed = 1000.0f;
    ProjectileMovement->MaxSpeed = 1000.0f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->ProjectileGravityScale = 0.0f;

    TrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailComponent"));
    TrailComponent->SetupAttachment(MeshComponent);
    TrailComponent->bAutoActivate = true;
    TrailComponent->OnSystemFinished.AddDynamic(this, &AFSProjectile::OnTrailSystemFinished);

    InitialLifeSpan = Lifetime;
}

void AFSProjectile::BeginPlay()
{
    Super::BeginPlay();

    Owner = GetOwner();
    if (Owner)
        CollisionComponent->IgnoreActorWhenMoving(Owner, true);

    if (trailParticules && TrailComponent)
        TrailComponent->SetAsset(trailParticules);
}

void AFSProjectile::FireInDirection(const FVector& ShootDirection)
{
    ProjectileMovement->Velocity = ShootDirection * ProjectileMovement->InitialSpeed;
}

AFSProjectile* AFSProjectile::SpawnProjectile(UWorld* world, AActor* owner, TSubclassOf<AFSProjectile> projectileClass,
    FVector spawnLocation, FRotator spawnRotation)
{
    if (!world || !owner || !projectileClass)
        return nullptr;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = owner;
    SpawnParams.Instigator = owner->GetInstigator();

    AFSProjectile* projectile{ world->SpawnActor<AFSProjectile>(
        projectileClass,
        spawnLocation,
        spawnRotation,
        SpawnParams
    ) };

    return projectile;
}

void AFSProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    //DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.0f, 12, FColor::Red, false, 2.0f);

    if (!OtherActor || OtherActor == Owner)
        return;

    IFSDamageable* damageable{ Cast<IFSDamageable>(OtherActor) };
    bool isPlayer{ OtherActor->ActorHasTag("Player") };
    if (damageable && isPlayer)
        damageable->ReceiveDamage(Damage, Owner);

    SpawnHitVFX(Hit.ImpactPoint);

    if (!TrailComponent)
        Destroy();

    TrailComponent->Deactivate();
    CollisionComponent->DestroyComponent();
}

void AFSProjectile::OnTrailSystemFinished(UNiagaraComponent* PSystem)
{
    Destroy();
}

void AFSProjectile::SpawnHitVFX(const FVector& location)
{
    if (hitParticlesSystemArray.IsEmpty())
        return;

    uint32 randIndex{ static_cast<uint32>(FMath::RandRange(0, hitParticlesSystemArray.Num() - 1)) };
    if (!hitParticlesSystemArray.IsValidIndex(randIndex))
        return;

    UNiagaraSystem* hitParticulesSystem{ hitParticlesSystemArray[randIndex] };
    if (hitParticulesSystem)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            hitParticulesSystem,
            location,
            FRotator::ZeroRotator,
            FVector(1.0f),
            true,
            true,
            ENCPoolMethod::None
        );
    }
}
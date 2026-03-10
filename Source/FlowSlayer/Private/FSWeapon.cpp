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

    if (SwordTrailSystem && SwordTrailComponent)
        SwordTrailComponent->SetAsset(SwordTrailSystem);
}

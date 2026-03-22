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

    TrailNiagaraBaseComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraTrailBase"));
    verifyf(TrailNiagaraBaseComponent, TEXT("WARNING: TrailNiagaraBaseComponent is NULL or INVALID !"));
    TrailNiagaraBaseComponent->SetupAttachment(WeaponMesh, "S_WeaponBase");
    TrailNiagaraBaseComponent->bAutoActivate = false;

    TrailNiagaraTipComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraTrailTip"));
    verifyf(TrailNiagaraTipComponent, TEXT("WARNING: TrailNiagaraTipComponent is NULL or INVALID !"));
    TrailNiagaraTipComponent->SetupAttachment(WeaponMesh, "S_WeaponTip");
    TrailNiagaraTipComponent->bAutoActivate = false;
}

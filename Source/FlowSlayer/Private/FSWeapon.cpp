#include "FSWeapon.h"

AFSWeapon::AFSWeapon()
{
    PrimaryActorTick.bCanEverTick = false;
    InitializeComponents();
}

void AFSWeapon::EquipPart(EWeaponPartType PartType, const FWeaponPartData& PartData)
{
    // Undo the previous tier's stat contribution before applying the new tier
    if (FWeaponPartData* previous{ EquippedPartDataCache.Find(PartType) })
        ApplyPartStat(BuildReversePart(*previous));

    ApplyPartStat(PartData);
    EquippedPartTiers.Add(PartType, PartData.Tier);
    EquippedPartDataCache.Add(PartType, PartData);

    UE_LOG(LogTemp, Log, TEXT("[FSWeapon] Equipped part '%s' T%d — DamageMultiplier: %.3f"), *PartData.PartID.ToString(), PartData.Tier, WeaponDamageMultiplier);
}

int32 AFSWeapon::GetCurrentTier(EWeaponPartType PartType) const
{
    const int32* tier{ EquippedPartTiers.Find(PartType) };
    return tier ? *tier : 0;
}

void AFSWeapon::ApplyPartStat(const FWeaponPartData& PartData)
{
    switch (PartData.Stat)
    {
    case EUpgradeStat::Damage:
        if (PartData.ValueType == EUpgradeValueType::Additive)
            WeaponDamageMultiplier += PartData.Value;
        else
            WeaponDamageMultiplier *= PartData.Value;
        WeaponDamageMultiplier = FMath::Max(0.f, WeaponDamageMultiplier);
        break;

    default:
        // Extension point — add Handle / Gem stat cases here when their stats are defined
        break;
    }
}

FWeaponPartData AFSWeapon::BuildReversePart(const FWeaponPartData& PartData) const
{
    FWeaponPartData reverse{ PartData };

    if (reverse.ValueType == EUpgradeValueType::Additive)
        reverse.Value = -reverse.Value;
    else
        reverse.Value = (reverse.Value != 0.f) ? (1.f / reverse.Value) : 1.f;

    return reverse;
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

#include "FSEnemy_Grunt.h"

AFSEnemy_Grunt::AFSEnemy_Grunt()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

    DamageHitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageHitbox"));
    DamageHitbox->SetupAttachment(RootComponent);
    DamageHitbox->SetBoxExtent(FVector{ 50.f, 50.f, 50.f });
    DamageHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFSEnemy_Grunt::BeginPlay()
{
	Super::BeginPlay();

    OnHitboxActivated.AddUObject(this, &AFSEnemy_Grunt::ActivateDamageHitbox);
    OnHitboxDeactivated.AddUObject(this, &AFSEnemy_Grunt::DeactivateDamageHitbox);
}

void AFSEnemy_Grunt::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDamageHitboxActive && !bIsDead)
		UpdateDamageHitbox();
}

void AFSEnemy_Grunt::Attack_Implementation()
{
    Super::Attack_Implementation();

	UE_LOG(LogTemp, Warning, TEXT("[GRUNT SPECIFIC] Slow heavy swing!"));

	// TODO: Comportement unique au Grunt
	// - Animation spéciale du Grunt
	// - Son spécifique
	// - Effets visuels uniques
	// - Logique d'attaque différente (par ex: AOE, knockback, etc.)
}

void AFSEnemy_Grunt::ActivateDamageHitbox()
{
	bIsDamageHitboxActive = true;
	ActorsHitThisAttack.Empty();
	PreviousHitboxLocation = DamageHitbox->GetComponentLocation();

	UE_LOG(LogTemp, Warning, TEXT("⚔️ Hitbox ACTIVATED"));
}

void AFSEnemy_Grunt::DeactivateDamageHitbox()
{
	bIsDamageHitboxActive = false;
	ActorsHitThisAttack.Empty();
}

void AFSEnemy_Grunt::UpdateDamageHitbox()
{
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());
    QueryParams.AddIgnoredActor(this);

    FVector CurrentLocation{ DamageHitbox->GetComponentLocation() };
    FVector BoxExtent{ DamageHitbox->GetScaledBoxExtent() };
    TArray<FHitResult> SweepResults;

    GetWorld()->SweepMultiByObjectType(
        SweepResults,
        PreviousHitboxLocation,
        CurrentLocation,
        DamageHitbox->GetComponentQuat(),
        FCollisionObjectQueryParams::AllObjects,
        FCollisionShape::MakeBox(BoxExtent),
        QueryParams
    );

    for (const FHitResult& Hit : SweepResults)
    {
        AActor* hitActor{ Hit.GetActor() };
        if (!hitActor)
            continue;

        if (ActorsHitThisAttack.Contains(hitActor))
            continue;

        ActorsHitThisAttack.Add(hitActor);
        DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.0f, 12, FColor::Red, false, 2.0f);

        IFSDamageable* damageableActor{ Cast<IFSDamageable>(hitActor) };
        bool isPlayer{ hitActor->ActorHasTag("Player") };
        if (damageableActor && isPlayer)
            damageableActor->ReceiveDamage(Damage, this);
    }
    PreviousHitboxLocation = CurrentLocation;
}
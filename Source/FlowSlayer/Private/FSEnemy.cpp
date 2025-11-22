#include "../Public/FSEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"

AFSEnemy::AFSEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    CurrentHealth = MaxHealth;

    DamageHitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageHitbox"));
    DamageHitbox->SetupAttachment(RootComponent);
    DamageHitbox->SetBoxExtent(FVector{ 50.f, 50.f, 50.f });
    DamageHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    GetCharacterMovement()->bOrientRotationToMovement = true;
}

void AFSEnemy::BeginPlay()
{
    Super::BeginPlay();

    Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    OnHitboxActivated.AddUObject(this, &AFSEnemy::ActivateDamageHitbox);
    OnHitboxDeactivated.AddUObject(this, &AFSEnemy::DeactivateDamageHitbox);

    UAnimInstance* AnimInstance{ GetMesh()->GetAnimInstance() };
    if (AnimInstance)
        AnimInstance->OnMontageEnded.AddDynamic(this, &AFSEnemy::OnAttackMontageEnded);
}

void AFSEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsDamageHitboxActive)
        UpdateDamageHitbox();
}

void AFSEnemy::ReceiveDamage(float DamageAmount, AActor* DamageDealer)
{
    if (bIsDead)
        return;

    CurrentHealth -= DamageAmount;
    UE_LOG(LogTemp, Warning, TEXT("[%s] Received %.1f damage from %s - Health: %.1f/%.1f"),
        *GetName(), DamageAmount, *DamageDealer->GetName(), CurrentHealth, MaxHealth);

    if (CurrentHealth <= 0.f)
        Die();
}

bool AFSEnemy::IsDead() const
{
    return bIsDead;
}

void AFSEnemy::Attack_Implementation()
{
    bIsAttacking = true;

    UE_LOG(LogTemp, Warning, TEXT("[%s] Generic Attack: Dealing %.1f damage"), *GetName(), Damage);

    // TODO: Logique commune à tous les ennemis
    // - Animation d'attaque par défaut
    // - Son d'attaque
    // - Application des dégâts

    if (Player && AttackMontage)
        PlayAnimMontage(AttackMontage);
}

void AFSEnemy::Die()
{
    bIsDead = true;
    UE_LOG(LogTemp, Warning, TEXT("[%s] DIED - Awarded %d XP"), *GetName(), XPReward);

    // Disable collision and movement
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCharacterMovement()->DisableMovement();

    // TODO: Play death animation
    // TODO: Award XP to player
    // TODO: Spawn loot/pickups

    SetLifeSpan(destroyDelay);
}

void AFSEnemy::ActivateDamageHitbox()
{
    bIsDamageHitboxActive = true;
    ActorsHitThisAttack.Empty();
    PreviousHitboxLocation = DamageHitbox->GetComponentLocation();

    UE_LOG(LogTemp, Warning, TEXT("⚔️ Hitbox ACTIVATED"));
}

void AFSEnemy::UpdateDamageHitbox()
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
        AActor* HitActor = Hit.GetActor();
        if (!HitActor)
            continue;

        if (ActorsHitThisAttack.Contains(HitActor))
            continue;

        ActorsHitThisAttack.Add(HitActor);
        DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.0f, 12, FColor::Red, false, 2.0f);

        if (IFSDamageable * DamageableActor{ Cast<IFSDamageable>(HitActor) })
            DamageableActor->ReceiveDamage(Damage, this);
    }
    PreviousHitboxLocation = CurrentLocation;
}

void AFSEnemy::DeactivateDamageHitbox()
{
    bIsDamageHitboxActive = false;
    ActorsHitThisAttack.Empty();
}

void AFSEnemy::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == AttackMontage)
        bIsAttacking = false;
}

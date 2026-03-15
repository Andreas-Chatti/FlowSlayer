#include "../Public/FSEnemy.h"

AFSEnemy::AFSEnemy()
{
    PrimaryActorTick.bCanEverTick = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // Lock On UI setup
    LockOnWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnWidget"));
    LockOnWidget->SetupAttachment(GetMesh());
    LockOnWidget->SetWidgetSpace(EWidgetSpace::Screen);
    LockOnWidget->SetDrawSize(FVector2D(40.0f, 40.0f));
    LockOnWidget->SetVisibility(false);

    HitboxComponent = CreateDefaultSubobject<UHitboxComponent>(TEXT("HitboxComponent"));
    checkf(HitboxComponent, TEXT("FATAL: HitboxComponent is NULL or INVALID !"));
    HitboxComponent->OnHitboxHitLanded.BindUObject(this, &AFSEnemy::HandleOnHitLanded);

    HitFeedbackComponent = CreateDefaultSubobject<UHitFeedbackComponent>(TEXT("HitFeedbackComponent"));
    checkf(HitFeedbackComponent, TEXT("FATAL: HitFeedbackComponent is NULL or INVALID !"));

    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
    checkf(HealthComponent, TEXT("FATAL: HealthComponent is NULL or INVALID !"));
    HealthComponent->OnDeath.BindUObject(this, &AFSEnemy::HandleOnDeath);
    HealthComponent->OnDamageReceived.AddUniqueDynamic(this, &AFSEnemy::HandleOnDamageReceived);

    OnHitReceived.AddUniqueDynamic(this, &AFSEnemy::HandleOnHitReceived);
}

void AFSEnemy::BeginPlay()
{
    Super::BeginPlay();

    Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    UAnimInstance* AnimInstance{ GetMesh()->GetAnimInstance() };
    if (AnimInstance)
        AnimInstance->OnMontageEnded.AddDynamic(this, &AFSEnemy::OnAttackMontageEnded);

    // Ignoring Player's camera collision to avoid weird camera snap
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

bool AFSEnemy::IsStunned() const
{
    return bIsStunned;
}

void AFSEnemy::Attack_Implementation()
{
    bIsAttacking = true;

    if (Player && MainAttack.Montage)
        PlayAnimMontage(MainAttack.Montage);
}

void AFSEnemy::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == MainAttack.Montage)
        bIsAttacking = false;

    else if (Montage == HitMontage)
        bIsStunned = false;
}

void AFSEnemy::PlayDeathMontage()
{
    if (!DeathMontage)
        return;

    PlayAnimMontage(DeathMontage);
    
    float MontageLength{ DeathMontage->GetPlayLength() };
    float BlendOutTime{ DeathMontage->BlendOut.GetBlendTime() };
    float TimerDelay{ MontageLength - BlendOutTime };
    
    FTimerHandle DeathTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        DeathTimerHandle,
        [this]()
        {
            USkeletalMeshComponent* Mesh{ GetMesh() };
            if (Mesh && Mesh->GetAnimInstance())
                Mesh->bPauseAnims = true;
        },
        TimerDelay,
        false
    );
}

void AFSEnemy::DisplayLockedOnWidget(bool bShowWidget)
{
    LockOnWidget->SetVisibility(bShowWidget);
}

void AFSEnemy::DisplayHealthBarWidget(bool bShowWidget)
{
    HealthComponent->DisplayLifeBar(bShowWidget);
}

void AFSEnemy::DisplayAllWidgets(bool bShowWidget)
{
    DisplayLockedOnWidget(bShowWidget);
    DisplayHealthBarWidget(bShowWidget);
}

void AFSEnemy::HandleOnHitLanded(AActor* hitActor, const FVector& hitLocation)
{
    IFSDamageable* hitActorDamageable{ Cast<IFSDamageable>(hitActor) };
    if (!hitActor || !hitActorDamageable || (hitActorDamageable && hitActorDamageable->GetHealthComponent()->IsDead()))
        return;

    HitFeedbackComponent->OnLandHit(hitLocation);

    hitActorDamageable->NotifyHitReceived(this, MainAttack);
}

void AFSEnemy::HandleOnHitReceived(AActor* instigatorActor, const FAttackData& usedAttack)
{
    HitFeedbackComponent->OnReceiveHit(instigatorActor->GetActorLocation(), usedAttack.KnockbackForce, usedAttack.KnockbackUpForce);
    HealthComponent->ReceiveDamage(usedAttack.Damage, instigatorActor);

    if (usedAttack.AttackContext == EAttackDataContext::Air && (GetCharacterMovement()->IsFalling() || GetCharacterMovement()->IsFlying()))
        StartAirStall(usedAttack.ComboWindowDuration);
}

void AFSEnemy::NotifyHitReceived(AActor* instigator, const FAttackData& usedAttack)
{
    OnHitReceived.Broadcast(instigator, usedAttack);
}

void AFSEnemy::HandleOnDamageReceived(AActor* damageInstigator, float damageAmount, float currentHealth, float maxHealth)
{
    if (HitMontage && !bIsCcImune)
    {
        PlayAnimMontage(HitMontage);
        bIsStunned = true;
        bIsCcImune = true;
    
        TWeakObjectPtr<AFSEnemy> WeakThis{ this };
        FTimerHandle ccImuneTimer;
        GetWorld()->GetTimerManager().SetTimer(
            ccImuneTimer,
            [WeakThis]()
            {
                if (WeakThis.IsValid())
                    WeakThis->bIsCcImune = false;
            },
            CcImuneDelay,
            false
        );
    }
}

void AFSEnemy::HandleOnFSProjectileHit(AActor* hitActor, const FVector& hitLocation)
{
    if (hitActor->IsA<AFSEnemy>())
        return;

    HandleOnHitLanded(hitActor, hitLocation);
}

void AFSEnemy::HandleOnDeath()
{
    GetCapsuleComponent()->SetCollisionProfileName("Ragdoll");
    GetCharacterMovement()->DisableMovement();

    PlayDeathMontage();

    // TODO: Award XP to player
    // TODO: Spawn loot/pickups

    OnEnemyDeath.Broadcast(this);

    SetLifeSpan(destroyDelay);
}

void AFSEnemy::StartAirStall(float airStallDuration)
{
    TWeakObjectPtr movementComp{ MakeWeakObjectPtr(GetCharacterMovement()) };
    movementComp->SetMovementMode(EMovementMode::MOVE_Flying);

    GetWorld()->GetTimerManager().SetTimer(
        AirStallTimer,
        [movementComp]()
        {
            if (movementComp.IsValid())
                movementComp->SetMovementMode(EMovementMode::MOVE_Falling);
        },
        airStallDuration,
        false
    );
}
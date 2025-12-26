#include "../Public/FSEnemy.h"

AFSEnemy::AFSEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // Lock On UI setup
    LockOnWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("LockOnWidget"));
    LockOnWidget->SetupAttachment(GetMesh());
    LockOnWidget->SetWidgetSpace(EWidgetSpace::Screen);
    LockOnWidget->SetDrawSize(FVector2D(40.0f, 40.0f));
    LockOnWidget->SetVisibility(false);

    // Life bar UI setup
    LifeBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("LifeBarWidget"));
    LifeBarWidget->SetupAttachment(GetMesh());
    LifeBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
    LifeBarWidget->SetDrawSize(FVector2D(40.0f, 40.0f));
    LifeBarWidget->SetVisibility(false);
}

void AFSEnemy::InitializeLifeBarWidgetRef()
{
    if (!LifeBarWidget)
        return;

    UUserWidget* Widget{ LifeBarWidget->GetUserWidgetObject() };
    if (!Widget)
        return;

    // Set the OwningEnemy property on the widget
    FObjectProperty* Prop{ FindFProperty<FObjectProperty>(Widget->GetClass(), TEXT("OwningEnemy")) };
    if (!Prop)
        return;

    void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Widget);
    Prop->SetObjectPropertyValue(ValuePtr, this);
}

void AFSEnemy::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;

    Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    UAnimInstance* AnimInstance{ GetMesh()->GetAnimInstance() };
    if (AnimInstance)
        AnimInstance->OnMontageEnded.AddDynamic(this, &AFSEnemy::OnAttackMontageEnded);

    InitializeLifeBarWidgetRef();
}

void AFSEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AFSEnemy::ReceiveDamage(float DamageAmount, AActor* DamageDealer)
{
    if (bIsDead)
        return;

    CurrentHealth -= DamageAmount;
    LifeBarWidget->SetVisibility(true);
    //UE_LOG(LogTemp, Warning, TEXT("[%s] Received %.1f damage from %s - Health: %.1f/%.1f"),
      //  *GetName(), DamageAmount, *DamageDealer->GetName(), CurrentHealth, MaxHealth);

    if (CurrentHealth <= 0.f)
        Die();

    else if (HitMontage && !bIsCcImune)
    {
        PlayAnimMontage(HitMontage);
        bIsStunned = true;
        bIsCcImune = true;

        FTimerHandle ccImuneTimer;
        GetWorld()->GetTimerManager().SetTimer(ccImuneTimer, [this]() {bIsCcImune = false;}, CcImuneDelay, false);
    }
}

bool AFSEnemy::IsDead() const
{
    return bIsDead;
}

bool AFSEnemy::IsStunned() const
{
    return bIsStunned;
}

void AFSEnemy::Attack_Implementation()
{
    bIsAttacking = true;

    if (Player && AttackMontage)
        PlayAnimMontage(AttackMontage);
}

void AFSEnemy::Die()
{
    bIsDead = true;
    //UE_LOG(LogTemp, Warning, TEXT("[%s] DIED - Awarded %d XP"), *GetName(), XPReward);

    GetCapsuleComponent()->SetCollisionProfileName("Ragdoll");
    GetCharacterMovement()->DisableMovement();
    LifeBarWidget->SetVisibility(false);

    PlayDeathMontage();

    // TODO: Award XP to player
    // TODO: Spawn loot/pickups

    SetLifeSpan(destroyDelay);
}

void AFSEnemy::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == AttackMontage)
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
    LifeBarWidget->SetVisibility(bShowWidget);
}

void AFSEnemy::DisplayAllWidgets(bool bShowWidget)
{
    LockOnWidget->SetVisibility(bShowWidget);
    LifeBarWidget->SetVisibility(bShowWidget);
}

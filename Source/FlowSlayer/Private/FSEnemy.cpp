#include "../Public/FSEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"

AFSEnemy::AFSEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    GetCharacterMovement()->bOrientRotationToMovement = true;
}

void AFSEnemy::BeginPlay()
{
    Super::BeginPlay();

    CurrentHealth = MaxHealth;

    Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

    UAnimInstance* AnimInstance{ GetMesh()->GetAnimInstance() };
    if (AnimInstance)
        AnimInstance->OnMontageEnded.AddDynamic(this, &AFSEnemy::OnAttackMontageEnded);
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

    PlayDeathMontage();

    // TODO: Award XP to player
    // TODO: Spawn loot/pickups

    SetLifeSpan(destroyDelay);
}

void AFSEnemy::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == AttackMontage)
        bIsAttacking = false;
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

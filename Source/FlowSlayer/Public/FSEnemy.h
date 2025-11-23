#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "FSDamageable.h"
#include "Components/BoxComponent.h"
#include "FSEnemy.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnHitboxActivated)
DECLARE_MULTICAST_DELEGATE(FOnHitboxDeactivated)
DECLARE_MULTICAST_DELEGATE(FOnProjectileSpawned)

UCLASS(Abstract)
class FLOWSLAYER_API AFSEnemy : public ACharacter, public IFSDamageable
{
    GENERATED_BODY()

public:

    AFSEnemy();

    virtual bool IsDead() const override;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
    void Attack();

    float getAttackRange() const { return AttackRange; }
    bool IsAttacking() const { return bIsAttacking; }

    void setIsAttacking(bool isAttacking) { bIsAttacking = isAttacking; }

    FOnHitboxActivated OnHitboxActivated;
    FOnHitboxDeactivated OnHitboxDeactivated;
    FOnProjectileSpawned OnProjectileSpawned;

protected:

    virtual void BeginPlay() override;

    virtual void Tick(float DeltaTime) override;

    virtual void ReceiveDamage(float DamageAmount, AActor* DamageDealer) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
    float MaxHealth{ 100.f };

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    float CurrentHealth;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
    float Damage{ 10.f };

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
    int32 XPReward{ 10 };

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
    float AttackRange{ 150.f };

    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void Die();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
    UAnimMontage* AttackMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
    UAnimMontage* DeathMontage;

    bool bIsDead{ false };
    bool bIsAttacking{ false };

    /** Player Reference */
    UPROPERTY()
    APawn* Player;

private:

    static constexpr float destroyDelay{ 5.f };

    UFUNCTION()
    void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    void PlayDeathMontage();
};

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "FSDamageable.h"
#include "FSFocusable.h"
#include "HitFeedbackComponent.h"
#include "HitboxComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "Components/WidgetComponent.h"
#include "FSEnemy.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnProjectileSpawned);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnemyDeath, AFSEnemy* enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReceiveDamage, float, level);

UCLASS(Abstract)
class FLOWSLAYER_API AFSEnemy : public ACharacter, public IFSDamageable, public IFSFocusable
{
    GENERATED_BODY()

public:

    AFSEnemy();

    virtual bool IsDead() const override;

    bool IsStunned() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
    void Attack();

    virtual void DisplayLockedOnWidget(bool bShowWidget) override;

    virtual void DisplayHealthBarWidget(bool bShowWidget) override;

    virtual void DisplayAllWidgets(bool bShowWidget) override;

    virtual float GetCurrentHealth() const override { return CurrentHealth; }

    virtual float GetMaxHealth() const override { return MaxHealth; }

    float GetAttackRange() const { return AttackRange; }
    bool IsAttacking() const { return bIsAttacking; }

    void SetIsAttacking(bool isAttacking) { bIsAttacking = isAttacking; }

    FOnProjectileSpawned OnProjectileSpawned;
    FOnEnemyDeath OnEnemyDeath;

    UPROPERTY(BlueprintAssignable)
    FOnReceiveDamage OnReceiveDamage;

protected:

    virtual void BeginPlay() override;

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

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
    float CcImuneDelay{ 6.f };

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
    float ForwardKnockback{ 50.f };

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
    float UpKnockback{ 100.f };

    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void Die();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
    UAnimMontage* AttackMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animations")
    UAnimMontage* HitMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animations")
    UAnimMontage* DeathMontage;

    bool bIsDead{ false };
    bool bIsAttacking{ false };
    bool bIsStunned{ false };
    bool bIsCcImune{ false };

    /** Player Reference */
    UPROPERTY()
    APawn* Player;


    /** Lock-on ui widget indicator when this enemy is locked-on */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UWidgetComponent* LockOnWidget{ nullptr };

    /** Life bar widget ui */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UWidgetComponent* LifeBarWidget{ nullptr };

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hitbox")
    UHitboxComponent* HitboxComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitFeedback")
    UHitFeedbackComponent* HitFeedbackComponent;

private:

    static constexpr float destroyDelay{ 5.f };

    UFUNCTION()
    void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    void PlayDeathMontage();

    /* Initialize directly the blueprint variable "OwningEnemy" from this method 
    * Used for now because there's no way to directly a direct reference to FSEnemy in widget blueprint
    */
    void InitializeLifeBarWidgetRef();

    /** Called by HitboxComponent (UHitboxComponent)
    * Called upon target hit
    * Applies damage, hitstop, vfx, sfx and cameraShake
    */
    void HandleOnHitLanded(AActor* hitActor, AActor* instigator, const FVector& hitLocation);
};

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "HealthComponent.h"
#include "FSDamageable.h"
#include "FSFocusable.h"
#include "HitFeedbackComponent.h"
#include "HitboxComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "Components/WidgetComponent.h"
#include "CombatData.h"
#include "FSEnemy.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnProjectileSpawned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDeath, AFSEnemy*, enemy);

UCLASS(Abstract)
class FLOWSLAYER_API AFSEnemy : public ACharacter, public IFSDamageable, public IFSFocusable
{
    GENERATED_BODY()

public:

    AFSEnemy();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
    void Attack();

    virtual UHealthComponent* GetHealthComponent() override { return HealthComponent; }

    virtual void DisplayLockedOnWidget(bool bShowWidget) override;

    virtual void DisplayHealthBarWidget(bool bShowWidget) override;

    virtual void DisplayAllWidgets(bool bShowWidget) override;

    float GetAttackRange() const { return AttackRange; }
    bool IsAttacking() const { return bIsAttacking; }
    bool CanAttack() const { return bCanAttack; }

    void SetIsAttacking(bool isAttacking) { bIsAttacking = isAttacking; }

    FOnProjectileSpawned OnProjectileSpawned;

    UPROPERTY(BlueprintAssignable, Category = "Combat")
    FOnEnemyDeath OnEnemyDeath;

    UPROPERTY(BlueprintAssignable, Category = "Combat")
    FOnHitReceived OnHitReceived;

protected:

    virtual void BeginPlay() override;

    /* OnDeath (UHealthComponent) handler */
    virtual void HandleOnDeath();

    /** Called by HitboxComponent (UHitboxComponent)
    * Called on successful hit upon a target
    * Applies damage, hitstop, vfx, sfx and cameraShake
    */
    void HandleOnHitLanded(AActor* hitActor, const FVector& hitLocation);

    /** Called by HitboxComponent (UHitboxComponent)
    * Called when getting hit by an actor
    * Applies damage, hitstop, vfx, sfx and cameraShake
    */
    UFUNCTION()
    void HandleOnHitReceived(AActor* instigatorActor, const FAttackData& usedAttack);

    /** Called by HealthComponent (UHealthComponent) 
    * Called when received damage from any source
    */
    UFUNCTION()
    void HandleOnDamageReceived(AActor* instigatorActor, float damageAmount, float currentHealth, float maxHealth);

    /** Called when owning spawned projectile has hit a target */
    void HandleOnFSProjectileHit(AActor* hitActor, const FVector& hitLocation);

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
    int32 XPReward{ 10 };

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
    float AttackRange{ 150.f };

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attack")
    FAttackData MainAttack;

    bool bIsAttacking{ false };

    bool bCanAttack{ true };

    /** Player Reference */
    UPROPERTY()
    APawn* Player;


    /** Lock-on ui widget indicator when this enemy is locked-on */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UWidgetComponent* LockOnWidget{ nullptr };

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hitbox")
    UHitboxComponent* HitboxComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitFeedback")
    UHitFeedbackComponent* HitFeedbackComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
    UHealthComponent* HealthComponent;


private:

    static constexpr float destroyDelay{ 5.f };

    UFUNCTION()
    void HandleOnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    UFUNCTION()
    virtual void NotifyHitReceived(AActor* instigatorActor, const FAttackData& usedAttack) override;

    // === AIRSTALL ===

    UPROPERTY()
    FTimerHandle AirStallTimer;

    /** Activates airstall for this instance 
    * Flying movement mode is activated for airStallDuration
    * then setting back up to falling mode
    */
    void StartAirStall(float airStallDuration);
};

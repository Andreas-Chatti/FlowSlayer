#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "FSDamageable.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "FSWeapon.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEnemyHit, AActor* hitActor, const FVector& hitLocation);

/** Delegates used to activate and deactivate damage hitbox */
DECLARE_MULTICAST_DELEGATE(FOnHitboxActivated);
DECLARE_MULTICAST_DELEGATE(FOnHitboxDeactivated);

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class FLOWSLAYER_API AFSWeapon : public AActor
{
    GENERATED_BODY()

public:

    /** Event delegates notify state
    * Notified during a MELEE attack Animation
    */
    FOnHitboxActivated OnHitboxActivated;
    FOnHitboxDeactivated OnHitboxDeactivated;

    AFSWeapon();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ActivateHitbox();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void DeactivateHitbox();

    /** Delegate called by equippedWeapon (AFSWeapon)
    * When an enemy is hit inside the hitbox
    */
    FOnEnemyHit OnEnemyHit;

protected:

    virtual void Tick(float DeltaTime) override;

    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* rootComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* weaponMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* hitbox;

    /** Sword trail VFX */
    UPROPERTY(EditDefaultsOnly, Category = "VFX")
    UNiagaraSystem* SwordTrailSystem;

    UPROPERTY()
    UNiagaraComponent* SwordTrailComponent{ nullptr };

private:

    // Track actors hit during current attack (reset on deactivate)
    TSet<AActor*> actorsHitThisAttack;

    // Continuous collision detection
    bool bHitboxActive{ false };
    FVector previousHitboxLocation{ FVector::ZeroVector };
    const FVector DEFAULT_HITBOX_TRANSFORM{ 50.0f, 20.0f, 80.0f };

    void initializeComponents();
    void UpdateDamageHitbox();
};
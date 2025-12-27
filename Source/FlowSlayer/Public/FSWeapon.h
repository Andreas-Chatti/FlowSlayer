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

    /** Activate the weapon hitbox and enable collision detection
    * Called via AnimNotify during attack animations
    * Enables tick, initializes sweep starting position, and activates sword trail VFX
    */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ActivateHitbox();

    /** Deactivate the weapon hitbox and disable collision detection
    * Called via AnimNotify at the end of attack animations
    * Disables tick, clears hit actors list, and deactivates sword trail VFX
    */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void DeactivateHitbox();

    /** Delegate called by equippedWeapon (AFSWeapon)
    * When an enemy is hit inside the hitbox
    */
    FOnEnemyHit OnEnemyHit;

protected:

    virtual void Tick(float DeltaTime) override;

    virtual void BeginPlay() override;

    /** Root component for the weapon actor */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootComp;

    /** Weapon mesh component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* WeaponMesh;

    /** Box collision component for damage detection
    * Only active during attack animations when hitbox is enabled
    */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* Hitbox;

    /** Sword trail VFX component
    * Activated/deactivated when hitbox is enabled/disabled
    */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UNiagaraComponent* SwordTrailComponent{ nullptr };

    /** Sword trail VFX system asset */
    UPROPERTY(EditDefaultsOnly, Category = "VFX")
    UNiagaraSystem* SwordTrailSystem;

private:

    /** Track actors hit during current attack to prevent multiple hits
    * Cleared when hitbox is deactivated (end of attack)
    */
    TSet<AActor*> ActorsHitThisAttack;

    /** Is the hitbox currently active for damage detection? */
    bool bHitboxActive{ false };

    /** Previous hitbox location for continuous sweep collision detection */
    FVector PreviousHitboxLocation{ FVector::ZeroVector };

    /** Default hitbox extents (X, Y, Z) */
    const FVector DEFAULT_HITBOX_TRANSFORM{ 50.0f, 20.0f, 80.0f };

    /** Initialize all weapon components in constructor */
    void InitializeComponents();

    /** Perform continuous sweep collision detection between frames
    * Called every tick when hitbox is active
    * Sweeps from PreviousHitboxLocation to current location to detect fast-moving hits
    */
    void UpdateDamageHitbox();
};
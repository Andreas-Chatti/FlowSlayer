#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "FSDamageable.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "FSWeapon.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEnemyHit, AActor* hitActor, const FVector& hitLocation);

/** Delegates used to activate and deactivate damage hitbox */
DECLARE_DELEGATE_OneParam(FOnActiveFrameStarted, float attackRadius);
DECLARE_DELEGATE(FOnActiveFrameStopped);

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
    FOnActiveFrameStarted OnActiveFrameStarted;
    FOnActiveFrameStopped OnActiveFrameStopped;

    AFSWeapon();

    /** Delegate called by equippedWeapon (AFSWeapon)
    * When an enemy is hit inside the hitbox
    */
    FOnEnemyHit OnEnemyHit;

    /** Shows hit debug lines */
    UPROPERTY(EditAnywhere, Category = "Debug")
    bool DebugLines{ false };

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

    /* Socket name of the base weapon where the hitbox starts 
    * Can be empty if there's no socket
    */
    UPROPERTY(EditDefaultsOnly, Category = "Sockets")
    FName BaseSocket{ "S_WeaponBase" };

    /* Socket name of the tip of the weapon where the hitbox ends
    * Can be empty if there's no socket
    */
    UPROPERTY(EditDefaultsOnly, Category = "Sockets")
    FName TipSocket{ "S_WeaponTip" };

    /** Activate the weapon hitbox and enable collision detection
    * Called via AnimNotify during attack animations
    * Enables tick, initializes sweep starting position, and activates sword trail VFX
    */
    void HandleActiveFrameStarted(float attackRadius);

    /** Deactivate the weapon hitbox and disable collision detection
    * Called via AnimNotify at the end of attack animations
    * Disables tick, clears hit actors list, and deactivates sword trail VFX
    */
    void HandleActiveFrameStopped();

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


    void TriggerActiveFrame(float attackRadius);
};
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "FSDamageable.h"
#include "CombatData.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "FSWeapon.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEnemyHit, AActor* hitActor, const FVector& hitLocation);

/** Delegates used to activate and deactivate damage hitbox */
DECLARE_DELEGATE_OneParam(FOnActiveFrameStarted, const FHitboxProfile* hitboxProfile);
DECLARE_DELEGATE(FOnActiveFrameStopped);

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class FLOWSLAYER_API AFSWeapon : public AActor
{
    GENERATED_BODY()

public:

    AFSWeapon();

    /** Event delegates notify state
    * Notified during a MELEE attack Animation
    */
    FOnActiveFrameStarted OnActiveFrameStarted;
    FOnActiveFrameStopped OnActiveFrameStopped;

    /** Delegate called by equippedWeapon (AFSWeapon)
    * When an enemy is hit inside the hitbox
    */
    FOnEnemyHit OnEnemyHit;

    /** Shows hit debug lines */
    UPROPERTY(EditAnywhere, Category = "Debug")
    bool DebugLines{ false };

    UPROPERTY(EditAnywhere, Category = "Debug", meta = (EditCondition = "DebugLines"))
    float DebugLinesDuration{ 5.f };

protected:

    virtual void BeginPlay() override;

    /** Root component for the weapon actor */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootComp;

    /** Weapon mesh component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* WeaponMesh;

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
    void HandleActiveFrameStarted(const FHitboxProfile* hitboxProfile);

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

    /** Initialize all weapon components in constructor */
    void InitializeComponents();

    void DetectWeaponSweep(float radius);
    void DetectSphere(float range, const FVector& offset);
    void DetectCone(float range, float halfAngleDeg, const FVector& offset);
    void DetectBox(const FVector& extent, float range, const FVector& offset);

    /** Process and adds valid damageable actors to ActorsHitThisAttack 
    * Prevents targets from being hit multiple times during one attack
    */
    void ProcessHits(const TArray<FHitResult>& hits);
};
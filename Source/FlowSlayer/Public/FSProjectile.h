#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "FSDamageable.h"
#include "FSProjectile.generated.h"

UCLASS()
class FLOWSLAYER_API AFSProjectile : public AActor
{
    GENERATED_BODY()

public:

    AFSProjectile();

    void FireInDirection(const FVector& ShootDirection);

    void SetDamage(float damage) { Damage = damage; }

    static AFSProjectile* SpawnProjectile(UWorld* world, AActor* owner, TSubclassOf<AFSProjectile> projectileClass, 
        FVector spawnLocation, FRotator spawnRotation);

protected:

    virtual void BeginPlay() override;

    // === VFX ===

    /** Trail Particules VFX */
    UPROPERTY(EditDefaultsOnly, Category = "VFX")
    UNiagaraSystem* trailParticules;

    UPROPERTY()
    UNiagaraComponent* TrailComponent;

    /** Hit Particules VFX */
    UPROPERTY(EditDefaultsOnly, Category = "VFX")
    TArray<UNiagaraSystem*> hitParticlesSystemArray;

    void SpawnHitVFX(const FVector& location);

private:

    UPROPERTY(VisibleAnywhere, Category = "Components")
    USphereComponent* CollisionComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(EditDefaultsOnly, Category = "Projectile")
    float Lifetime{ 5.0f };

    /** Projectile Damage
    * Is directly related and equal to Owner's damage
    */
    float Damage{ 0.f };

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, 
        const FHitResult& Hit);

    UFUNCTION()
    void OnTrailSystemFinished(UNiagaraComponent* PSystem);
};
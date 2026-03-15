#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "FSDamageable.h"
#include "HitFeedbackComponent.h"
#include "FSProjectile.generated.h"

/* Delegate executed when this projectile hit a valid target 
* Handler (HandleOnFSProjectileHit) is in AFSEnemy
*/
DECLARE_DELEGATE_TwoParams(FOnFSProjectileHit, AActor* hitActor, const FVector& hitLocation);

UCLASS()
class FLOWSLAYER_API AFSProjectile : public AActor
{
    GENERATED_BODY()

public:

    AFSProjectile();

    static AFSProjectile* SpawnProjectile(UWorld* world, AActor* owner, AActor* target, TSubclassOf<AFSProjectile> projectileClass, 
        FVector spawnLocation, FRotator spawnRotation);

    /* Delegate executed when this projectile hit a valid target
    * Handler (HandleOnFSProjectileHit) is in AFSEnemy
    */
    FOnFSProjectileHit OnFSProjectileHit;

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

    UPROPERTY(EditDefaultsOnly, Category = "Debug")
    bool DebugLines{ false };

    void SpawnHitVFX(const FVector& location);

private:

    void FireInDirection(const FVector& ShootDirection);

    UPROPERTY(VisibleAnywhere, Category = "Components")
    USphereComponent* CollisionComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(EditDefaultsOnly, Category = "Projectile")
    float Lifetime{ 5.0f };

    UFUNCTION()
    void HandleOnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, 
        const FHitResult& Hit);

    UFUNCTION()
    void OnTrailSystemFinished(UNiagaraComponent* PSystem);
};
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FSWeapon.h"
#include "HitboxComponent.generated.h"

UENUM(BlueprintType)
enum class EHitboxShape : uint8
{
    WeaponSweep,
    Sphere,
    Cone,
    Box
};

USTRUCT(BlueprintType)
struct FHitboxProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    EHitboxShape Shape{ EHitboxShape::WeaponSweep };

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", EditCondition = "Shape == EHitboxShape::WeaponSweep"))
    float SweepRadius{ 10.f };

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", EditCondition = "Shape != EHitboxShape::WeaponSweep"))
    float Range{ 200.f };

    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "180.0", EditCondition = "Shape == EHitboxShape::Cone"))
    float ConeHalfAngle{ 45.f };

    UPROPERTY(EditAnywhere, meta = (EditCondition = "Shape == EHitboxShape::Box"))
    FVector BoxExtent{ 100.f, 100.f, 50.f };

    UPROPERTY(EditAnywhere, meta = (EditCondition = "Shape != EHitboxShape::WeaponSweep"))
    FVector Offset{ FVector::ZeroVector };
};

DECLARE_DELEGATE_TwoParams(FOnHitboxHitLanded, AActor* hitActor, const FVector& hitLocation);

/** Delegates used to activate and deactivate damage hitbox */
DECLARE_DELEGATE_OneParam(FOnActiveFrameStarted, const FHitboxProfile* hitboxProfile);
DECLARE_DELEGATE(FOnActiveFrameStopped);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FLOWSLAYER_API UHitboxComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UHitboxComponent();

    /** Event delegates notify state
    * Notified during a MELEE attack Animation
    */
    FOnActiveFrameStarted OnActiveFrameStarted;
    FOnActiveFrameStopped OnActiveFrameStopped;

    /** Broadcasted when a target is hit inside the hitbox */
    FOnHitboxHitLanded OnHitboxHitLanded;

    /** Shows hit debug lines */
    UPROPERTY(EditAnywhere, Category = "Debug")
    bool DebugLines{ false };

    UPROPERTY(EditAnywhere, Category = "Debug", meta = (EditCondition = "DebugLines"))
    float DebugLinesDuration{ 5.f };

    void SetOwnerWeaponRef(AFSWeapon* weaponRef) { OwnerWeapon = weaponRef; }

protected:

	virtual void BeginPlay() override;

private:

    /* Owner weapon reference 
    * Can be nullptr if the owner doesn't have any weapon
    * Used mostly to get sockets for sweep detection
    */
    UPROPERTY()
    AFSWeapon* OwnerWeapon{ nullptr };

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

    /** Track actors hit during current attack to prevent multiple hits
    * Cleared when hitbox is deactivated (end of attack)
    */
    TSet<AActor*> ActorsHitThisAttack;

    void DetectWeaponSweep(float radius);
    void DetectSphere(float range, const FVector& offset);
    void DetectCone(float range, float halfAngleDeg, const FVector& offset);
    void DetectBox(const FVector& extent, float range, const FVector& offset);

    /** Process and adds valid damageable actors to ActorsHitThisAttack
    * Prevents targets from being hit multiple times during one attack
    */
    void ProcessHits(const TArray<FHitResult>& hits);
};

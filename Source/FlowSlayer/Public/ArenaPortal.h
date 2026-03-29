#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "GameFramework/Pawn.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "ArenaPortal.generated.h"

/** Broadcasted when the player has been teleported — RunManager listens to this to activate the next arena */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerTeleported);

/**
 * Placed in the level by the designer, hidden by default.
 * RunManager calls ShowPortal() when the current arena is cleared.
 * Teleports the player to DestinationActor on overlap.
 * Notifies RunManager via OnPlayerTeleported — no direct dependency on RunManager.
 */
UCLASS()
class FLOWSLAYER_API AArenaPortal : public AActor
{
	GENERATED_BODY()

public:

	AArenaPortal();

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

	virtual void BeginPlay() override;

	/** Broadcasted after the player is teleported — RunManager binds StartNextArena here */
	UPROPERTY(BlueprintAssignable, Category = "Portal")
	FOnPlayerTeleported OnPlayerTeleported;

	/** Makes the portal visible and activates its overlap. Called by RunManager when the arena is cleared. */
	UFUNCTION(BlueprintCallable, Category = "Portal")
	void ShowPortal();

private:

	/** Visual mesh of the portal */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* MeshComponent;

	/** Niagara VFX component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* NiagaraComponent;

	/** Niagara system to play on the portal */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	UNiagaraSystem* PortalVFX;

	/** Looped Sound Cue to play on the portal */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	USoundCue* PortalSFX;

	/** Audio component for the looped portal SFX — used at runtime via ShowPortal() */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	UAudioComponent* PortalAudioComponent{ nullptr };

#if WITH_EDITORONLY_DATA
	/** Editor-only cached reference to the spawned preview sound — prevents duplicates on recompile */
	UPROPERTY()
	UAudioComponent* EditorPreviewAudioComponent{ nullptr };
#endif

	/** Overlap trigger — player enters this to activate the portal */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* OverlapBox;

	/** Actor placed in the level representing the player arrival point in the next arena */
	UPROPERTY(EditAnywhere, Category = "Portal")
	AActor* DestinationActor;

	/** Called when an actor overlaps the trigger box */
	UFUNCTION()
	void HandleOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	/** Teleports the player to DestinationActor then broadcasts OnPlayerTeleported */
	void TeleportPlayer(APawn* Player);
};

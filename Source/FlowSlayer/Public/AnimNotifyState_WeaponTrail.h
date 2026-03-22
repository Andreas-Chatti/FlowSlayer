#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "FSCombatComponent.h"
#include "AnimNotifyState_WeaponTrail.generated.h"

UCLASS(meta = (DisplayName = "WeaponTrail"))
class FLOWSLAYER_API UAnimNotifyState_WeaponTrail : public UAnimNotifyState
{
	GENERATED_BODY()

public:

    UAnimNotifyState_WeaponTrail();

    /** Sword trail Niagara VFX system asset */
    UPROPERTY(EditAnywhere, Category = "VFX")
    UNiagaraSystem* NiagaraTrailSystem;

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:

    UPROPERTY()
    UFSCombatComponent* CombatCompRef{ nullptr };

    UPROPERTY()
    AFSWeapon* WeaponRef{ nullptr };

    /** Trail component attached at weapon base socket */
    UPROPERTY()
    UNiagaraComponent* TrailNiagaraBaseCompRef{ nullptr };

    /** Trail component attached at weapon tip socket */
    UPROPERTY()
    UNiagaraComponent* TrailNiagaraTipCompRef{ nullptr };
};

#include "AnimNotifyState_WeaponTrail.h"

UAnimNotifyState_WeaponTrail::UAnimNotifyState_WeaponTrail()
{
	NotifyColor = FColor::Magenta;
}

void UAnimNotifyState_WeaponTrail::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	CombatCompRef = MeshComp->GetOwner()->GetComponentByClass<UFSCombatComponent>();
	if (!CombatCompRef)
		return;

	WeaponRef = CombatCompRef->GetEquippedWeapon();
	if (!WeaponRef)
		return;

	TrailNiagaraBaseCompRef = WeaponRef->GetTrailNiagaraBaseComponent();
	TrailNiagaraTipCompRef = WeaponRef->GetTrailNiagaraTipComponent();

	if (TrailNiagaraBaseCompRef && NiagaraTrailSystem)
	{
		TrailNiagaraBaseCompRef->SetAsset(NiagaraTrailSystem);
		TrailNiagaraBaseCompRef->Activate();
	}

	if (TrailNiagaraTipCompRef && NiagaraTrailSystem)
	{
		TrailNiagaraTipCompRef->SetAsset(NiagaraTrailSystem);
		TrailNiagaraTipCompRef->Activate();
	}
}

void UAnimNotifyState_WeaponTrail::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (TrailNiagaraBaseCompRef)
		TrailNiagaraBaseCompRef->Deactivate();

	if (TrailNiagaraTipCompRef)
		TrailNiagaraTipCompRef->Deactivate();
}

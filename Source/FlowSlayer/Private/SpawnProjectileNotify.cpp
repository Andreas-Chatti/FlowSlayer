#include "SpawnProjectileNotify.h"

void USpawnProjectileNotify::Notify(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& EventReference)
{
	if (!meshComp || !meshComp->GetOwner())
		return;

	if (AFSEnemy * enemy{ Cast<AFSEnemy>(meshComp->GetOwner()) })
		enemy->OnProjectileSpawned.Broadcast();
}

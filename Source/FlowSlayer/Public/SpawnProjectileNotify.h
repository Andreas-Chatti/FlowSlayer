#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "FSEnemy.h"
#include "SpawnProjectileNotify.generated.h"

UCLASS()
class FLOWSLAYER_API USpawnProjectileNotify : public UAnimNotify
{
	GENERATED_BODY()
	
public:

	virtual void Notify(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& EventReference) override;
};

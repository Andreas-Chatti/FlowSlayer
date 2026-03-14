#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameFramework/Character.h"
#include "AnimNotify_Launch.generated.h"

UCLASS(meta = (DisplayName = "LaunchOwner"))
class FLOWSLAYER_API UAnimNotify_Launch : public UAnimNotify
{
	GENERATED_BODY()

public:

	UAnimNotify_Launch();
	
protected:

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere)
	float VelocityX{ 0.f };

	UPROPERTY(EditAnywhere)
	float VelocityY{ 0.f };

	UPROPERTY(EditAnywhere)
	float VelocityZ{ 0.f };

	UPROPERTY(EditAnywhere)
	bool bXYOverride{ false };

	UPROPERTY(EditAnywhere)
	bool bZOverride{ false };
};

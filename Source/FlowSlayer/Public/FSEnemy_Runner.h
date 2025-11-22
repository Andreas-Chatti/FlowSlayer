#pragma once
#include "CoreMinimal.h"
#include "FSEnemy.h"
#include "FSEnemy_Runner.generated.h"

UCLASS()
class FLOWSLAYER_API AFSEnemy_Runner : public AFSEnemy
{
	GENERATED_BODY()
	
public:

	AFSEnemy_Runner();

protected:

	virtual void BeginPlay() override;

	// Override Attack pour ajouter comportement spécifique au Runner
	virtual void Attack_Implementation() override;
};


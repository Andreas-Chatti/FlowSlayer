#pragma once
#include "CoreMinimal.h"
#include "FSEnemy.h"
#include "FSEnemy_Grunt.generated.h"

UCLASS()
class FLOWSLAYER_API AFSEnemy_Grunt : public AFSEnemy
{
	GENERATED_BODY()
	
public:

	AFSEnemy_Grunt();

protected:

	void Attack_Implementation() override;
};

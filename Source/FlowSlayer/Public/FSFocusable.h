#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FSFocusable.generated.h"

UINTERFACE(MinimalAPI)
class UFSFocusable : public UInterface
{
	GENERATED_BODY()
};

class FLOWSLAYER_API IFSFocusable
{
	GENERATED_BODY()

public:

	virtual void DisplayAllWidgets(bool bShowWidget) = 0;

	virtual void DisplayLockedOnWidget(bool bShowWidget) = 0;

	virtual void DisplayHealthBarWidget(bool bShowWidget) = 0;
};

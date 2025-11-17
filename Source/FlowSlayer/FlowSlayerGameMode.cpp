// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlowSlayerGameMode.h"
#include "FlowSlayerCharacter.h"
#include "UObject/ConstructorHelpers.h"

AFlowSlayerGameMode::AFlowSlayerGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

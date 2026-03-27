#include "RunManager.h"

ARunManager::ARunManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARunManager::BeginPlay()
{
	Super::BeginPlay();

	PlayerCharacter = Cast<AFlowSlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	StartRun();
}

void ARunManager::StartRun()
{
	if (Arenas.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunManager] No arenas assigned — run cannot start."));
		return;
	}

	CurrentArenaIndex = 0;
	ActivateArena(Arenas[CurrentArenaIndex]);

	UE_LOG(LogTemp, Log, TEXT("[RunManager] Run started. Total arenas: %d"), Arenas.Num());
}

void ARunManager::StartNextArena()
{
	CurrentArenaIndex++;

	if (!Arenas.IsValidIndex(CurrentArenaIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RunManager] StartNextArena called but no arena at index %d."), CurrentArenaIndex);
		return;
	}

	ActivateArena(Arenas[CurrentArenaIndex]);

	UE_LOG(LogTemp, Log, TEXT("[RunManager] Entering arena %d / %d"), CurrentArenaIndex + 1, Arenas.Num());
}

void ARunManager::ActivateArena(AFSArenaManager* Arena)
{
	if (!Arena)
		return;

	Arena->OnArenaCleared.AddUniqueDynamic(this, &ARunManager::HandleOnArenaCleared);

	if (AArenaPortal* portal{ Arena->GetExitPortal() })
		portal->OnPlayerTeleported.AddUniqueDynamic(this, &ARunManager::StartNextArena);

	Arena->StartArena();
}

void ARunManager::HandleOnArenaCleared()
{
	UE_LOG(LogTemp, Log, TEXT("[RunManager] Arena %d cleared."), CurrentArenaIndex + 1);

	if (IsLastArena())
	{
		UE_LOG(LogTemp, Log, TEXT("[RunManager] Run completed!"));
		OnRunCompleted.Broadcast();
		return;
	}

	OnRunArenaCleared.Broadcast();
}

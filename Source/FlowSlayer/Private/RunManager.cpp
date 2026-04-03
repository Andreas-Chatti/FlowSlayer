#include "RunManager.h"

ARunManager::ARunManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

float ARunManager::GetElapsedRunTime() const
{
	if (bRunCompleted)
		return ElapsedRunTime;

	return GetWorld()->GetTimeSeconds() - RunStartTime;
}

#if WITH_EDITOR
void ARunManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	TArray<AActor*> existing;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARunManager::StaticClass(), existing);
	if (existing.Num() > 1)
		UE_LOG(LogTemp, Error, TEXT("[RunManager] More than one RunManager in the level — only one is allowed."));
}
#endif

void ARunManager::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> existing;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARunManager::StaticClass(), existing);
	if (existing.Num() > 1)
	{
		UE_LOG(LogTemp, Error, TEXT("[RunManager] Duplicate detected — destroying self."));
		Destroy();
		return;
	}

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
	RunStartTime = GetWorld()->GetTimeSeconds();
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
		ElapsedRunTime = GetWorld()->GetTimeSeconds() - RunStartTime;
		bRunCompleted = true;
		UE_LOG(LogTemp, Log, TEXT("[RunManager] Run completed in %.2f seconds."), ElapsedRunTime);
		OnRunCompleted.Broadcast();
		return;
	}

	OnRunArenaCleared.Broadcast();
}


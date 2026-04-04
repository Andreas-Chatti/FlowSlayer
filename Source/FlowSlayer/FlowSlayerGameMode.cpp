#include "FlowSlayerGameMode.h"

AFlowSlayerGameMode::AFlowSlayerGameMode()
{
}

void AFlowSlayerGameMode::BeginPlay()
{
	Super::BeginPlay();

	PlayerCharacter = Cast<AFlowSlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	verifyf(PlayerCharacter, TEXT("[GameMode] PlayerCharacter not found in BeginPlay."));
	PlayerCharacter->OnPlayerDeath.AddUniqueDynamic(this, &AFlowSlayerGameMode::HandleOnPlayerDeath);
	PlayerCharacter->GetProgressionComponent()->OnMilestoneLevelUp.AddUniqueDynamic(this, &AFlowSlayerGameMode::HandleOnMilestoneLevelUp);
	PlayerCharacter->GetInputManagerComponent()->OnPauseActionStarted.BindUObject(this, &AFlowSlayerGameMode::HandleOnPlayerPausePressed);

	TArray<AActor*> foundManagers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARunManager::StaticClass(), foundManagers);
	verifyf(!foundManagers.IsEmpty(), TEXT("[GameMode] No RunManager found in BeginPlay."));

	ARunManager* runManager{ Cast<ARunManager>(foundManagers[0]) };
	runManager->OnRunCompleted.AddUniqueDynamic(this, &AFlowSlayerGameMode::HandleOnRunCompleted);

	TArray<AActor*> foundChests;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARewardChest::StaticClass(), foundChests);
	for (AActor* actor : foundChests)
	{
		if (ARewardChest* chest{ Cast<ARewardChest>(actor) })
			chest->OnChestOpened.AddUniqueDynamic(this, &AFlowSlayerGameMode::HandleOnChestOpened);
	}

	TArray<AActor*> foundArenas;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFSArenaManager::StaticClass(), foundArenas);
	for (AActor* actor : foundArenas)
	{
		if (AFSArenaManager* arena{ Cast<AFSArenaManager>(actor) })
			arena->OnEnemySpawned.AddUniqueDynamic(this, &AFlowSlayerGameMode::HandleOnEnemySpawned);
	}
}

bool AFlowSlayerGameMode::IsScreenActive(UUserWidget* WidgetInstance) const
{
	if(WidgetInstance && WidgetInstance->IsVisible())
		return true;

	return false;
}

void AFlowSlayerGameMode::HandleOnMilestoneLevelUp(int32 newLevel)
{
	ShowScreen(UpgradeScreenClass, UpgradeScreenInstance, true);

	// See BP_ThirdPersonGameMode (child BP) for the rest of the logic
	// This is requiered in order to listen to OnUpgradeConfirmed from WBP_UpgradeScreen and initialize UProgressionComp ref
	// When OnUpgradeConfirmed is broadcasted, HideScreen() is called
}

void AFlowSlayerGameMode::HandleOnPlayerDeath(AFlowSlayerCharacter* player)
{
	ShowScreen(DeathScreenClass, DeathScreenInstance);
}

void AFlowSlayerGameMode::HandleOnRunCompleted()
{
	if (PlayerCharacter)
	{
		PlayerCharacter->GetMesh()->GetAnimInstance()->StopAllMontages(0.3f);
		PlayerCharacter->GetInputManagerComponent()->DisableAllInputs();
	}

	ShowScreen(WinScreenClass, WinScreenInstance);
}

void AFlowSlayerGameMode::HandleOnPlayerPausePressed()
{
	if (IsScreenActive(PauseScreenInstance))
		HideScreen(PauseScreenInstance);

	else if (!IsScreenActive(UpgradeScreenInstance) && !IsScreenActive(DeathScreenInstance) && !IsScreenActive(WinScreenInstance))
		ShowScreen(PauseScreenClass, PauseScreenInstance, true);
}

void AFlowSlayerGameMode::HandleOnChestOpened()
{
	ShowScreen(UpgradeScreenClass, UpgradeScreenInstance, true);
}

void AFlowSlayerGameMode::HandleOnEnemySpawned(AFSEnemy* Enemy)
{
	if (!Enemy)
		return;

	Enemy->OnEnemyDeath.AddUniqueDynamic(this, &AFlowSlayerGameMode::HandleOnEnemyDeath);
}

void AFlowSlayerGameMode::HandleOnEnemyDeath(AFSEnemy* Enemy)
{
	if (!Enemy || !PlayerCharacter)
		return;

	UProgressionComponent* progressionComp{ PlayerCharacter->GetProgressionComponent() };

	progressionComp->AddXP(Enemy->GetXPReward());

	if (FMath::FRand() < Enemy->GetWeaponPartDropChance())
		progressionComp->ApplyRandomWeaponPartDrop();
}

void AFlowSlayerGameMode::ShowScreen(TSubclassOf<UUserWidget> WidgetClass, UUserWidget*& WidgetInstance, bool bPauseWorld)
{
	if (!WidgetClass)
		return;

	if (!WidgetInstance)
	{
		WidgetInstance = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
		if (WidgetInstance)
			WidgetInstance->AddToViewport();
	}

	else if (WidgetInstance->Visibility == ESlateVisibility::Collapsed)
		WidgetInstance->SetVisibility(ESlateVisibility::Visible);

	UGameplayStatics::SetGamePaused(GetWorld(), bPauseWorld);

	APlayerController* pc{ UGameplayStatics::GetPlayerController(GetWorld(), 0) };
	if (pc)
	{
		FInputModeGameAndUI inputMode;
		pc->SetInputMode(inputMode);
		pc->SetShowMouseCursor(true);
	}
}

void AFlowSlayerGameMode::HideScreen(UUserWidget* WidgetInstance)
{
	if (!WidgetInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Widget instance given was invalid"));
		return;
	}

	WidgetInstance->SetVisibility(ESlateVisibility::Collapsed);

	if (UGameplayStatics::IsGamePaused(GetWorld()))
		UGameplayStatics::SetGamePaused(GetWorld(), false);

	APlayerController* pc{ UGameplayStatics::GetPlayerController(GetWorld(), 0) };
	if (pc)
	{
		FInputModeGameOnly inputMode;
		pc->SetInputMode(inputMode);
		pc->SetShowMouseCursor(false);
	}
}

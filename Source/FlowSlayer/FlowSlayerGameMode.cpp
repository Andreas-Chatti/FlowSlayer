#include "FlowSlayerGameMode.h"
#include "Kismet/GameplayStatics.h"

AFlowSlayerGameMode::AFlowSlayerGameMode()
{
}

void AFlowSlayerGameMode::BeginPlay()
{
	Super::BeginPlay();

	PlayerCharacter = Cast<AFlowSlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	verifyf(PlayerCharacter, TEXT("[GameMode] PlayerCharacter not found in BeginPlay."));
	PlayerCharacter->OnPlayerDeath.AddUniqueDynamic(this, &AFlowSlayerGameMode::HandleOnPlayerDeath);

	TArray<AActor*> foundManagers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ARunManager::StaticClass(), foundManagers);
	verifyf(!foundManagers.IsEmpty(), TEXT("[GameMode] No RunManager found in BeginPlay."));

	ARunManager* runManager{ Cast<ARunManager>(foundManagers[0]) };
	runManager->OnRunCompleted.AddUniqueDynamic(this, &AFlowSlayerGameMode::HandleOnRunCompleted);
}

void AFlowSlayerGameMode::HandleOnPlayerDeath(AFlowSlayerCharacter* player)
{
	ShowEndScreen(DeathScreenClass, DeathScreenInstance);
}

void AFlowSlayerGameMode::HandleOnRunCompleted()
{
	if (PlayerCharacter)
	{
		PlayerCharacter->GetMesh()->GetAnimInstance()->StopAllMontages(0.3f);
		PlayerCharacter->GetInputManagerComponent()->DisableAllInputs();
	}

	ShowEndScreen(WinScreenClass, WinScreenInstance);
}

void AFlowSlayerGameMode::ShowEndScreen(TSubclassOf<UUserWidget> WidgetClass, UUserWidget*& WidgetInstance)
{
	if (!WidgetClass)
		return;

	WidgetInstance = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
	if (WidgetInstance)
		WidgetInstance->AddToViewport();

	APlayerController* pc{ UGameplayStatics::GetPlayerController(GetWorld(), 0) };
	if (pc)
	{
		FInputModeUIOnly inputMode;
		pc->SetInputMode(inputMode);
		pc->SetShowMouseCursor(true);
	}
}

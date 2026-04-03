#include "RewardChest.h"

ARewardChest::ARewardChest()
{
	PrimaryActorTick.bCanEverTick = false;

	ChestMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestMesh"));
	ChestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = ChestMesh;

	OverlapZone = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapZone"));
	OverlapZone->SetupAttachment(RootComponent);
	OverlapZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OverlapZone->OnComponentBeginOverlap.AddUniqueDynamic(this, &ARewardChest::HandleOnBeginOverlap);
	OverlapZone->OnComponentEndOverlap.AddUniqueDynamic(this, &ARewardChest::HandleOnEndOverlap);

	KeyUIWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("KeyUIWidget"));
	KeyUIWidget->SetupAttachment(RootComponent);
	KeyUIWidget->SetWidgetSpace(EWidgetSpace::Screen);
	KeyUIWidget->SetDrawSize(FVector2D(40.0f, 40.0f));
	KeyUIWidget->SetVisibility(false);
}

void ARewardChest::ShowChest()
{
	ChestMesh->SetVisibility(true);
	ChestMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	OverlapZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void ARewardChest::BeginPlay()
{
	Super::BeginPlay();

	PlayerController = GetWorld()->GetFirstPlayerController();
	verifyf(PlayerController, TEXT("[RewardChest] WARNING: PlayerController is NULL or INVALID !"));

	EnableInput(PlayerController); // Only used in order to create InputComponent implicitely
	if (UEnhancedInputComponent* EnhancedInputComp{ Cast<UEnhancedInputComponent>(InputComponent) })
		EnhancedInputComp->BindAction(InteractAction, ETriggerEvent::Started, this, &ARewardChest::OpenChest);
	DisableInput(PlayerController); // Disabling input by default (only active on overlap)

	PlayerRef = PlayerController->GetPawn();

	ChestMesh->SetVisibility(false, true);
}

void ARewardChest::OpenChest()
{
	OnChestOpened.Broadcast();

	OverlapZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChestMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChestMesh->SetVisibility(false, true);
	DisableInput(PlayerController);
}

void ARewardChest::HandleOnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == PlayerRef && KeyUIWidget)
	{
		KeyUIWidget->SetVisibility(true);
		EnableInput(PlayerController);
	}
}

void ARewardChest::HandleOnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == PlayerRef && KeyUIWidget)
	{
		KeyUIWidget->SetVisibility(false);
		DisableInput(PlayerController);
	}
}

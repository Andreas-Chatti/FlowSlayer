#include "ArenaPortal.h"

AArenaPortal::AArenaPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetVisibility(false);

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	NiagaraComponent->SetupAttachment(RootComponent);
	NiagaraComponent->bAutoActivate = false;

	PortalAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("PortalAudioComponent"));
	PortalAudioComponent->SetupAttachment(RootComponent);
	PortalAudioComponent->bAutoActivate = false;

	OverlapBox = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapBox"));
	OverlapBox->SetupAttachment(RootComponent);
	OverlapBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	OverlapBox->SetCollisionProfileName(TEXT("Trigger"));
	OverlapBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

#if WITH_EDITOR
void AArenaPortal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (GetWorld() && (GetWorld()->WorldType == EWorldType::EditorPreview || GetWorld()->WorldType == EWorldType::Editor))
	{
		if (PortalVFX)
		{
			NiagaraComponent->SetAsset(PortalVFX);
			NiagaraComponent->Activate(true);
		}

		if (PortalSFX && (!EditorPreviewAudioComponent || !EditorPreviewAudioComponent->IsPlaying()))
		{
			if (EditorPreviewAudioComponent)
				EditorPreviewAudioComponent->Stop();

			EditorPreviewAudioComponent = UGameplayStatics::SpawnSoundAtLocation(this, PortalSFX, GetActorLocation());
		}
	}
}
#endif

void AArenaPortal::BeginPlay()
{
	Super::BeginPlay();

	OverlapBox->OnComponentBeginOverlap.AddDynamic(this, &AArenaPortal::HandleOnOverlapBegin);
}

void AArenaPortal::ShowPortal()
{
	MeshComponent->SetVisibility(true);
	OverlapBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	if (PortalVFX)
	{
		NiagaraComponent->SetAsset(PortalVFX);
		NiagaraComponent->Activate();
	}

	if (PortalSFX)
	{
		PortalAudioComponent->SetSound(PortalSFX);
		PortalAudioComponent->Play();
	}
}

void AArenaPortal::HandleOnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* player{ Cast<APawn>(OtherActor) };
	if (!player)
		return;

	TeleportPlayer(player);
}

void AArenaPortal::TeleportPlayer(APawn* Player)
{
	if (!DestinationActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ArenaPortal] TeleportPlayer called but DestinationActor is not set."));
		return;
	}

	Player->SetActorLocation(DestinationActor->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	Player->GetController()->SetControlRotation(DestinationActor->GetActorRotation());

	OnPlayerTeleported.Broadcast();
}

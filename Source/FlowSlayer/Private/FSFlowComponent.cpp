#include "FSFlowComponent.h"

UFSFlowComponent::UFSFlowComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UFSFlowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsDecaying)
		RemoveFlow(DecayRate * DeltaTime);
}

void UFSFlowComponent::OnHitLanded(AActor* actorHit, const FVector& hitLocation, float damageAmount, float flowReward)
{
	AddFlow(flowReward);
}

void UFSFlowComponent::OnFlowChanged(float currentFlow, float maxFlow)
{
	EFlowTier newTier{ GetFlowTier() };
	if (newTier == CurrentTier)
		return;

	FlowTierChanged.Broadcast(newTier, CurrentTier);
}

void UFSFlowComponent::OnFlowTierChanged(EFlowTier newTier, EFlowTier oldTier)
{
	CurrentTier = newTier;
}

void UFSFlowComponent::BeginPlay()
{
	Super::BeginPlay();

	FlowChanged.AddUniqueDynamic(this, &UFSFlowComponent::OnFlowChanged);
	FlowTierChanged.AddUniqueDynamic(this, &UFSFlowComponent::OnFlowTierChanged);
}

void UFSFlowComponent::AddFlow(float amount)
{
	CurrentFlow = FMath::Clamp(CurrentFlow + amount, 0.f, MaxFlow);

	FlowChanged.Broadcast(CurrentFlow, MaxFlow);

	// Stop any active decay and reset the grace period
	bIsDecaying = false;
	GetWorld()->GetTimerManager().ClearTimer(DecayGraceTimer);
	GetWorld()->GetTimerManager().SetTimer(
		DecayGraceTimer,
		[this]() { bIsDecaying = true; },
		DecayGracePeriod,
		false
	);
}

void UFSFlowComponent::RemoveFlow(float amount)
{
	CurrentFlow = FMath::Clamp(CurrentFlow - amount, 0.f, MaxFlow);

	FlowChanged.Broadcast(CurrentFlow, MaxFlow);

	if (CurrentFlow <= 0.f)
		bIsDecaying = false;
}

void UFSFlowComponent::ConsumeFlow(float amount)
{
	RemoveFlow(amount);
}

void UFSFlowComponent::OnPlayerHit(float damageAmount, AActor* damageDealer)
{
	RemoveFlow(damageAmount / 2.f);
}

EFlowTier UFSFlowComponent::GetFlowTier() const
{
	float ratio{ GetFlowRatio() };
	if (ratio < 0.25f)       
		return EFlowTier::None;
	else if (ratio < 0.50f)  
		return EFlowTier::Low;
	else if (ratio < 0.75f)  
		return EFlowTier::Medium;
	else if (ratio < 1.00f)  
		return EFlowTier::High;
	else                     
		return EFlowTier::Max;
}

float UFSFlowComponent::GetFlowRatio() const
{
	return CurrentFlow / MaxFlow;
}


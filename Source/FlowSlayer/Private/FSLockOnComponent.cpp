#include "FSLockOnComponent.h"

UFSLockOnComponent::UFSLockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UFSLockOnComponent::BeginPlay()
{
	Super::BeginPlay();

	PlayerOwner = Cast<ACharacter>(GetOwner());
	checkf(PlayerOwner, TEXT("FSLockOnComponent: Owner is not a Character!"));

	CombatComponent = PlayerOwner->FindComponentByClass<UFSCombatComponent>();
	checkf(CombatComponent, TEXT("FSLockOnComponent: CombatComponent not found on owner!"));
}

void UFSLockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsLockedOnEngaged && CurrentLockedOnTarget)
	{
		UpdateLockOnCamera(DeltaTime);
		LockOnValidCheck();
	}
}

// ==================== Core ====================

void UFSLockOnComponent::LockOnValidCheck()
{
	if (!CurrentLockedOnTarget || !CachedDamageableLockOnTarget)
		return;

	FVector targetLocation{ CurrentLockedOnTarget->GetActorLocation() };
	FVector playerLocation{ PlayerOwner->GetActorLocation() };

	double distanceSq{ FVector::DistSquared(playerLocation, targetLocation) };
	if (distanceSq >= FMath::Square(LockOnDetectionRadius))
		DisengageLockOn();
	else if (CachedDamageableLockOnTarget->GetHealthComponent()->IsDead())
	{
		if (!SwitchToNearestTarget())
			DisengageLockOn();
	}
}

void UFSLockOnComponent::UpdateLockOnCamera(float deltaTime)
{
	UpdatePlayerFacingTarget(deltaTime);
	UpdateCameraFacingTarget(deltaTime);
}

void UFSLockOnComponent::UpdatePlayerFacingTarget(float deltaTime)
{
	FVector PlayerLocation{ PlayerOwner->GetActorLocation() };
	FVector TargetLocation{ CurrentLockedOnTarget->GetActorLocation() };

	FRotator PlayerLookAtRotation{ UKismetMathLibrary::FindLookAtRotation(PlayerLocation, TargetLocation) };
	PlayerLookAtRotation.Pitch = 0.0f;

	FRotator CurrentPlayerRotation{ PlayerOwner->GetActorRotation() };
	FRotator SmoothedPlayerRotation{ FMath::RInterpTo(CurrentPlayerRotation, PlayerLookAtRotation, deltaTime, CameraRotationInterpSpeed) };
	PlayerOwner->SetActorRotation(SmoothedPlayerRotation);
}

void UFSLockOnComponent::UpdateCameraFacingTarget(float deltaTime)
{
	FVector PlayerLocation{ PlayerOwner->GetActorLocation() };
	FVector TargetLocation{ CurrentLockedOnTarget->GetActorLocation() };

	FRotator CameraLookAtRotation{ UKismetMathLibrary::FindLookAtRotation(PlayerLocation, TargetLocation) };
	FVector DirectionToTarget{ (TargetLocation - PlayerLocation).GetSafeNormal() };
	FVector PlayerRight{ PlayerOwner->GetActorRightVector() };
	float DotRight{ static_cast<float>(FVector::DotProduct(DirectionToTarget, PlayerRight)) };

	double Distance{ FVector::Dist(PlayerLocation, TargetLocation) };
	double maxClampDistance{ LockOnDetectionRadius / 2 };
	double DistanceRatio{ FMath::Clamp(Distance / maxClampDistance, 0.0, 1.0) };
	double CurrentYawOffset{ FMath::Lerp(CloseCameraYawOffset, FarCameraYawOffset, DistanceRatio) };
	double CurrentPitchOffset{ FMath::Lerp(CloseCameraPitchOffset, FarCameraPitchOffset, DistanceRatio) };

	// Widen the yaw tolerance during attacks to keep the camera stable
	float dotRightTolerance{ (CombatComponent && CombatComponent->IsAttacking()) ? -0.15f : 0.f };

	CameraLookAtRotation.Yaw += DotRight > dotRightTolerance ? -CurrentYawOffset : CurrentYawOffset;
	CameraLookAtRotation.Pitch += CurrentPitchOffset;
	CameraLookAtRotation.Roll = 0.f;

	FRotator CurrentCameraRotation{ PlayerOwner->GetController()->GetControlRotation() };
	FRotator SmoothedCameraRotation{ FMath::RInterpTo(CurrentCameraRotation, CameraLookAtRotation, deltaTime, CameraRotationInterpSpeed) };
	PlayerOwner->GetController()->SetControlRotation(SmoothedCameraRotation);
}

bool UFSLockOnComponent::EngageLockOn()
{
	if (!PlayerOwner)
		return false;

	TArray<FHitResult> outHits;
	if (!FindTargetsInRadius(outHits))
		return false;

	AController* Controller{ PlayerOwner->GetController() };
	if (!Controller)
		return false;

	FRotator ControlRotation{ Controller->GetControlRotation() };
	FVector CameraForward{ ControlRotation.Vector() };
	CameraForward.Z = 0;
	CameraForward.Normalize();

	CollectValidTargets(outHits);

	if (TargetsInLockOnRadius.IsEmpty())
		return false;

	AActor* BestTarget{ FindBestScoredTarget(CameraForward) };
	if (!BestTarget)
		return false;

	SetCurrentTarget(BestTarget);
	bIsLockedOnEngaged = true;

	CachedFocusableTarget->DisplayAllWidgets(true);

	PrimaryComponentTick.SetTickFunctionEnable(true);
	SetPlayerLockOnMovementMode(true);

	OnLockOnStarted.Execute(CurrentLockedOnTarget);

	return true;
}

void UFSLockOnComponent::SwitchLockOnTarget(float axisValueX)
{
	if (!CurrentLockedOnTarget || GetWorld()->GetTimerManager().IsTimerActive(delaySwitchLockOnTimer))
		return;

	TArray<FHitResult> outHits;
	if (!FindTargetsInRadius(outHits))
		return;

	bool bLookingRight{ axisValueX > 0 };
	AActor* BestTarget{ FindBestTargetInDirection(outHits, bLookingRight) };

	if (!BestTarget)
		return;

	HidePreviousTargetWidgets();

	SetCurrentTarget(BestTarget);
	CachedFocusableTarget->DisplayAllWidgets(true);

	GetWorld()->GetTimerManager().SetTimer(
		delaySwitchLockOnTimer,
		targetSwitchDelay,
		false
	);

	if (CombatComponent)
		CombatComponent->SetLockedOnTargetRef(CurrentLockedOnTarget);
}

void UFSLockOnComponent::DisengageLockOn()
{
	TargetsInLockOnRadius.Empty();

	HidePreviousTargetWidgets();

	SetCurrentTarget(nullptr);
	bIsLockedOnEngaged = false;

	PrimaryComponentTick.SetTickFunctionEnable(false);
	SetPlayerLockOnMovementMode(false);

	// Reset residual Roll accumulated by the lock-on camera interpolation
	FRotator controlRot{ PlayerOwner->GetController()->GetControlRotation() };
	controlRot.Roll = 0.f;
	PlayerOwner->GetController()->SetControlRotation(controlRot);

	OnLockOnStopped.Broadcast();
}

// ==================== Helpers ====================

bool UFSLockOnComponent::IsValidLockOnTarget(AActor* actor) const
{
	if (!actor)
		return false;

	if (!actor->Implements<UFSFocusable>() || !actor->Implements<UFSDamageable>())
		return false;

	IFSDamageable* damageable{ Cast<IFSDamageable>(actor) };
	return damageable && !damageable->GetHealthComponent()->IsDead();
}

void UFSLockOnComponent::SetCurrentTarget(AActor* newTarget)
{
	CurrentLockedOnTarget = newTarget;
	CachedDamageableLockOnTarget = Cast<IFSDamageable>(CurrentLockedOnTarget);
	CachedFocusableTarget = Cast<IFSFocusable>(CurrentLockedOnTarget);
}

void UFSLockOnComponent::SetPlayerLockOnMovementMode(bool bLockOnActive)
{
	if (bLockOnActive)
	{
		PlayerOwner->GetController()->SetIgnoreLookInput(true);
		PlayerOwner->GetCharacterMovement()->bOrientRotationToMovement = false;
		PlayerOwner->GetCharacterMovement()->bUseControllerDesiredRotation = true;
	}
	else
	{
		PlayerOwner->GetController()->ResetIgnoreLookInput();
		PlayerOwner->GetCharacterMovement()->bOrientRotationToMovement = true;
		PlayerOwner->GetCharacterMovement()->bUseControllerDesiredRotation = false;
	}
}

void UFSLockOnComponent::CollectValidTargets(const TArray<FHitResult>& hits)
{
	TSet<AActor*> uniqueHitActors;
	for (const FHitResult& hit : hits)
	{
		AActor* HitActor{ hit.GetActor() };

		if (uniqueHitActors.Contains(HitActor) || TargetsInLockOnRadius.Contains(HitActor))
			continue;

		uniqueHitActors.Add(HitActor);

		if (IsValidLockOnTarget(HitActor))
			TargetsInLockOnRadius.Add(HitActor);
	}
}

AActor* UFSLockOnComponent::SwitchToNearestTarget()
{
	TArray<FHitResult> outHits;
	if (!FindTargetsInRadius(outHits))
		return nullptr;

	FVector PlayerLocation{ PlayerOwner->GetActorLocation() };
	AActor* NearestTarget{ nullptr };
	float SmallestDistance{ FLT_MAX };

	TSet<AActor*> ProcessedActors;
	for (const FHitResult& hit : outHits)
	{
		AActor* HitActor{ hit.GetActor() };

		if (ProcessedActors.Contains(HitActor))
			continue;

		ProcessedActors.Add(HitActor);

		if (HitActor == CurrentLockedOnTarget || !IsValidLockOnTarget(HitActor))
			continue;

		float Distance{ static_cast<float>(FVector::Dist(PlayerLocation, HitActor->GetActorLocation())) };
		if (Distance < SmallestDistance)
		{
			SmallestDistance = Distance;
			NearestTarget = HitActor;
		}
	}

	if (!NearestTarget)
		return nullptr;

	HidePreviousTargetWidgets();

	SetCurrentTarget(NearestTarget);
	CachedFocusableTarget->DisplayAllWidgets(true);

	if (CombatComponent)
		CombatComponent->SetLockedOnTargetRef(CurrentLockedOnTarget);

	return CurrentLockedOnTarget;
}

AActor* UFSLockOnComponent::FindBestScoredTarget(const FVector& cameraForward) const
{
	FVector PlayerLocation{ PlayerOwner->GetActorLocation() };
	AActor* BestTarget{ nullptr };
	float BestScore{ -FLT_MAX };

	for (AActor* TargetActor : TargetsInLockOnRadius)
	{
		FVector ToTarget{ TargetActor->GetActorLocation() - PlayerLocation };
		ToTarget.Z = 0;
		ToTarget.Normalize();

		float DotProduct{ static_cast<float>(FVector::DotProduct(cameraForward, ToTarget)) };
		float Distance{ static_cast<float>(FVector::Dist(PlayerLocation, TargetActor->GetActorLocation())) };
		float Score{ DotProduct * 1000.0f - Distance };

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = TargetActor;
		}
	}

	return BestTarget;
}

AActor* UFSLockOnComponent::FindBestTargetInDirection(const TArray<FHitResult>& hits, bool bLookingRight) const
{
	FRotator ControlRotation{ PlayerOwner->GetController()->GetControlRotation() };
	FVector CameraForward{ ControlRotation.Vector() };
	CameraForward.Z = 0;
	CameraForward.Normalize();

	FVector CameraRight{ FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::Y) };
	CameraRight.Z = 0;
	CameraRight.Normalize();

	FVector PlayerLocation{ PlayerOwner->GetActorLocation() };
	AActor* BestTarget{ nullptr };
	float SmallestAngle{ FLT_MAX };

	TSet<AActor*> ProcessedActors;
	for (const FHitResult& hit : hits)
	{
		AActor* HitActor{ hit.GetActor() };

		if (ProcessedActors.Contains(HitActor))
			continue;

		ProcessedActors.Add(HitActor);

		if (HitActor == CurrentLockedOnTarget || !IsValidLockOnTarget(HitActor))
			continue;

		FVector ToTarget{ HitActor->GetActorLocation() - PlayerLocation };
		ToTarget.Z = 0;
		ToTarget.Normalize();

		float RightDot{ static_cast<float>(FVector::DotProduct(ToTarget, CameraRight)) };
		bool bTargetOnRight{ RightDot > 0 };

		if (bTargetOnRight != bLookingRight)
			continue;

		float ForwardDot{ static_cast<float>(FVector::DotProduct(ToTarget, CameraForward)) };

		if (ForwardDot < 0.3f)
			continue;

		FVector ToCurrentTarget{ CurrentLockedOnTarget->GetActorLocation() - PlayerLocation };
		ToCurrentTarget.Z = 0;
		ToCurrentTarget.Normalize();

		float AngleBetween{ static_cast<float>(FMath::Acos(FVector::DotProduct(ToTarget, ToCurrentTarget))) };

		if (AngleBetween < SmallestAngle)
		{
			SmallestAngle = AngleBetween;
			BestTarget = HitActor;
		}
	}

	return BestTarget;
}

bool UFSLockOnComponent::FindTargetsInRadius(TArray<FHitResult>& outHits)
{
	if (!PlayerOwner)
		return false;

	FVector Start{ PlayerOwner->GetActorLocation() };
	FVector End{ Start };

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	return UKismetSystemLibrary::SphereTraceMultiForObjects(
		GetWorld(),
		Start,
		End,
		LockOnDetectionRadius,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		outHits,
		true
	);
}

void UFSLockOnComponent::HidePreviousTargetWidgets()
{
	if (!CachedDamageableLockOnTarget || !CachedFocusableTarget)
		return;

	UHealthComponent* LockOnTargetHealthComp{ CachedDamageableLockOnTarget->GetHealthComponent() };
	if (!LockOnTargetHealthComp)
		return;

	bool isFullLife{ LockOnTargetHealthComp->GetCurrentHealth() >= LockOnTargetHealthComp->GetMaxHealth() };
	bool isDead{ LockOnTargetHealthComp->IsDead() };

	if (isFullLife || isDead)
		CachedFocusableTarget->DisplayAllWidgets(false);
	else
		CachedFocusableTarget->DisplayLockedOnWidget(false);
}

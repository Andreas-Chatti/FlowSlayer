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
	if (!PlayerOwner)
		UE_LOG(LogTemp, Error, TEXT("FSLockOnComponent: Owner is not a Character!"));
}

void UFSLockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsLockedOnEngaged && CurrentLockedOnTarget)
		UpdateLockOnCamera(DeltaTime);
}

void UFSLockOnComponent::LockOnValidCheck()
{
	if (!CurrentLockedOnTarget || !CachedDamageableLockOnTarget)
		return;

	FVector targetLocation{ CurrentLockedOnTarget->GetActorLocation() };
	FVector playerLocation{ PlayerOwner->GetActorLocation() };

	double distanceSq{ FVector::DistSquared(playerLocation, targetLocation) };
	if (distanceSq >= FMath::Square(LockOnDetectionRadius) || CachedDamageableLockOnTarget->IsDead())
		DisengageLockOn();
}

void UFSLockOnComponent::UpdateLockOnCamera(float deltaTime)
{
	FVector PlayerLocation{ PlayerOwner->GetActorLocation() };
	FVector TargetLocation{ CurrentLockedOnTarget->GetActorLocation() };
	FRotator PlayerLookAtRotation{ UKismetMathLibrary::FindLookAtRotation(PlayerLocation, TargetLocation) };
	PlayerLookAtRotation.Pitch = 0.0f;

	FRotator CurrentPlayerRotation{ PlayerOwner->GetActorRotation() };
	FRotator SmoothedPlayerRotation{ FMath::RInterpTo(CurrentPlayerRotation, PlayerLookAtRotation, deltaTime, CameraRotationInterpSpeed) };
	PlayerOwner->SetActorRotation(SmoothedPlayerRotation);

	FRotator CameraLookAtRotation{ UKismetMathLibrary::FindLookAtRotation(PlayerLocation, TargetLocation) };
	FVector DirectionToTarget{ (TargetLocation - PlayerLocation).GetSafeNormal() };
	FVector PlayerRight{ PlayerOwner->GetActorRightVector() };
	float DotRight{ static_cast<float>(FVector::DotProduct(DirectionToTarget, PlayerRight)) };

	double Distance{ FVector::Dist(PlayerLocation, TargetLocation) };
	double maxClampDistance{ LockOnDetectionRadius / 2 };
	double DistanceRatio{ FMath::Clamp(Distance / maxClampDistance, 0.0, 1.0) };
	double CurrentYawOffset{ FMath::Lerp(CloseCameraYawOffset, FarCameraYawOffset, DistanceRatio) };

	double CurrentPitchOffset{ FMath::Lerp(CloseCameraPitchOffset, FarCameraPitchOffset, DistanceRatio) };

	// Check if player is attacking to adjust tolerance
	UFSCombatComponent* CombatComp{ PlayerOwner->FindComponentByClass<UFSCombatComponent>() };
	float dotRightTolerance{ (CombatComp && CombatComp->isAttacking()) ? -0.15f : 0.f };

	CameraLookAtRotation.Yaw += DotRight > dotRightTolerance ? -CurrentYawOffset : CurrentYawOffset;
	CameraLookAtRotation.Pitch += CurrentPitchOffset;

	FRotator CurrentCameraRotation{ PlayerOwner->GetController()->GetControlRotation() };
	FRotator SmoothedCameraRotation{ FMath::RInterpTo(CurrentCameraRotation, CameraLookAtRotation, deltaTime, CameraRotationInterpSpeed) };

	PlayerOwner->GetController()->SetControlRotation(SmoothedCameraRotation);
}

bool UFSLockOnComponent::EngageLockOn()
{
	if (!PlayerOwner)
		return false;

	FVector Start{ PlayerOwner->GetActorLocation() };
	FVector End{ Start };

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	TArray<FHitResult> outHits;
	bool bHit{ UKismetSystemLibrary::SphereTraceMultiForObjects(
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
	) };

	if (!bHit)
		return false;

	AController* Controller{ PlayerOwner->GetController() };
	if (!Controller)
		return false;

	FRotator ControlRotation{ Controller->GetControlRotation() };
	FVector CameraForward{ ControlRotation.Vector() };
	CameraForward.Z = 0;
	CameraForward.Normalize();

	TSet<AActor*> uniqueHitActors;
	for (const FHitResult& hit : outHits)
	{
		AActor* HitActor{ hit.GetActor() };
		if (uniqueHitActors.Contains(HitActor))
			continue;

		uniqueHitActors.Add(HitActor);

		if (!HitActor->Implements<UFSFocusable>() ||
			TargetsInLockOnRadius.Contains(HitActor) ||
			!HitActor->Implements<UFSDamageable>() ||
			Cast<IFSDamageable>(HitActor)->IsDead())
			continue;

		TargetsInLockOnRadius.Add(HitActor);
	}

	if (TargetsInLockOnRadius.IsEmpty())
		return false;

	FVector PlayerLocation{ PlayerOwner->GetActorLocation() };
	AActor* NearestTarget{ nullptr };
	float BestScore{ -FLT_MAX };

	for (AActor* TargetActor : TargetsInLockOnRadius)
	{
		FVector ToTarget{ TargetActor->GetActorLocation() - PlayerLocation };
		ToTarget.Z = 0;
		ToTarget.Normalize();

		float DotProduct{ static_cast<float>(FVector::DotProduct(CameraForward, ToTarget)) };
		float Distance{ static_cast<float>(FVector::Dist(PlayerLocation, TargetActor->GetActorLocation())) };
		float Score{ DotProduct * 1000.0f - Distance };

		if (Score > BestScore)
		{
			BestScore = Score;
			NearestTarget = TargetActor;
		}
	}

	if (!NearestTarget)
		return false;

	CurrentLockedOnTarget = NearestTarget;
	CachedDamageableLockOnTarget = Cast<IFSDamageable>(CurrentLockedOnTarget);
	CachedFocusableTarget = Cast<IFSFocusable>(CurrentLockedOnTarget);
	bIsLockedOnEngaged = true;

	CachedFocusableTarget->DisplayAllWidgets(true);

	PrimaryComponentTick.SetTickFunctionEnable(true);

	PlayerOwner->GetController()->SetIgnoreLookInput(true);
	PlayerOwner->GetCharacterMovement()->bOrientRotationToMovement = false;
	PlayerOwner->GetCharacterMovement()->bUseControllerDesiredRotation = true;

	GetWorld()->GetTimerManager().SetTimer(
		LockOnValidCheckTimer,
		this, &UFSLockOnComponent::LockOnValidCheck,
		LockOnDistanceCheckDelay,
		true
	);

	OnLockOnStarted.Execute(CurrentLockedOnTarget);

	return true;
}

bool UFSLockOnComponent::SwitchLockOnTarget(float axisValueX)
{
	if (!CurrentLockedOnTarget || GetWorld()->GetTimerManager().IsTimerActive(delaySwitchLockOnTimer))
		return false;

	FVector Start{ PlayerOwner->GetActorLocation() };
	FVector End{ Start };

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	TArray<FHitResult> outHits;
	bool bHit{ UKismetSystemLibrary::SphereTraceMultiForObjects(
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
	) };

	if (!bHit)
		return false;

	FRotator ControlRotation{ PlayerOwner->GetController()->GetControlRotation() };
	FVector CameraForward{ ControlRotation.Vector() };
	CameraForward.Z = 0;
	CameraForward.Normalize();

	FVector CameraRight{ FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::Y) };
	CameraRight.Z = 0;
	CameraRight.Normalize();

	FVector PlayerLocation{ PlayerOwner->GetActorLocation() };
	bool bLookingRight{ axisValueX > 0 };

	AActor* BestTarget{ nullptr };
	float SmallestAngle{ FLT_MAX };

	TSet<AActor*> ProcessedActors;
	for (const FHitResult& hit : outHits)
	{
		AActor* HitActor{ hit.GetActor() };

		if (ProcessedActors.Contains(HitActor))
			continue;

		ProcessedActors.Add(HitActor);

		if (HitActor == CurrentLockedOnTarget ||
			!HitActor->Implements<UFSFocusable>() ||
			!HitActor->Implements<UFSDamageable>() ||
			Cast<IFSDamageable>(HitActor)->IsDead())
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

	if (!BestTarget)
		return false;

	bool isFullLife{ CachedDamageableLockOnTarget && (CachedDamageableLockOnTarget->GetCurrentHealth() >= CachedDamageableLockOnTarget->GetMaxHealth()) };
	if (isFullLife)
		CachedFocusableTarget->DisplayAllWidgets(false);
	else
		CachedFocusableTarget->DisplayLockedOnWidget(false);

	CurrentLockedOnTarget = BestTarget;
	CachedDamageableLockOnTarget = Cast<IFSDamageable>(CurrentLockedOnTarget);
	CachedFocusableTarget = Cast<IFSFocusable>(CurrentLockedOnTarget);
	CachedFocusableTarget->DisplayAllWidgets(true);

	GetWorld()->GetTimerManager().SetTimer(
		delaySwitchLockOnTimer,
		targetSwitchDelay,
		false
	);

	UFSCombatComponent* CombatComp{ PlayerOwner->FindComponentByClass<UFSCombatComponent>() };
	CombatComp->SetLockedOnTargetRef(CurrentLockedOnTarget);

	return true;
}

void UFSLockOnComponent::DisengageLockOn()
{
	TargetsInLockOnRadius.Empty();

	bool isFullLife{ CachedDamageableLockOnTarget && (CachedDamageableLockOnTarget->GetCurrentHealth() >= CachedDamageableLockOnTarget->GetMaxHealth()) };
	bool isDead{ CachedDamageableLockOnTarget && CachedDamageableLockOnTarget->IsDead() };
	if (isFullLife || isDead)
		CachedFocusableTarget->DisplayAllWidgets(false);
	else
		CachedFocusableTarget->DisplayLockedOnWidget(false);

	CurrentLockedOnTarget = nullptr;
	CachedDamageableLockOnTarget = nullptr;

	bIsLockedOnEngaged = false;

	PrimaryComponentTick.SetTickFunctionEnable(false);

	PlayerOwner->GetController()->ResetIgnoreLookInput();
	PlayerOwner->GetCharacterMovement()->bOrientRotationToMovement = true;
	PlayerOwner->GetCharacterMovement()->bUseControllerDesiredRotation = false;

	LockOnValidCheckTimer.Invalidate();
	OnLockOnStopped.Broadcast();
}

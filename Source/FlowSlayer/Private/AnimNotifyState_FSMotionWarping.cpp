#include "AnimNotifyState_FSMotionWarping.h"

void UAnimNotifyState_FSMotionWarping::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    PlayerOwner = Cast<ACharacter>(MeshComp->GetOwner());
    LockOnCompRef = PlayerOwner ? PlayerOwner->GetComponentByClass<UFSLockOnComponent>() : nullptr;
    MotionWarpingComponent = PlayerOwner ? PlayerOwner->FindComponentByClass<UMotionWarpingComponent>() : nullptr;

    URootMotionModifier_Warp* warpModifier{ Cast<URootMotionModifier_Warp>(RootMotionModifier) };
    if (!warpModifier || !LockOnCompRef || !MotionWarpingComponent)
        return;

    const AActor* targetActor{ GetTargetForMotionWarp(SearchRadius, bDebugLines) };

    if (attackType == EFSMotionWarpingAttackType::Ground)
        SetupGroundAttackMotionWarp(warpModifier->WarpTargetName, targetActor, ForwardOffset, bDebugLines);
    else
        SetupAirAttackMotionWarp(warpModifier->WarpTargetName, targetActor, ZOffset, ForwardOffset, bDebugLines);
        
}

void UAnimNotifyState_FSMotionWarping::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (!MotionWarpingComponent)
        return;

    MotionWarpingComponent->RemoveAllWarpTargets();

    if (!PlayerOwner)
        return;

    EMovementMode currentMovementMode{ PlayerOwner->GetCharacterMovement()->MovementMode };
    if (attackType == EFSMotionWarpingAttackType::Launcher && currentMovementMode == EMovementMode::MOVE_Flying)
        PlayerOwner->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
}

const AActor* UAnimNotifyState_FSMotionWarping::GetTargetForMotionWarp(float searchRadius, bool debugLines)
{
    const AActor* lockedOnTarget{ LockOnCompRef->GetCurrentLockedOnTarget() };

    if (lockedOnTarget && (FVector::DistSquared(PlayerOwner->GetActorLocation(), lockedOnTarget->GetActorLocation()) <= (searchRadius * searchRadius)))
        return lockedOnTarget;

    return GetNearestEnemyFromPlayer(searchRadius, debugLines);
}

void UAnimNotifyState_FSMotionWarping::SetupAirAttackMotionWarp(FName motionWarpingTargetName, const AActor* targetActor, float zOffset, float forwardOffset, bool debugLines)
{
    if (!MotionWarpingComponent || !targetActor)
        return;

    FVector playerLocation{ PlayerOwner->GetActorLocation() };
    FVector enemyLocation{ targetActor->GetActorLocation() };

    FVector directionToEnemy{ (enemyLocation - playerLocation).GetSafeNormal() };

    FVector targetLocation{ enemyLocation };
    targetLocation.Z += zOffset;
    targetLocation -= directionToEnemy * forwardOffset;

    FRotator lookAtRotation{ UKismetMathLibrary::FindLookAtRotation(playerLocation, enemyLocation) };

    FMotionWarpingTarget target;
    target.Name = motionWarpingTargetName;
    target.Location = targetLocation;
    target.Rotation = lookAtRotation;

    PlayerOwner->SetActorRotation(FRotator(0.f, lookAtRotation.Yaw, 0.f), ETeleportType::TeleportPhysics);

    if (attackType == EFSMotionWarpingAttackType::Launcher)
        PlayerOwner->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);

    MotionWarpingComponent->AddOrUpdateWarpTarget(target);
}

void UAnimNotifyState_FSMotionWarping::SetupGroundAttackMotionWarp(FName motionWarpingTargetName, const AActor* targetActor, float forwardOffset, bool debugLines)
{
    if (!MotionWarpingComponent || !targetActor)
        return;
    
    FVector targetLocation{ targetActor->GetActorLocation() };
    FVector forwardOffsetVector{ PlayerOwner->GetActorForwardVector() * forwardOffset };
    targetLocation += forwardOffsetVector;

    FVector playerLocation{ PlayerOwner->GetActorLocation() };
    FVector enemyLocation{ targetActor->GetActorLocation() };
    FRotator lookAtRotation{ UKismetMathLibrary::FindLookAtRotation(playerLocation, enemyLocation) };

    FMotionWarpingTarget target;
    target.Name = motionWarpingTargetName;
    target.Location = targetLocation;
    target.Rotation = lookAtRotation;

    MotionWarpingComponent->AddOrUpdateWarpTarget(target);
}

AActor* UAnimNotifyState_FSMotionWarping::GetNearestEnemyFromPlayer(float distanceRadius, bool debugLines) const
{
    FVector Start{ PlayerOwner->GetActorLocation() };
    FVector End{ Start };

    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(PlayerOwner);

    TArray<FHitResult> outHits;
    bool bHit{ UKismetSystemLibrary::SphereTraceMultiForObjects(
        PlayerOwner->GetWorld(),
        Start,
        End,
        distanceRadius,
        ObjectTypes,
        false,
        ActorsToIgnore,
        debugLines ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
        outHits,
        true
    ) };

    if (!bHit)
        return nullptr;

    TSet<AActor*> uniqueHitActors;
    float shortestDistance{ FLT_MAX };
    AActor* nearestEnemy{ nullptr };
    for (const auto& hit : outHits)
    {
        AActor* hitActor{ hit.GetActor() };
        if (uniqueHitActors.Contains(hitActor))
            continue;

        uniqueHitActors.Add(hitActor);

        if (hitActor->Implements<UFSDamageable>() && !Cast<IFSDamageable>(hitActor)->GetHealthComponent()->IsDead())
        {
            float newDistance{ static_cast<float>(FVector::Distance(hitActor->GetActorLocation(), PlayerOwner->GetActorLocation())) };
            if (newDistance < shortestDistance)
            {
                shortestDistance = newDistance;
                nearestEnemy = hitActor;
            }
        }
    }

    return nearestEnemy;
}
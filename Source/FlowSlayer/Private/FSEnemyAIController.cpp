#include "FSEnemyAIController.h"

AFSEnemyAIController::AFSEnemyAIController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AFSEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    UPathFollowingComponent* PathFollowingComp{ GetPathFollowingComponent() };
    if (PathFollowingComp)
        PathFollowingComp->OnRequestFinished.AddUObject(this, &AFSEnemyAIController::OnMoveToTargetCompleted);
}

void AFSEnemyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    OwnedEnemyPawn = Cast<AFSEnemy>(InPawn);
}

void AFSEnemyAIController::OnMoveToTargetCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    EPathFollowingResult::Type ResultCode{ Result.Code };
    if (ResultCode != EPathFollowingResult::Success)
        return;

    if (OwnedEnemyPawn && !OwnedEnemyPawn->IsDead())
    {
        RotateToPlayer();
        OwnedEnemyPawn->Attack();
    }
}

void AFSEnemyAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (OwnedEnemyPawn && !OwnedEnemyPawn->IsAttacking() && !OwnedEnemyPawn->IsDead() && !OwnedEnemyPawn->IsStunned())
        FollowPlayer();
}

void AFSEnemyAIController::FollowPlayer()
{
    UWorld* World{ GetWorld() };
    APawn* PlayerPawn{ UGameplayStatics::GetPlayerPawn(World, 0) };
    if (!World || !PlayerPawn)
        return;

    EPathFollowingStatus::Type Status{ GetMoveStatus() };
    if (Status == EPathFollowingStatus::Idle)
    {
        FAIMoveRequest MoveRequest{ PlayerPawn };
        MoveRequest.SetAcceptanceRadius(OwnedEnemyPawn->GetAttackRange());
        MoveTo(MoveRequest);
    }
}

void AFSEnemyAIController::RotateToPlayer()
{
    UWorld* World{ GetWorld() };
    APawn* PlayerPawn{ UGameplayStatics::GetPlayerPawn(World, 0) };
    if (!World || !PlayerPawn)
        return;

    FVector DirectionToPlayer = PlayerPawn->GetActorLocation() - OwnedEnemyPawn->GetActorLocation();
    DirectionToPlayer.Z = 0.0f;

    FRotator LookAtRotation = DirectionToPlayer.Rotation();
    OwnedEnemyPawn->SetActorRotation(LookAtRotation);
}

void AFSEnemyAIController::JumpToDestination(FVector Destination)
{
    ACharacter* localChar{ Cast<ACharacter>(GetPawn()) };
    if (!localChar)
        return;

    FVector StartLocation{ localChar->GetActorLocation() };
    FVector OutLaunchVelocity;

    FVector HorizontalDelta{ Destination - StartLocation };
    HorizontalDelta.Z = 0.f;
    float HorizontalDistance{ static_cast<float>(HorizontalDelta.Size()) };

    float HeightDifference{ static_cast<float>(Destination.Z - StartLocation.Z) };

    const float MinHeightToJump{ 50.f };
    const float MaxHorizontalDistanceForFall{ 300.f }; 

    if (HeightDifference < -MinHeightToJump && HorizontalDistance < MaxHorizontalDistanceForFall)
        return;

    float ArcParam{ FMath::Clamp(0.5f + (HorizontalDistance / 1500.f), 0.5f, 0.85f) };

    float JumpApexHeight{ FMath::Max(300.f, HorizontalDistance * 0.5f) };

    FVector AdjustedDestination{ Destination };
    AdjustedDestination.Z += JumpApexHeight;

    bool bSuccess{ UGameplayStatics::SuggestProjectileVelocity_CustomArc(
        GetWorld(),
        OutLaunchVelocity,
        StartLocation,
        AdjustedDestination,
        0.f,      
        ArcParam  
    ) };

    if (bSuccess)
        localChar->LaunchCharacter(OutLaunchVelocity, true, true);
}
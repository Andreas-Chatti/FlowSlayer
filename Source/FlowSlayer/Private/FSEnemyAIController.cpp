#include "FSEnemyAIController.h"

AFSEnemyAIController::AFSEnemyAIController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AFSEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    UPathFollowingComponent* PathFollowingComp{ GetPathFollowingComponent() };
    checkf(PathFollowingComp, TEXT("FATAL: PathFollowingComp is NULL or INVALID !"));
    PathFollowingComp->OnRequestFinished.AddUObject(this, &AFSEnemyAIController::OnMoveToTargetCompleted);

    PlayerRef = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    verifyf(PlayerRef, TEXT("WARNING: PlayerRef is NULL or INVALID !"));
}

void AFSEnemyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    OwnedEnemy = Cast<AFSEnemy>(InPawn);
    checkf(OwnedEnemy, TEXT("FATAL: OwnedEnemy is NULL or INVALID !"));
}

void AFSEnemyAIController::OnMoveToTargetCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    EPathFollowingResult::Type ResultCode{ Result.Code };
    if (ResultCode != EPathFollowingResult::Success)
        return;

    if (OwnedEnemy && !OwnedEnemy->GetHealthComponent()->IsDead() && OwnedEnemy->CanAttack())
        OwnedEnemy->Attack();
}

void AFSEnemyAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (OwnedEnemy && !OwnedEnemy->IsAttacking() && !OwnedEnemy->GetHealthComponent()->IsDead() && OwnedEnemy->CanAttack())
        FollowPlayer();
}

void AFSEnemyAIController::FollowPlayer()
{
    if (!PlayerRef)
        return;

    EPathFollowingStatus::Type Status{ GetMoveStatus() };
    if (Status == EPathFollowingStatus::Idle)
    {
        FAIMoveRequest MoveRequest{ PlayerRef };
        MoveRequest.SetAcceptanceRadius(OwnedEnemy->GetAttackRange());
        MoveTo(MoveRequest);
    }
}

void AFSEnemyAIController::JumpToDestination(FVector Destination)
{
    if (!OwnedEnemy)
        return;

    FVector StartLocation{ OwnedEnemy->GetActorLocation() };
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
        OwnedEnemy->LaunchCharacter(OutLaunchVelocity, true, true);
}
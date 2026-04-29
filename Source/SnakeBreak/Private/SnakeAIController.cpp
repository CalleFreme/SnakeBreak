#include "SnakeAIController.h"
#include "FoodActor.h"
#include "GridManagerActor.h"
#include "Kismet/GameplayStatics.h"
#include "SnakeGameMode.h"

ASnakeAIController::ASnakeAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASnakeAIController::BeginPlay()
{
	Super::BeginPlay();
	CacheWorldReferences();
}

void ASnakeAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ControlledSnake = Cast<ASnakePawn>(InPawn);
	CacheWorldReferences();
}

void ASnakeAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAI(DeltaSeconds);
}

void ASnakeAIController::CacheWorldReferences()
{
	GridManager = Cast<AGridManagerActor>(
		UGameplayStatics::GetActorOfClass(this, AGridManagerActor::StaticClass()));

	FoodActor = Cast<AFoodActor>(
		UGameplayStatics::GetActorOfClass(this, AFoodActor::StaticClass()));
}

void ASnakeAIController::UpdateAI(float DeltaSeconds)
{
	// Alternatively, GameMode could expose the current Food/Grid references directly, 
	// or broadcast an even when the food actor changes.
	if (!GridManager || !FoodActor)
	{
		CacheWorldReferences();
	}
	
	if (!GridManager || !FoodActor)
	{
		return;
	}
	
	if (!ControlledSnake || ControlledSnake->IsDead())
	{
		return;
	}

	// Important:
	// Only decide when the snake is between cells / about to start a new step.
	// Otherwise the AI may spam direction changes mid-lerp.
	if (ControlledSnake->IsMovingToTarget())
	{
		return;
	}

	switch (AIMode)
	{
	case ESnakeAIMode::GreedySafe:
		DecideGreedySafe();
		break;

	case ESnakeAIMode::GridAStar:
		DecideGridAStar();
		break;

	case ESnakeAIMode::FreeMovingNav:
		DecideFreeMovingNav();
		break;
	}
}

TArray<ESnakeDirection> ASnakeAIController::GetCandidateDirections() const
{
	return {
		ESnakeDirection::Up,
		ESnakeDirection::Down,
		ESnakeDirection::Left,
		ESnakeDirection::Right
	};
}

bool ASnakeAIController::IsCellSafe(const FIntPoint& Cell) const
{
	if (!GridManager || !ControlledSnake)
	{
		return false;
	}

	if (!GridManager->IsInsidePlayableArea(Cell))
	{
		return false;
	}

	if (GridManager->IsBlockedCell(Cell))
	{
		return false;
	}

	if (ControlledSnake->GetAllOccupiedGridCells().Contains(Cell))
	{
		return false;
	}

	if (ASnakeGameMode* GM = GetWorld()->GetAuthGameMode<ASnakeGameMode>())
	{
		if (GM->IsCellOccupiedByOtherSnake(ControlledSnake, Cell))
		{
			return false;
		}
	}

	return true;
}

void ASnakeAIController::DecideGreedySafe()
{
	// Simple but not very robust. Can trap itself.
	if (!ControlledSnake || !FoodActor)
	{
		return;
	}

	const FIntPoint FoodCell = FoodActor->GetFoodGridPosition();

	bool bFoundMove = false;
	ESnakeDirection BestDirection = ControlledSnake->GetCurrentDirection();
	int32 BestScore = TNumericLimits<int32>::Max();

	for (ESnakeDirection Direction : GetCandidateDirections())
	{
		if (!ControlledSnake->CanRequestDirection(Direction))
		{
			continue;
		}

		const FIntPoint NextCell = ControlledSnake->GetNextCellForDirection(Direction);

		if (!IsCellSafe(NextCell))
		{
			continue;
		}
		// This will also reject illegal 180-turns.
		// But RequestDirection changes state, so don't call it yet.
		// Add a public CanRequestDirection(...) if you want this cleaner.
		const int32 Distance =
			FMath::Abs(NextCell.X - FoodCell.X) +
			FMath::Abs(NextCell.Y - FoodCell.Y);

		int32 Score = Distance;
		
		// Prefer moves that do not immediately enter a dead end.
		if (!HasEscapeMoveFrom(NextCell))
		{
			Score += 1000;
		}

		if (Score < BestScore)
		{
			BestScore = Score;
			BestDirection = Direction;
			bFoundMove = true;
		}
	}

	if (bFoundMove)
	{
		ControlledSnake->RequestDirection(BestDirection);
		
		UE_LOG(LogTemp, Log, TEXT("AI chose %s with score %d"),
			*UEnum::GetValueAsString(BestDirection),
			BestScore);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AI found no safe move."));
	}

}

void ASnakeAIController::DecideGridAStar()
{
	DecideGreedySafe();
}

void ASnakeAIController::DecideFreeMovingNav()
{
	DecideGreedySafe();
}

bool ASnakeAIController::HasEscapeMoveFrom(const FIntPoint& Cell) const
{
	// A more accurate version could simulate:
	// - current head moves to NextCell
	// - body shifts forwards
	// - tail may disappear unless growing
	// - then evalue follow-up moves
	int32 SafeFollowUpMoves = 0;

	for (ESnakeDirection Direction : GetCandidateDirections())
	{
		const FIntPoint Candidate = Cell + ASnakePawn::DirectionToGridOffset(Direction);

		if (Candidate == ControlledSnake->GetCurrentGridPosition())
		{
			continue;
		}

		if (IsCellSafe(Candidate))
		{
			++SafeFollowUpMoves;
		}
	}

	return SafeFollowUpMoves > 0;
}

ESnakeDirection ASnakeAIController::DirectionFromStep(
	const FIntPoint& From,
	const FIntPoint& To) const
{
	const FIntPoint Delta = To - From;

	if (Delta == FIntPoint(1, 0))
	{
		return ESnakeDirection::Up;
	}
	if (Delta == FIntPoint(-1, 0))
	{
		return ESnakeDirection::Down;
	}
	if (Delta == FIntPoint(0, -1))
	{
		return ESnakeDirection::Left;
	}
	if (Delta == FIntPoint(0, 1))
	{
		return ESnakeDirection::Right;
	}

	return ControlledSnake
		? ControlledSnake->GetCurrentDirection()
		: ESnakeDirection::Right;
}

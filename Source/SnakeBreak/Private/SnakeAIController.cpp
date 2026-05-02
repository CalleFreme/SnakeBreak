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
	if (!IsValid(GridManager) || !IsValid(FoodActor))
	{
		CacheWorldReferences();
	}
	
	if (!IsValid(GridManager) || !IsValid(FoodActor))
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

	RememberCurrentHeadCell();

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

	return ControlledSnake->IsNextGridCellSafe(Cell);
}

void ASnakeAIController::DecideGreedySafe()
{
	if (!ControlledSnake || !FoodActor)
	{
		return;
	}

	const FIntPoint FoodCell = FoodActor->GetFoodGridPosition();
	const int32 OccupiedCellCount = ControlledSnake->GetAllOccupiedGridCells().Num();
	const int32 MinimumUsefulSpace = OccupiedCellCount + MinimumReachableSpaceBuffer;

	bool bFoundMove = false;
	ESnakeDirection BestDirection = ControlledSnake->GetCurrentDirection();
	int32 BestScore = TNumericLimits<int32>::Max();

	// This is still greedy, but it is no longer only "closest to food".
	// Each legal move is scored by food distance, available space, loop history,
	// and a tiny turn cost. Lower scores are better.
	for (ESnakeDirection Direction : GetCandidateDirections())
	{
		if (!ControlledSnake->CanMoveInDirection(Direction))
		{
			continue;
		}

		const FIntPoint NextCell = ControlledSnake->GetNextCellForDirection(Direction);

		if (!IsCellSafe(NextCell))
		{
			continue;
		}

		const int32 Distance =
			FMath::Abs(NextCell.X - FoodCell.X) +
			FMath::Abs(NextCell.Y - FoodCell.Y);

		const int32 ReachableCells = CountReachableCellsAfterMove(NextCell);

		int32 Score = Distance * 10;
		Score += GetRecentCellPenalty(NextCell);
		Score += GetContestedCellPenalty(NextCell);
		Score += GetTurnPenalty(Direction);

		// Flood-fill space is a cheap local substitute for pathfinding.
		// If a move leaves less room than the snake's body plus a buffer,
		// treat it as risky even when it technically has an exit.
		if (ReachableCells < MinimumUsefulSpace)
		{
			Score += (MinimumUsefulSpace - ReachableCells) * 50;
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
		
		if (bDrawDebugAI)
		{
			UE_LOG(LogTemp, Log, TEXT("AI chose %s with score %d"),
				*UEnum::GetValueAsString(BestDirection),
				BestScore);
		}
	}
	else
	{
		if (bDrawDebugAI)
		{
			UE_LOG(LogTemp, Warning, TEXT("AI found no safe move."));
		}
	}

}

bool ASnakeAIController::IsCellReachableAfterMove(
	const FIntPoint& Cell,
	const TSet<FIntPoint>& ProjectedSelfCells) const
{
	if (!GridManager)
	{
		return false;
	}

	if (!GridManager->IsInsidePlayableArea(Cell) || GridManager->IsBlockedCell(Cell))
	{
		return false;
	}

	if (ProjectedSelfCells.Contains(Cell))
	{
		return false;
	}

	const ASnakeGameMode* GM = GetWorld()->GetAuthGameMode<ASnakeGameMode>();
	return !GM || !GM->IsCellOccupiedByOtherSnake(ControlledSnake, Cell);
}

int32 ASnakeAIController::CountReachableCellsAfterMove(const FIntPoint& NextCell) const
{
	if (!ControlledSnake || !GridManager)
	{
		return 0;
	}

	TSet<FIntPoint> ProjectedSelfCells;
	ControlledSnake->GetProjectedOccupiedGridCellsAfterMove(NextCell, ProjectedSelfCells);

	TSet<FIntPoint> Visited;
	TArray<FIntPoint> Frontier;
	Frontier.Add(NextCell);
	Visited.Add(NextCell);

	// Breadth-first flood fill from the candidate head cell. This estimates
	// how much open board the snake can still reach after committing to a move.
	for (int32 FrontierIndex = 0; FrontierIndex < Frontier.Num(); ++FrontierIndex)
	{
		const FIntPoint Current = Frontier[FrontierIndex];

		for (ESnakeDirection Direction : GetCandidateDirections())
		{
			const FIntPoint Neighbor = Current + ASnakePawn::DirectionToGridOffset(Direction);
			if (Visited.Contains(Neighbor))
			{
				continue;
			}

			if (!IsCellReachableAfterMove(Neighbor, ProjectedSelfCells))
			{
				continue;
			}

			Visited.Add(Neighbor);
			Frontier.Add(Neighbor);
		}
	}

	return Visited.Num();
}

int32 ASnakeAIController::GetRecentCellPenalty(const FIntPoint& Cell) const
{
	int32 Penalty = 0;

	for (int32 i = 0; i < RecentHeadCells.Num(); ++i)
	{
		if (RecentHeadCells[i] == Cell)
		{
			// Recent repeats are more likely to indicate a local loop.
			Penalty += (RecentHeadCells.Num() - i) * 8;
		}
	}

	return Penalty;
}

int32 ASnakeAIController::GetContestedCellPenalty(const FIntPoint& Cell) const
{
	const ASnakeGameMode* GM = GetWorld()->GetAuthGameMode<ASnakeGameMode>();
	if (!GM || !GM->IsCellReachableByOtherSnakeHead(ControlledSnake, Cell))
	{
		return 0;
	}

	// This is a risk penalty, not a hard block. A contested cell may still be
	// the only legal move, but the AI should avoid voluntary head-on races.
	return 250;
}

int32 ASnakeAIController::GetTurnPenalty(ESnakeDirection Direction) const
{
	return Direction == ControlledSnake->GetCurrentDirection() ? 0 : 2;
}

void ASnakeAIController::RememberCurrentHeadCell()
{
	if (!ControlledSnake || RecentCellMemory <= 0)
	{
		return;
	}

	const FIntPoint CurrentCell = ControlledSnake->GetCurrentGridPosition();
	if (CurrentCell == LastRememberedHeadCell)
	{
		return;
	}

	RecentHeadCells.Add(CurrentCell);
	LastRememberedHeadCell = CurrentCell;

	while (RecentHeadCells.Num() > RecentCellMemory)
	{
		RecentHeadCells.RemoveAt(0);
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

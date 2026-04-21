#include "SnakeGameMode.h"
#include "SnakeGameState.h"
#include "SnakePawn.h"
#include "FoodActor.h"
#include "GridManagerActor.h"
#include "Kismet/GameplayStatics.h"

void ASnakeGameMode::BeginPlay()
{
	Super::BeginPlay();
	CacheGridManager();
	StartPlayingRun();
}

void ASnakeGameMode::CacheGridManager()
{
	GridManager = Cast<AGridManagerActor>(UGameplayStatics::GetActorOfClass(this, AGridManagerActor::StaticClass()));
}

ASnakeGameState* ASnakeGameMode::GetSnakeGameState()
{
	return GetGameState<ASnakeGameState>();
}

void ASnakeGameMode::StartPlayingRun()
{
	if (ASnakeGameState* GS = GetSnakeGameState())
	{
		GS->Score = 0;
		GS->MatchPhase = ESnakeMatchPhase::Playing;
	}

	SpawnSnake();
	SpawnFood();
	MoveFoodToRandomFreeCell();
}

void ASnakeGameMode::SpawnSnake()
{
	if (!SnakePawnClass || !GridManager)
	{
		return;
	}

	if (SpawnedSnakePawn)
	{
		SpawnedSnakePawn->Destroy();
		SpawnedSnakePawn = nullptr;
	}

	const FIntPoint SnakeSpawnCell(10, 10);
	const FVector SnakeSpawnLocation = GridManager->GridToWorld(SnakeSpawnCell);

	SpawnedSnakePawn = GetWorld()->SpawnActor<ASnakePawn>(
		SnakePawnClass, SnakeSpawnLocation, FRotator::ZeroRotator);

	if (!SpawnedSnakePawn)
	{
		return;
	}

	SpawnedSnakePawn->OnFoodConsumed.AddDynamic(this, &ASnakeGameMode::HandleFoodConsumed);
	SpawnedSnakePawn->OnSnakeDied.AddDynamic(this, &ASnakeGameMode::HandleSnakeDied);
}

void ASnakeGameMode::SpawnFood()
{
	if (!FoodActorClass || !GridManager)
	{
		return;
	}

	if (SpawnedFoodActor)
	{
		SpawnedFoodActor->Destroy();
		SpawnedFoodActor = nullptr;
	}

	SpawnedFoodActor = GetWorld()->SpawnActor<AFoodActor>(
		FoodActorClass, FVector::ZeroVector, FRotator::ZeroRotator);
}

void ASnakeGameMode::MoveFoodToRandomFreeCell()
{
	if (!GridManager || !SpawnedFoodActor || !SpawnedSnakePawn)
	{
		return;
	}

	TArray<FIntPoint> ForbiddenCells = SpawnedSnakePawn->GetAllOccupiedGridCells();

	FIntPoint NewFoodCell;
	if (GridManager->TryGetRandomFreeCell(NewFoodCell, ForbiddenCells))
	{
		SpawnedFoodActor->SetActorEnableCollision(true);
		SpawnedFoodActor->SetFoodGridPosition(NewFoodCell, GridManager->GridToWorld(NewFoodCell));
	}
}

void ASnakeGameMode::HandleFoodConsumed(int32 ScoreValue)
{
	if (ASnakeGameState* GS = GetSnakeGameState())
	{
		GS->Score += ScoreValue;
	}

	MoveFoodToRandomFreeCell();
}

void ASnakeGameMode::HandleSnakeDied()
{
	if (ASnakeGameState* GS = GetSnakeGameState())
	{
		GS->MatchPhase = ESnakeMatchPhase::Outro;
	}
}

void ASnakeGameMode::RestartRun()
{
	StartPlayingRun();
}

void ASnakeGameMode::ReturnToMainMenu()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("MainMenuMap")));
}


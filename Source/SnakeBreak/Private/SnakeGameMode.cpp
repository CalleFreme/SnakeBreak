#include "SnakeGameMode.h"
#include "SnakeGameState.h"
#include "SnakePawn.h"
#include "FoodActor.h"
#include "GridManagerActor.h"
#include "GameFramework/PlayerController.h"
#include "SnakePlayerController.h"
#include "Kismet/GameplayStatics.h"

void ASnakeGameMode::BeginPlay()
{
	// This is sort of the entry point of our game application!
	Super::BeginPlay();
	CacheGridManager(); // We create and store a GridManager
	StartPlayingRun();	// We start the gameplay loop
}

// The GameMode is responsible for holding some important entities, such as a GridManager:
void ASnakeGameMode::CacheGridManager()
{
	GridManager = Cast<AGridManagerActor>(UGameplayStatics::GetActorOfClass(this, AGridManagerActor::StaticClass()));
}

// The GameMode has a reference to the GameState object through this method:
ASnakeGameState* ASnakeGameMode::GetSnakeGameState()
{
	return GetGameState<ASnakeGameState>();
}

// Set up everything needed for a new play run
void ASnakeGameMode::StartPlayingRun()
{
	// We set the GameState to proper starting values (like Score = 0) 
	if (ASnakeGameState* GS = GetSnakeGameState())
	{
		GS->Score = 0;
		GS->SetMatchPhase(ESnakeMatchPhase::Playing);
	}

	// We spawn and place important scene actors
	SpawnSnake();
	SpawnFood();
	MoveFoodToRandomFreeCell();
}

void ASnakeGameMode::SpawnSnake()
{
	// This function spawns a SnakePawn instance in the world,
	// and stores it in our SpawnedSnakePawn member variable,
	// and binds the pawn instance and its event functions to a
	// multi-cast delegate (i.e. this GameMode object)
	if (!SnakePawnClass || !GridManager)
	{
		return;
	}

	if (SpawnedSnakePawn)
	{
		SpawnedSnakePawn->Destroy();
		SpawnedSnakePawn = nullptr;
	}

	const FIntPoint SnakeSpawnCell(GridManager->GridDimensions.X/2, GridManager->GridDimensions.Y/2);
	const FVector SnakeSpawnLocation = GridManager->GridToWorld(SnakeSpawnCell);

	SpawnedSnakePawn = GetWorld()->SpawnActor<ASnakePawn>(
		SnakePawnClass, SnakeSpawnLocation, FRotator::ZeroRotator);

	if (!SpawnedSnakePawn)
	{
		return;
	}
	
	SpawnedSnakePawn->OnFoodConsumed.AddDynamic(this, &ASnakeGameMode::HandleFoodConsumed);
	SpawnedSnakePawn->OnSnakeDied.AddDynamic(this, &ASnakeGameMode::HandleSnakeDied);
	
	// Possesion setup
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		PC->Possess(SpawnedSnakePawn);
		UE_LOG(LogTemp, Warning, TEXT("Player Controller possessed SnakePawn."));
	}
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
		SpawnedFoodActor->RespawnFood(NewFoodCell, GridManager->GridToWorld(NewFoodCell));
	}
}

void ASnakeGameMode::HandleFoodConsumed(int32 ScoreValue)
{
	if (ASnakeGameState* GS = GetSnakeGameState())
	{
		GS->AddScore(ScoreValue);
	}

	MoveFoodToRandomFreeCell();
}

void ASnakeGameMode::HandleSnakeDied()
{
	if (ASnakeGameState* GS = GetSnakeGameState())
	{
		ChangePhase(ESnakeMatchPhase::Outro);
	}
}

void ASnakeGameMode::ChangePhase(const ESnakeMatchPhase NewPhase)
{
	if (ASnakeGameState* GS = GetSnakeGameState())
	{
		// The GameMode sets the state, does care about driving UI
		GS->SetMatchPhase(NewPhase);
	}
}

void ASnakeGameMode::RestartRun()
{
	if (SpawnedSnakePawn)
	{
		SpawnedSnakePawn->ResetSnake();
	}
	if (ASnakeGameState* GS = GetSnakeGameState())
	{
		GS->Score = 0;
		GS->SetMatchPhase(ESnakeMatchPhase::Playing);
	}
	MoveFoodToRandomFreeCell();
}

void ASnakeGameMode::ReturnToMainMenu()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("MainMenuMap")));
}


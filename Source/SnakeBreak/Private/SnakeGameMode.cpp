#include "SnakeGameMode.h"
#include "SnakeGameInstance.h"
#include "SnakeGameState.h"
#include "SnakePawn.h"
#include "FoodActor.h"
#include "GridManagerActor.h"
#include "SnakeGameTypes.h"
#include "GameFramework/PlayerController.h"
#include "SnakePlayerController.h"
#include "SnakeStageConfig.h"
#include "Kismet/GameplayStatics.h"

void ASnakeGameMode::BeginPlay()
{
	// This is sort of the entry point of our game application!
	Super::BeginPlay();
	
	CacheGridManager(); // We create and store a GridManager
	
	if (USnakeGameInstance* GI = GetGameInstance<USnakeGameInstance>())
	{
		ActiveGameMode = GI->SelectedGameMode;
		Slot0Type = GI->Slot0Type;
		Slot1Type = GI->Slot1Type;
	}
	
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
	BattleResult = ESnakeBattleResult::None;

	if (ASnakeGameState* GS = GetSnakeGameState())
	{
		GS->Score = 0;
		GS->SetMatchPhase(ESnakeMatchPhase::Playing);
	}

	LoadStage(0);
}

void ASnakeGameMode::SpawnSnake()
{
	// Legacy single-player spawn path. Prefer SpawnSnakeForSlot().
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
	if (!GridManager || !SpawnedFoodActor)
	{
		return;
	}

	const TArray<FIntPoint> ForbiddenCells = GetAllSnakeOccupiedCells();

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
	
	FoodEatenThiStage++;
	
	// Later: introduce a separate rules for when Battle ends (one snake died, or score target reacher, or timer runs out)
	if (ActiveGameMode != ESnakeGameModeType::Battle)
	{
		const FSnakeStageConfig& Stage = Stages[CurrentStageIndex];
	
		if (FoodEatenThiStage >= Stage.FoodToClear)
		{
			AdvanceStage();
			return;
		}
	}

	MoveFoodToRandomFreeCell();
}

void ASnakeGameMode::HandleSnakeDied(ASnakePawn* DeadSnake)
{
	if (ActiveGameMode != ESnakeGameModeType::Battle)
	{
		ChangePhase(ESnakeMatchPhase::Outro);
		return;
	}

	const int32 DeadIndex = SpawnedSnakes.IndexOfByKey(DeadSnake);

	if (DeadIndex == 0)
	{
		BattleResult = ESnakeBattleResult::Player0Won;
	}
	else if (DeadIndex == 1)
	{
		BattleResult = ESnakeBattleResult::Player1Won;
	}
	else
	{
		BattleResult = ESnakeBattleResult::None;
	}

	if (ASnakeGameState* GS = GetSnakeGameState())
	{
		GS->BattleResult = BattleResult;
	}

	ChangePhase(ESnakeMatchPhase::Outro);
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
	BattleResult = ESnakeBattleResult::None;
	FoodEatenThiStage = 0;

	if (ASnakeGameState* GS = GetSnakeGameState())
	{
		GS->Score = 0;
		GS->SetMatchPhase(ESnakeMatchPhase::Playing);
	}

	LoadStage(0);
}

void ASnakeGameMode::ReturnToMainMenu()
{
	UGameplayStatics::OpenLevel(this, FName(TEXT("MainMenuMap")));
}

void ASnakeGameMode::LoadStage(int32 StageIndex)
{

	if (!Stages.IsValidIndex(StageIndex) || !GridManager)
	{
		return;
	}
	
	CurrentStageIndex = StageIndex;
	FoodEatenThiStage = 0;
		
	const FSnakeStageConfig& Stage = Stages[StageIndex];
		
	GridManager->ApplyStage(Stage);
	
	DestroySpawnedSnakes();
	
	if (ActiveGameMode == ESnakeGameModeType::NormalSinglePlayer)
	{
		SpawnSinglePlayerSnake();
	}
	else if (ActiveGameMode == ESnakeGameModeType::Battle)
	{
		SpawnBattleSnakes();
	}
	else if (ActiveGameMode == ESnakeGameModeType::Coop)
	{
		SpawnCoopSnakes();
	}

	SpawnFood();
	MoveFoodToRandomFreeCell();

	if (ASnakeGameState* GS = GetSnakeGameState())
	{
		GS->CurrentStageIndex = CurrentStageIndex;
		GS->FoodEatenThisStage = FoodEatenThiStage;
		GS->FoodToClearStage = Stage.FoodToClear;
	}
}

void ASnakeGameMode::AdvanceStage()
{
	const int32 NextStage = CurrentStageIndex + 1;
	
	if (!Stages.IsValidIndex(NextStage))
	{
		ChangePhase(ESnakeMatchPhase::Outro);
		return;
	}
	
	LoadStage(NextStage);
}

ASnakePawn* ASnakeGameMode::SpawnSnakeForSlot(
	int32 SlotIndex,
	const FIntPoint& SpawnCell,
	ESnakePlayerSlotType SlotType)
{
	if (!SnakePawnClass || !GridManager)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot spawn snake: missing SnakePawnClass or GridManager."));
		return nullptr;
	}

	const FSnakeStageConfig* Stage = Stages.IsValidIndex(CurrentStageIndex)
		? &Stages[CurrentStageIndex]
		: nullptr;

	const FVector SpawnLocation = GridManager->GridToWorld(SpawnCell);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASnakePawn* NewSnake = GetWorld()->SpawnActor<ASnakePawn>(
		SnakePawnClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams);

	if (!NewSnake)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn snake for slot %d."), SlotIndex);
		return nullptr;
	}

	NewSnake->OnFoodConsumed.AddDynamic(this, &ASnakeGameMode::HandleFoodConsumed);
	NewSnake->OnSnakeDied.AddDynamic(this, &ASnakeGameMode::HandleSnakeDied);

	if (Stage)
	{
		NewSnake->ApplyStageSettings(
			Stage->CellSize,
			Stage->GridDimensions,
			Stage->MoveStepTime);
	}

	NewSnake->SetStartGridPosition(SpawnCell);

	if (SlotIndex == 0)
	{
		NewSnake->SetInitialDirection(ESnakeDirection::Right);
	}
	else
	{
		NewSnake->SetInitialDirection(ESnakeDirection::Left);
	}

	NewSnake->ResetSnake();

	if (SlotType == ESnakePlayerSlotType::Human)
	{
		APlayerController* PC = GetOrCreateLocalPlayerController(SlotIndex);
		
		if (PC)
		{
			NewSnake->SetPlayerSlotIndex(SlotIndex); // PossessedBy() calls SetupEnhancedInput(), which uses player slot index
			PC->Possess(NewSnake);

			UE_LOG(LogTemp, Warning,
				TEXT("Human player slot %d possessed snake %s with controller %s."),
				SlotIndex,
				*NewSnake->GetName(),
				*PC->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error,
				TEXT("Could not get/create PlayerController for slot %d."),
				SlotIndex);
		}
	}
	else
	{
		if (!SnakeAIControllerClass)
		{
			UE_LOG(LogTemp, Error, TEXT("SnakeAIControllerClass is not set."));
		}
		else
		{
			FActorSpawnParameters AIParams;
			AIParams.Owner = this;
			AIParams.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AController* AIController = GetWorld()->SpawnActor<AController>(
				SnakeAIControllerClass,
				SpawnLocation,
				FRotator::ZeroRotator,
				AIParams);

			if (AIController)
			{
				AIController->Possess(NewSnake);
				UE_LOG(LogTemp, Warning, TEXT("AI possessed snake for slot %d."), SlotIndex);
			}
		}
	}

	SpawnedSnakes.Add(NewSnake);
	return NewSnake;
}

void ASnakeGameMode::SpawnBattleSnakes()
{
	if (!GridManager)
	{
		return;
	}

	const int32 MidY = GridManager->GridDimensions.Y / 2;

	const FIntPoint PlayerSpawnCell(2, MidY);
	const FIntPoint OpponentSpawnCell(GridManager->GridDimensions.X - 3, MidY);

	ASnakePawn* PlayerSnake = SpawnSnakeForSlot(0, PlayerSpawnCell, Slot0Type);
	ASnakePawn* OpponentSnake = SpawnSnakeForSlot(1, OpponentSpawnCell, Slot1Type);
}

TArray<FIntPoint> ASnakeGameMode::GetAllSnakeOccupiedCells() const
{
	TArray<FIntPoint> Occupied;

	for (const ASnakePawn* Snake : SpawnedSnakes)
	{
		if (!Snake)
		{
			continue;
		}

		Occupied.Append(Snake->GetAllOccupiedGridCells());
	}

	return Occupied;
}

bool ASnakeGameMode::IsCellOccupiedByOtherSnake(
	const ASnakePawn* AskingSnake,
	const FIntPoint& Cell) const
{
	for (const ASnakePawn* Snake : SpawnedSnakes)
	{
		if (!Snake || Snake == AskingSnake)
		{
			continue;
		}

		if (Snake->GetAllOccupiedGridCells().Contains(Cell))
		{
			return true;
		}
	}

	return false;
}

void ASnakeGameMode::DestroySpawnedSnakes()
{
	for (ASnakePawn* Snake : SpawnedSnakes)
	{
		if (Snake)
		{
			Snake->Destroy();
		}
	}

	SpawnedSnakes.Empty();
	SpawnedSnakePawn = nullptr;
}

void ASnakeGameMode::SpawnSinglePlayerSnake()
{
	if (!GridManager)
	{
		return;
	}

	const FIntPoint SpawnCell(
		GridManager->GridDimensions.X / 2,
		GridManager->GridDimensions.Y / 2);

	ASnakePawn* Snake = SpawnSnakeForSlot(0, SpawnCell, ESnakePlayerSlotType::Human);

	SpawnedSnakePawn = Snake;
}

void ASnakeGameMode::SpawnCoopSnakes()
{
	if (!GridManager)
	{
		return;
	}

	const int32 MidY = GridManager->GridDimensions.Y / 2;

	const FIntPoint Player1SpawnCell(2, MidY);
	const FIntPoint Player2SpawnCell(GridManager->GridDimensions.X - 3, MidY);

	SpawnSnakeForSlot(0, Player1SpawnCell, Slot0Type);
	SpawnSnakeForSlot(1, Player2SpawnCell, Slot1Type);
}

APlayerController* ASnakeGameMode::GetOrCreateLocalPlayerController(int32 SlotIndex)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, SlotIndex);

	if (!PC && SlotIndex > 0)
	{
		PC = UGameplayStatics::CreatePlayer(this, SlotIndex, true);

		UE_LOG(LogTemp, Warning,
			TEXT("Created local player %d. Controller: %s"),
			SlotIndex,
			PC ? *PC->GetName() : TEXT("None"));
	}
	
	// Temporary logs
	for (int32 i = 0; i < 2; ++i)
	{
		APlayerController* PCTest = UGameplayStatics::GetPlayerController(this, i);
		UE_LOG(LogTemp, Warning,
			TEXT("Controller %d = %s, Pawn = %s"),
			i,
			PCTest ? *PCTest->GetName() : TEXT("None"),
			(PCTest && PCTest->GetPawn()) ? *PCTest->GetPawn()->GetName() : TEXT("None"));
	}

	return PC;
}
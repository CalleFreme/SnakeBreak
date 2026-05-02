// Start/restart run, load stages, spawn snake/food, handle food eaten/death, tell GameState about match result

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SnakeGameTypes.h"
#include "SnakeGameMode.generated.h"

class ASnakePawn;
class AFoodActor;
class AHazard;
class AGridManagerActor;
class ASnakeGameState;
class UAudioComponent;
class USoundBase;
struct FHazardSpawnData;
struct FSnakeStageConfig;

/**
 * 
 */
UCLASS()
class SNAKEBREAK_API ASnakeGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Snake")
	void StartPlayingRun();

	UFUNCTION(BlueprintCallable, Category = "Snake")
	void ReturnToMainMenu();
	
	UFUNCTION(BlueprintCallable, Category = "Snake")
	void RestartRun();
	
	ASnakePawn* SpawnSnakeForSlot(int32 SlotIndex, const FIntPoint& SpawnCell, ESnakePlayerSlotType SlotType);
	void SpawnBattleSnakes();
	TArray<FIntPoint> GetAllSnakeOccupiedCells() const;
	
	bool IsCellOccupiedByOtherSnake(const ASnakePawn* AskingSnake, const FIntPoint& Cell) const;
	bool IsCellReachableByOtherSnakeHead(const ASnakePawn* AskingSnake, const FIntPoint& Cell) const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Snake")
	TSubclassOf<ASnakePawn> SnakePawnClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Snake")
	TSubclassOf<AFoodActor> FoodActorClass;

	UPROPERTY()
	TObjectPtr<ASnakePawn> SpawnedSnakePawn;
	
	// Need: All spawned snakes, the AI player controller, track if battle mode, player slots
	UPROPERTY()
	TArray<TObjectPtr<ASnakePawn>> SpawnedSnakes;
	
	UPROPERTY(EditDefaultsOnly, Category = "Snake|AI")
	TSubclassOf<AController> SnakeAIControllerClass;
	
	UPROPERTY(EditAnywhere, Category = "Snake|Mode")
	bool bUseBattleMode = true;
	
	UPROPERTY(EditAnywhere, Category = "Snake|Battle")
	ESnakePlayerSlotType Slot0Type = ESnakePlayerSlotType::Human;

	UPROPERTY(EditAnywhere, Category = "Snake|Battle")
	ESnakePlayerSlotType Slot1Type = ESnakePlayerSlotType::AI;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snake|Mode", meta = (AllowPrivateAccess = "true"))
	ESnakeGameModeType ActiveGameMode = ESnakeGameModeType::NormalSinglePlayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snake|Battle", meta = (AllowPrivateAccess = "true"))
	ESnakeBattleResult BattleResult = ESnakeBattleResult::None;
	
	UPROPERTY()
	TObjectPtr<AFoodActor> SpawnedFoodActor;

	UPROPERTY()
	TArray<TObjectPtr<AHazard>> SpawnedHazards;
	
	UPROPERTY()
	TObjectPtr<AGridManagerActor> GridManager;
	
	UPROPERTY(EditAnywhere, Category = "Stages")
	TArray<FSnakeStageConfig> Stages;

	UPROPERTY(EditDefaultsOnly, Category = "Snake|Audio")
	TObjectPtr<USoundBase> HumanEatSound;

	UPROPERTY(EditDefaultsOnly, Category = "Snake|Audio")
	TObjectPtr<USoundBase> AIEatSound;

	UPROPERTY(EditDefaultsOnly, Category = "Snake|Audio")
	TObjectPtr<USoundBase> CollisionSound;

	UPROPERTY(EditDefaultsOnly, Category = "Snake|Audio")
	TObjectPtr<USoundBase> AmbientMusic;

	UPROPERTY()
	TObjectPtr<UAudioComponent> AmbientMusicComponent;
	
	int32 CurrentStageIndex = 0;
	int32 FoodEatenThiStage = 0;

	void CacheGridManager();
	void SpawnSnake();
	void SpawnFood();
	void MoveFoodToRandomFreeCell();
	void SpawnHazards(const FSnakeStageConfig& Config);
	bool ValidateHazardSpawnData(const FHazardSpawnData& HazardData, int32 HazardIndex) const;
	TArray<FIntPoint> GetValidatedPatrolCells(const FHazardSpawnData& HazardData, const AHazard* SpawnedHazard) const;
	void ClearHazards();
	void LoadStage(int32 StageIndex);
	void AdvanceStage();
	void DestroySpawnedSnakes();
	void SpawnSinglePlayerSnake();
	void SpawnCoopSnakes();

	UFUNCTION()
	void HandleFoodConsumed(ASnakePawn* EatingSnake, int32 ScoreValue);

	UFUNCTION()
	void HandleSnakeDied(ASnakePawn* DeadSnake);

	UFUNCTION()
	void ChangePhase(const ESnakeMatchPhase NewPhase);

	UFUNCTION()
	void HandleAmbientMusicFinished();

	void PlayEatSound(ASnakePawn* EatingSnake) const;
	void PlayCollisionSound(ASnakePawn* DeadSnake) const;
	void StartAmbientMusic();
	void StopAmbientMusic();
	
	ASnakeGameState* GetSnakeGameState();
	
	APlayerController* GetOrCreateLocalPlayerController(int32 SlotIndex);
};

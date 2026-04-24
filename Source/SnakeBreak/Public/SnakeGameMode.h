// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SnakeGameMode.generated.h"

class ASnakePawn;
class AFoodActor;
class AGridManagerActor;
class ASnakeGameState;
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

	UFUNCTION(BlueprintCallable, Category = "Snake")
	void StartPlayingRun();

	UFUNCTION(BlueprintCallable, Category = "Snake")
	void ReturnToMainMenu();
	
	UFUNCTION(BlueprintCallable, Category = "Snake")
	void RestartRun();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Snake")
	TSubclassOf<ASnakePawn> SnakePawnClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Snake")
	TSubclassOf<AFoodActor> FoodActorClass;

	UPROPERTY()
	TObjectPtr<ASnakePawn> SpawnedSnakePawn;
	
	UPROPERTY()
	TObjectPtr<AFoodActor> SpawnedFoodActor;

	UPROPERTY()
	TObjectPtr<AGridManagerActor> GridManager;
	
	UPROPERTY(EditAnywhere, Category = "Stages")
	TArray<FSnakeStageConfig> Stages;
	
	int32 CurrentStageIndex = 0;
	int32 FoodEatenThiStage = 0;

	void CacheGridManager();
	void SpawnSnake();
	void SpawnFood();
	void MoveFoodToRandomFreeCell();
	void LoadStage(int32 StageIndex);
	void AdvanceStage();

	UFUNCTION()
	void HandleFoodConsumed(int32 ScoreValue);

	UFUNCTION()
	void HandleSnakeDied();

	UFUNCTION()
	void ChangePhase(const ESnakeMatchPhase NewPhase);
	
	ASnakeGameState* GetSnakeGameState();
};

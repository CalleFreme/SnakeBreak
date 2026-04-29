// Score, current phase, current stage info
// shared or per-player score, battle winner, data needed by HUD/Outro

#pragma once

#include "CoreMinimal.h"
#include "SnakeGameTypes.h"
#include "GameFramework/GameStateBase.h"
#include "SnakeGameState.generated.h"

UENUM(BlueprintType)
enum class ESnakeMatchPhase : uint8
{
	MainMenu,
	Playing,
	Outro,
	None
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChangedSignature, ESnakeMatchPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreChangedSignature, int32, NewScore);

/**
 * 
 */
UCLASS()
class SNAKEBREAK_API ASnakeGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, Category = "Snake")
	int32 Score = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Snake")
	int32 CurrentStageIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Snake")
	int32 FoodEatenThisStage = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Snake")
	int32 FoodToClearStage = 1;
	
	// Use BlueprintAssignable so the HUD can bind to this
	UPROPERTY(BlueprintAssignable, Category = "Snake")
	FOnPhaseChangedSignature OnPhaseChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Snake")
	FOnScoreChangedSignature OnScoreChanged;

	UPROPERTY(BlueprintReadOnly, Category = "Snake|Battle")
	ESnakeBattleResult BattleResult = ESnakeBattleResult::None;
	
	void AddScore(int32 Amount)
	{
		Score += Amount;
		OnScoreChanged.Broadcast(Score);
	}
	
	void SetMatchPhase(ESnakeMatchPhase NewPhase)
	{
		MatchPhase = NewPhase;
		OnPhaseChanged.Broadcast(NewPhase);
	}
	
	UPROPERTY(BlueprintReadOnly, Category = "Snake")
	ESnakeMatchPhase MatchPhase = ESnakeMatchPhase::MainMenu;
};

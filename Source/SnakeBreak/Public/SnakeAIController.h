// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SnakePawn.h"
#include "SnakeAIController.generated.h"

class AFoodActor;
class AGridManagerActor;

UENUM(BlueprintType)
enum class ESnakeAIMode : uint8
{
	GreedySafe,
	GridAStar,
	FreeMovingNav
};

UCLASS()
class SNAKEBREAK_API ASnakeAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASnakeAIController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(EditAnywhere, Category = "Snake|AI")
	ESnakeAIMode AIMode = ESnakeAIMode::GreedySafe;

	UPROPERTY(EditAnywhere, Category = "Snake|AI")
	bool bDrawDebugAI = false;

	UPROPERTY()
	TObjectPtr<ASnakePawn> ControlledSnake;

	UPROPERTY()
	TObjectPtr<AGridManagerActor> GridManager;

	UPROPERTY()
	TObjectPtr<AFoodActor> FoodActor;

	UPROPERTY(EditAnywhere, Category = "Snake|AI", meta = (ClampMin = "0"))
	int32 RecentCellMemory = 8;

	UPROPERTY(EditAnywhere, Category = "Snake|AI", meta = (ClampMin = "1"))
	int32 MinimumReachableSpaceBuffer = 4;

	TArray<FIntPoint> RecentHeadCells;
	FIntPoint LastRememberedHeadCell = FIntPoint(INDEX_NONE, INDEX_NONE);

	void CacheWorldReferences();

	void UpdateAI(float DeltaSeconds);
	void DecideGreedySafe();
	void DecideGridAStar();
	void DecideFreeMovingNav();

	bool IsCellSafe(const FIntPoint& Cell) const;
	bool IsCellReachableAfterMove(const FIntPoint& Cell, const TSet<FIntPoint>& ProjectedSelfCells) const;
	int32 CountReachableCellsAfterMove(const FIntPoint& NextCell) const;
	int32 GetRecentCellPenalty(const FIntPoint& Cell) const;
	int32 GetContestedCellPenalty(const FIntPoint& Cell) const;
	int32 GetTurnPenalty(ESnakeDirection Direction) const;
	void RememberCurrentHeadCell();
	TArray<ESnakeDirection> GetCandidateDirections() const;
	ESnakeDirection DirectionFromStep(const FIntPoint& From, const FIntPoint& To) const;
	
	bool HasEscapeMoveFrom(const FIntPoint& Cell) const;
};

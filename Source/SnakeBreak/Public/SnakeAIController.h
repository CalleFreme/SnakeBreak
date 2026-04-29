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
	bool bDrawDebugAI = true;

	UPROPERTY()
	TObjectPtr<ASnakePawn> ControlledSnake;

	UPROPERTY()
	TObjectPtr<AGridManagerActor> GridManager;

	UPROPERTY()
	TObjectPtr<AFoodActor> FoodActor;

	float DecisionCooldown = 0.f;

	void CacheWorldReferences();

	void UpdateAI(float DeltaSeconds);
	void DecideGreedySafe();
	void DecideGridAStar();
	void DecideFreeMovingNav();

	bool IsCellSafe(const FIntPoint& Cell) const;
	TArray<ESnakeDirection> GetCandidateDirections() const;
	ESnakeDirection DirectionFromStep(const FIntPoint& From, const FIntPoint& To) const;
	
	bool HasEscapeMoveFrom(const FIntPoint& Cell) const;
};

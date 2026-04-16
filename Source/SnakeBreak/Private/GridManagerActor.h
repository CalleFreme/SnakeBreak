// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridManagerActor.generated.h"

UCLASS()
class AGridManagerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGridManagerActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Grid")
	FVector GridToWorld(const FIntPoint& GridCell) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsInsidePlayableArea(const FIntPoint& GridCell) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool IsBlockedCell(const FIntPoint& GridCell) const;

	UFUNCTION(BlueprintPure, Category = "Grid")
	bool TryGetRandomFreeCell(FIntPoint& OutCell, const TArray<FIntPoint>& ForbiddenCells, int32 MaxAttempts = 200) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> FloorInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> WallInstances;


	void RebuildGrid();
	void RebuildBlockedCells();
	void RebuildVisualInstances();
	void ClearVisualInstances();

public:	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	FIntPoint GridDimensions = FIntPoint(20, 20);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	float CellSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	FVector GridOrigin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation")
	bool bGenerateBorderWalls = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generation")
	bool bGenerateFloor = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	float WallZOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	float FloorZOffset = -5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector InstanceScale = FVector(1.f, 1.f, 1.f);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid", meta = (AllowPrivateAccess))
	TSet<FIntPoint> BlockedCells;


};

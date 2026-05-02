#pragma once

#include "CoreMinimal.h"
#include "SnakeStageConfig.generated.h"

class AHazard;

USTRUCT(BlueprintType)
struct FHazardSpawnData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	TSubclassOf<AHazard> HazardClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	FIntPoint SpawnCell = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	FRotator Rotation = FRotator::ZeroRotator;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	TArray<FIntPoint> PatrolCells;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "1.0"))
	float MovementSpeed = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.0"))
	float PatrolWaitSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool bAutoStartPatrol = true;
	
	// Laser walls are always active when this is 0. Positive values toggle on/off at this interval.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser", meta = (ClampMin = "0.0"))
	float ToggleRateSeconds = 4.f;
};

USTRUCT(BlueprintType)
struct FSnakeStageConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FIntPoint GridDimensions = FIntPoint(20, 20);
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CellSize = 100.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MoveStepTime = 0.2f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 FoodToClear = 3; // Our win condition!
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FHazardSpawnData> Hazards;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Pattern = 0; // 0 border, 1 lanes, 2 cross/maze
};

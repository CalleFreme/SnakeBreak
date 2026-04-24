#pragma once

#include "CoreMinimal.h"
#include "SnakeStageConfig.generated.h"

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
	int32 Pattern = 0; // 0 border, 1 lanes, 2 cross/maze
};
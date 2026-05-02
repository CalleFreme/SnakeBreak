#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HazardTarget.generated.h"

class AHazard;

UINTERFACE(BlueprintType)
class SNAKEBREAK_API UHazardTarget : public UInterface
{
	GENERATED_BODY()
};

class SNAKEBREAK_API IHazardTarget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hazard")
	void EliminateByHazard(AHazard* Hazard);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hazard")
	void TrimTailByHazard(AHazard* Hazard, int32 HitSegmentIndex);
};

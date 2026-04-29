#pragma once

#include "CoreMinimal.h"
#include "SnakeGameTypes.generated.h"

UENUM(BlueprintType)
enum class ESnakeGameModeType : uint8
{
	NormalSinglePlayer,
	Coop,
	Battle
};

UENUM(BlueprintType)
enum class ESnakePlayerSlotType : uint8
{
	Human,
	AI
};

UENUM(BlueprintType)
enum class ESnakeBattleResult : uint8
{
	None,
	Player0Won,
	Player1Won,
	Draw
};

// Extend to Classic, Coop, Battle modes
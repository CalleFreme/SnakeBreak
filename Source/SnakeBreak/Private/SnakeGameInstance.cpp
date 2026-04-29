#include "SnakeGameInstance.h"

void USnakeGameInstance::ConfigureNormalSinglePlayer()
{
	SelectedGameMode = ESnakeGameModeType::NormalSinglePlayer;
	Slot0Type = ESnakePlayerSlotType::Human;
	Slot1Type = ESnakePlayerSlotType::AI;
}

void USnakeGameInstance::ConfigureBattleVsAI()
{
	SelectedGameMode = ESnakeGameModeType::Battle;
	Slot0Type = ESnakePlayerSlotType::Human;
	Slot1Type = ESnakePlayerSlotType::AI;
}

void USnakeGameInstance::ConfigureBattleLocal()
{
	SelectedGameMode = ESnakeGameModeType::Battle;
	Slot0Type = ESnakePlayerSlotType::Human;
	Slot1Type = ESnakePlayerSlotType::Human;
}

void USnakeGameInstance::ConfigureCoopLocal()
{
	SelectedGameMode = ESnakeGameModeType::Coop;
	Slot0Type = ESnakePlayerSlotType::Human;
	Slot1Type = ESnakePlayerSlotType::Human;
}
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SnakeGameTypes.h"
#include "SnakeGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class SNAKEBREAK_API USnakeGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "Snake|Mode")
	ESnakeGameModeType SelectedGameMode = ESnakeGameModeType::NormalSinglePlayer;

	UPROPERTY(BlueprintReadWrite, Category = "Snake|Mode")
	ESnakePlayerSlotType Slot0Type = ESnakePlayerSlotType::Human;

	UPROPERTY(BlueprintReadWrite, Category = "Snake|Mode")
	ESnakePlayerSlotType Slot1Type = ESnakePlayerSlotType::AI;

	UFUNCTION(BlueprintCallable, Category = "Snake|Mode")
	void ConfigureNormalSinglePlayer();

	UFUNCTION(BlueprintCallable, Category = "Snake|Mode")
	void ConfigureBattleVsAI();

	UFUNCTION(BlueprintCallable, Category = "Snake|Mode")
	void ConfigureBattleLocal();

	UFUNCTION(BlueprintCallable, Category = "Snake|Mode")
	void ConfigureCoopLocal();
};

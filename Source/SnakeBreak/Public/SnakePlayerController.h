// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SnakeGameMode.h"
#include "GameFramework/PlayerController.h"
#include "SnakePlayerController.generated.h"

class UUserWidget;

/**
 * 
 */
UCLASS()
class SNAKEBREAK_API ASnakePlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	void ShowHUD();

	UFUNCTION(BlueprintCallable)
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable)
	void ShowOutro();

	UFUNCTION(BlueprintCallable)
	void HideAllWidgets();
	
	UFUNCTION(BlueprintCallable)
	void RequestStartGame();
	
	UFUNCTION(BlueprintCallable)
	void RequestRestartFromUI()
	{
		if (ASnakeGameMode* GM = GetWorld()->GetAuthGameMode<ASnakeGameMode>())
		{
			GM->RestartRun();
		}
	}
	
	UFUNCTION(BlueprintCallable)
	void RequestReturnToMenuFromUI()
	{
		if (ASnakeGameMode* GM = GetWorld()->GetAuthGameMode<ASnakeGameMode>())
		{
			GM->ReturnToMainMenu();
		}
	}

protected:
	UFUNCTION(BlueprintCallable)
	void HandlePhaseChanged(ESnakeMatchPhase Phase);
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> OutroWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveWidget;
};

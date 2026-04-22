// Fill out your copyright notice in the Description page of Project Settings.


#include "SnakePlayerController.h"

#include "SnakeGameState.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

void ASnakePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	// Bind to the GameState event. We want to react on phase change only when it happens.
	if (ASnakeGameState* GS = GetWorld()->GetGameState<ASnakeGameState>())
	{
		GS->OnPhaseChanged.AddDynamic(this, &ASnakePlayerController::HandlePhaseChanged);
		
		// If the game is already in a state, manually trigger handler once to catch up
		if (GS->MatchPhase != ESnakeMatchPhase::None)
		{
			HandlePhaseChanged(GS->MatchPhase);
		}
	}
}

void ASnakePlayerController::RequestStartGame()
{
	// Should use a TSoftObjectPtr<UWorld> member to pick level from a dropdown in editor
	UGameplayStatics::OpenLevel(this, FName(TEXT("TestGridLevel")));
}

void ASnakePlayerController::HandlePhaseChanged(ESnakeMatchPhase Phase)
{
	switch (Phase)
	{
	case ESnakeMatchPhase::Playing:
		{
			UGameplayStatics::SetGamePaused(GetWorld(), false);
			ShowHUD();
			break;
		}
	case ESnakeMatchPhase::Outro:
		{
			UGameplayStatics::SetGamePaused(GetWorld(), true);
			ShowOutro();
			break;
		}
	case ESnakeMatchPhase::MainMenu:
		{
			UGameplayStatics::SetGamePaused(GetWorld(), false);
			ShowMainMenu();
			break;
		}
	case ESnakeMatchPhase::None:
		break;
	}
}

void ASnakePlayerController::HideAllWidgets()
{
	if (ActiveWidget)
	{
		ActiveWidget->RemoveFromParent();
		ActiveWidget = nullptr;
	}
}

void ASnakePlayerController::ShowHUD()
{
	HideAllWidgets();
	if (!HUDWidgetClass) return;

	ActiveWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
	if (ActiveWidget)
	{
		ActiveWidget->AddToViewport();
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
}

void ASnakePlayerController::ShowMainMenu()
{
	HideAllWidgets();
	if (!MainMenuWidgetClass) return;

	ActiveWidget = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
	if (ActiveWidget)
	{
		ActiveWidget->AddToViewport();
	}

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ASnakePlayerController::ShowOutro()
{
	HideAllWidgets();
	if (!OutroWidgetClass) return;

	ActiveWidget = CreateWidget<UUserWidget>(this, OutroWidgetClass);
	if (ActiveWidget)
	{
		ActiveWidget->AddToViewport();
	}

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}


// Fill out your copyright notice in the Description page of Project Settings.


#include "SnakePlayerController.h"
#include "Blueprint/UserWidget.h"

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


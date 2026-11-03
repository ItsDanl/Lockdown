#include "Player/LockdownPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Core/LockdownGameMode.h"
#include "Engine/World.h"
#include "UI/EndScreenWidget.h"
#include "UI/HUDWidget.h"
#include "UI/PauseMenuWidget.h"

namespace
{
	/** Above the HUD (default z 0) so menus always cover it. */
	constexpr int32 MenuZOrder = 100;
}

ALockdownPlayerController::ALockdownPlayerController()
{
	HUDWidgetClass = UHUDWidget::StaticClass();
	PauseMenuWidgetClass = UPauseMenuWidget::StaticClass();
	EndScreenWidgetClass = UEndScreenWidget::StaticClass();
}

void ALockdownPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Keep ticking input while paused so Escape can resume the game.
	bShouldPerformFullTickWhenPaused = true;

	// Only the real match gets a HUD; this class could be reused in other maps.
	ALockdownGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALockdownGameMode>() : nullptr;
	if (!IsLocalController() || !GameMode)
	{
		return;
	}

	GameMode->OnMatchEnded.AddDynamic(this, &ALockdownPlayerController::HandleMatchEnded);
	CreateHUD();
	SetGameInputMode();
}

void ALockdownPlayerController::CreateHUD()
{
	if (!HUDWidgetClass)
	{
		return;
	}
	HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport();
	}
}

void ALockdownPlayerController::TogglePause()
{
	if (EndScreenWidget)
	{
		return; // Match is over; the end screen owns input.
	}
	if (PauseMenuWidget)
	{
		ClosePauseMenu();
	}
	else
	{
		OpenPauseMenu();
	}
}

void ALockdownPlayerController::OpenPauseMenu()
{
	if (!PauseMenuWidgetClass)
	{
		return;
	}
	PauseMenuWidget = CreateWidget<UPauseMenuWidget>(this, PauseMenuWidgetClass);
	if (!PauseMenuWidget)
	{
		return;
	}
	PauseMenuWidget->AddToViewport(MenuZOrder);
	SetPause(true);
	SetUIInputMode(true);
	PauseMenuWidget->FocusDefaultButton(); // Lets Escape reach the widget to resume.
}

void ALockdownPlayerController::ClosePauseMenu()
{
	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
		PauseMenuWidget = nullptr;
	}
	SetPause(false);
	SetGameInputMode();
}

void ALockdownPlayerController::HandleMatchEnded(EMatchResult Result)
{
	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
		PauseMenuWidget = nullptr;
	}
	if (EndScreenWidgetClass)
	{
		EndScreenWidget = CreateWidget<UEndScreenWidget>(this, EndScreenWidgetClass);
	}
	if (EndScreenWidget)
	{
		EndScreenWidget->SetResult(Result);
		EndScreenWidget->AddToViewport(MenuZOrder);
	}
	SetUIInputMode(false);
}

void ALockdownPlayerController::SetGameInputMode()
{
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void ALockdownPlayerController::SetUIInputMode(bool bAllowGameInput)
{
	if (bAllowGameInput)
	{
		// Game+UI keeps the Esc binding alive for the character while the cursor is free.
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
	else
	{
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
	bShowMouseCursor = true;
}

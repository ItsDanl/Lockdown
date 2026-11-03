#include "Core/MainMenuGameMode.h"

#include "Player/MainMenuPlayerController.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	// No pawn: the menu is pure UI over a blank view.
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AMainMenuPlayerController::StaticClass();
}

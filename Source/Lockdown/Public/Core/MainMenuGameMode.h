#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

/** Pawn-less game mode for the main menu map; AMainMenuPlayerController builds the UI. */
UCLASS()
class LOCKDOWN_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();
};

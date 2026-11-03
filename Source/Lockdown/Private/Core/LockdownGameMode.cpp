#include "Core/LockdownGameMode.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Player/LockdownCharacter.h"
#include "Player/LockdownPlayerController.h"
#include "TimerManager.h"
#include "World/Terminal.h"

ALockdownGameMode::ALockdownGameMode()
{
	DefaultPawnClass = ALockdownCharacter::StaticClass();
	PlayerControllerClass = ALockdownPlayerController::StaticClass();
}

void ALockdownGameMode::BeginPlay()
{
	Super::BeginPlay();

	CountTerminals();
	GetWorldTimerManager().SetTimer(TimeLimitHandle, this, &ALockdownGameMode::OnTimeExpired, TimeLimit, false);
}

void ALockdownGameMode::CountTerminals()
{
	TerminalsTotal = 0;
	for (TActorIterator<ATerminal> It(GetWorld()); It; ++It)
	{
		++TerminalsTotal;
	}
	TerminalsRemaining = TerminalsTotal;
}

float ALockdownGameMode::GetTimeRemaining() const
{
	if (bMatchEnded)
	{
		return FinalTimeRemaining;
	}
	if (!TimeLimitHandle.IsValid())
	{
		return TimeLimit;
	}
	// GetTimerRemaining returns -1 for an invalid timer; clamp so the HUD never shows negatives.
	return FMath::Max(0.f, GetWorldTimerManager().GetTimerRemaining(TimeLimitHandle));
}

void ALockdownGameMode::OnTerminalDestroyed(ATerminal* Terminal)
{
	if (bMatchEnded)
	{
		return;
	}
	if (Terminal)
	{
		bool bAlreadyCounted = false;
		DestroyedTerminals.Add(FObjectKey(Terminal), &bAlreadyCounted);
		if (bAlreadyCounted)
		{
			return;
		}
	}

	TerminalsRemaining = FMath::Max(0, TerminalsRemaining - 1);
	OnTerminalCountChanged.Broadcast(TerminalsRemaining, TerminalsTotal);

	if (TerminalsRemaining == 0)
	{
		EndMatch(EMatchResult::Won);
	}
}

void ALockdownGameMode::OnPlayerDied()
{
	EndMatch(EMatchResult::LostHealth);
}

void ALockdownGameMode::OnTimeExpired()
{
	EndMatch(EMatchResult::LostTime);
}

void ALockdownGameMode::EndMatch(EMatchResult Result)
{
	if (bMatchEnded)
	{
		return;
	}

	FinalTimeRemaining = GetTimeRemaining();
	bMatchEnded = true;
	MatchResult = Result;
	GetWorldTimerManager().ClearTimer(TimeLimitHandle);

	// Controller reacts first (shows end screen), then we freeze the world behind it.
	OnMatchEnded.Broadcast(Result);
	UGameplayStatics::SetGamePaused(this, true);
}

void ALockdownGameMode::RestartLevel()
{
	UGameplayStatics::SetGamePaused(this, false);
	const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*CurrentLevel));
}

void ALockdownGameMode::ReturnToMainMenu()
{
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, MainMenuMapName);
}

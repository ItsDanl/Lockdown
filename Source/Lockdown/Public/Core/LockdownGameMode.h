#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UObject/ObjectKey.h"
#include "Core/LockdownTypes.h"
#include "LockdownGameMode.generated.h"

class ATerminal;

/** Fired once when the match ends (win, death or timeout). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchEnded, EMatchResult, Result);
/** Fired whenever a terminal is destroyed; lets the HUD show "SECTOR UNLOCKED". */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTerminalCountChanged, int32, Remaining, int32, Total);

/** Facility match rules: countdown, terminal objective, win/lose resolution. */
UCLASS()
class LOCKDOWN_API ALockdownGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALockdownGameMode();

	/** Called by ATerminal when it is destroyed. */
	UFUNCTION(BlueprintCallable, Category = "Lockdown|Match")
	void OnTerminalDestroyed(ATerminal* Terminal);

	/** Called by ALockdownCharacter on death. */
	UFUNCTION(BlueprintCallable, Category = "Lockdown|Match")
	void OnPlayerDied();

	UFUNCTION(BlueprintCallable, Category = "Lockdown|Match")
	void RestartLevel();

	UFUNCTION(BlueprintCallable, Category = "Lockdown|Match")
	void ReturnToMainMenu();

	UFUNCTION(BlueprintPure, Category = "Lockdown|Match")
	float GetTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Lockdown|Match")
	int32 GetTerminalsRemaining() const { return TerminalsRemaining; }

	UFUNCTION(BlueprintPure, Category = "Lockdown|Match")
	int32 GetTerminalsTotal() const { return TerminalsTotal; }

	UFUNCTION(BlueprintPure, Category = "Lockdown|Match")
	EMatchResult GetMatchResult() const { return MatchResult; }

	UPROPERTY(BlueprintAssignable, Category = "Lockdown|Match")
	FOnMatchEnded OnMatchEnded;

	UPROPERTY(BlueprintAssignable, Category = "Lockdown|Match")
	FOnTerminalCountChanged OnTerminalCountChanged;

protected:
	virtual void BeginPlay() override;

	/** Total seconds before the facility self-destructs. */
	UPROPERTY(EditDefaultsOnly, Category = "Lockdown|Match", meta = (ClampMin = "1"))
	float TimeLimit = 300.f;

	/** Map opened by ReturnToMainMenu. */
	UPROPERTY(EditDefaultsOnly, Category = "Lockdown|Match")
	FName MainMenuMapName = TEXT("MainMenu");

private:
	void CountTerminals();
	void OnTimeExpired();
	/** Resolves the match once; later calls are ignored so death-at-timeout can't double fire. */
	void EndMatch(EMatchResult Result);

	FTimerHandle TimeLimitHandle;
	int32 TerminalsTotal = 0;
	int32 TerminalsRemaining = 0;
	float FinalTimeRemaining = 0.f;
	bool bMatchEnded = false;
	EMatchResult MatchResult = EMatchResult::None;

	/** Terminals already counted, to guard against duplicate notifications. */
	TSet<FObjectKey> DestroyedTerminals;
};

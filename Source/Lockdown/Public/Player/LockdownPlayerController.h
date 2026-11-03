#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/LockdownTypes.h"
#include "LockdownPlayerController.generated.h"

class UHUDWidget;
class UPauseMenuWidget;
class UEndScreenWidget;

/** Owns the in-match UI: HUD, pause menu and end screen. */
UCLASS()
class LOCKDOWN_API ALockdownPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALockdownPlayerController();

	/** Opens or closes the pause menu (no-op once the match has ended). */
	UFUNCTION(BlueprintCallable, Category = "Lockdown|UI")
	void TogglePause();

	UHUDWidget* GetHUDWidget() const { return HUDWidget; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Lockdown|UI")
	TSubclassOf<UHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lockdown|UI")
	TSubclassOf<UPauseMenuWidget> PauseMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Lockdown|UI")
	TSubclassOf<UEndScreenWidget> EndScreenWidgetClass;

private:
	UFUNCTION()
	void HandleMatchEnded(EMatchResult Result);

	void CreateHUD();
	void OpenPauseMenu();
	void ClosePauseMenu();
	void SetGameInputMode();
	void SetUIInputMode(bool bAllowGameInput);

	UPROPERTY(Transient)
	TObjectPtr<UHUDWidget> HUDWidget;

	UPROPERTY(Transient)
	TObjectPtr<UPauseMenuWidget> PauseMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UEndScreenWidget> EndScreenWidget;
};

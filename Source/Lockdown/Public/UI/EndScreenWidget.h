#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/LockdownTypes.h"
#include "EndScreenWidget.generated.h"

class UTextBlock;
class UButton;

/** Win/lose screen: result headline, Restart and Main Menu. Built entirely in C++. */
UCLASS()
class LOCKDOWN_API UEndScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Sets headline text/colour for the given result. Safe to call before or after the widget is shown. */
	void SetResult(EMatchResult InResult);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UFUNCTION()
	void OnRestartClicked();
	UFUNCTION()
	void OnMainMenuClicked();

	void BuildLayout(class UCanvasPanel* Root);
	void RefreshTexts();

	EMatchResult Result = EMatchResult::None;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> MainMenuButton;
};

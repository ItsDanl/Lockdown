#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;

/** Pause overlay: Resume, Restart, Main Menu. Built entirely in C++. */
UCLASS()
class LOCKDOWN_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Gives the Resume button keyboard focus so Escape key events reach this widget. */
	void FocusDefaultButton();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	UFUNCTION()
	void OnResumeClicked();
	UFUNCTION()
	void OnRestartClicked();
	UFUNCTION()
	void OnMainMenuClicked();

	void BuildLayout(class UCanvasPanel* Root);

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResumeButton;
};

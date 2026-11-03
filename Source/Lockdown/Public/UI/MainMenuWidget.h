#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UBorder;

/** Title screen: Start Game, Instructions (toggle), Quit. Built entirely in C++. */
UCLASS()
class LOCKDOWN_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Map opened by Start Game. */
	UPROPERTY(EditDefaultsOnly, Category = "Lockdown|UI")
	FName GameMapName = TEXT("Facility");

private:
	UFUNCTION()
	void OnStartClicked();
	UFUNCTION()
	void OnInstructionsClicked();
	UFUNCTION()
	void OnQuitClicked();

	void BuildLayout(class UCanvasPanel* Root);
	class UBorder* BuildInstructionsPanel();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> InstructionsPanel;
};

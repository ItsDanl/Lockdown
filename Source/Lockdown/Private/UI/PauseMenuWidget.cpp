#include "UI/PauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"
#include "Core/LockdownGameMode.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Player/LockdownPlayerController.h"
#include "UI/LockdownUIStyle.h"

TSharedRef<SWidget> UPauseMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		BuildLayout(Root);
	}
	return Super::RebuildWidget();
}

void UPauseMenuWidget::BuildLayout(UCanvasPanel* Root)
{
	// Full-screen dimmer so the frozen scene recedes behind the menu.
	UBorder* Dim = LockdownUI::MakePanel(WidgetTree, FMargin(0.f), FLinearColor(0.f, 0.f, 0.f, 0.55f));
	LockdownUI::Fill(Root, Dim);

	UBorder* Panel = LockdownUI::MakePanel(WidgetTree, FMargin(48.f, 36.f), LockdownUI::PanelColor());
	LockdownUI::Place(Root, Panel, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->AddChild(Column);

	UTextBlock* Title = LockdownUI::MakeText(WidgetTree, TEXT("Paused"), 44, LockdownUI::Accent(), 10);
	LockdownUI::AddRow(Column, Title, FMargin(0.f, 0.f, 0.f, 28.f), HAlign_Center);

	ResumeButton = LockdownUI::MakeButton(WidgetTree, TEXT("Resume"), LockdownUI::Accent());
	UButton* RestartButton = LockdownUI::MakeButton(WidgetTree, TEXT("Restart"), LockdownUI::Warning());
	UButton* MenuButton = LockdownUI::MakeButton(WidgetTree, TEXT("Main Menu"), LockdownUI::Danger());

	ResumeButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResumeClicked);
	RestartButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnRestartClicked);
	MenuButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnMainMenuClicked);

	const FMargin ButtonGap(0.f, 6.f);
	LockdownUI::AddRow(Column, ResumeButton, ButtonGap);
	LockdownUI::AddRow(Column, RestartButton, ButtonGap);
	LockdownUI::AddRow(Column, MenuButton, ButtonGap);
}

void UPauseMenuWidget::FocusDefaultButton()
{
	if (ResumeButton)
	{
		ResumeButton->SetKeyboardFocus();
	}
}

FReply UPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// While paused, gameplay bindings are dead, so Escape must be handled here to resume.
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnResumeClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPauseMenuWidget::OnResumeClicked()
{
	if (ALockdownPlayerController* PC = Cast<ALockdownPlayerController>(GetOwningPlayer()))
	{
		PC->TogglePause();
	}
}

void UPauseMenuWidget::OnRestartClicked()
{
	if (ALockdownGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ALockdownGameMode>() : nullptr)
	{
		GM->RestartLevel();
	}
}

void UPauseMenuWidget::OnMainMenuClicked()
{
	if (ALockdownGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ALockdownGameMode>() : nullptr)
	{
		GM->ReturnToMainMenu();
	}
}

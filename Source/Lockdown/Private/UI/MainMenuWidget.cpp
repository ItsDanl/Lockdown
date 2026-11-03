#include "UI/MainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/LockdownUIStyle.h"

TSharedRef<SWidget> UMainMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		BuildLayout(Root);
	}
	return Super::RebuildWidget();
}

void UMainMenuWidget::BuildLayout(UCanvasPanel* Root)
{
	// Opaque backdrop: there is no pawn/camera in the menu map.
	UBorder* Backdrop = LockdownUI::MakePanel(WidgetTree, FMargin(0.f), FLinearColor(0.01f, 0.015f, 0.03f, 1.f));
	LockdownUI::Fill(Root, Backdrop);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	LockdownUI::Place(Root, Column, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

	UTextBlock* Title = LockdownUI::MakeText(WidgetTree, TEXT("Shutdown/Lockdown name TBA"), 96, LockdownUI::Accent(), 24);
	LockdownUI::AddRow(Column, Title, FMargin(0.f, 0.f, 0.f, 4.f), HAlign_Center);

	UTextBlock* Tagline = LockdownUI::MakeText(WidgetTree, TEXT("Destroy the terminals!"), 18, LockdownUI::TextDim(), 6);
	LockdownUI::AddRow(Column, Tagline, FMargin(0.f, 0.f, 0.f, 40.f), HAlign_Center);

	UButton* StartButton = LockdownUI::MakeButton(WidgetTree, TEXT("Start Game"), LockdownUI::Accent());
	UButton* InstructionsButton = LockdownUI::MakeButton(WidgetTree, TEXT("Instructions"), LockdownUI::Warning());
	UButton* QuitButton = LockdownUI::MakeButton(WidgetTree, TEXT("Quit"), LockdownUI::Danger());
	StartButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnStartClicked);
	InstructionsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnInstructionsClicked);
	QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitClicked);

	// Fixed-width column so all three buttons line up.
	const FMargin ButtonGap(120.f, 6.f);
	LockdownUI::AddRow(Column, StartButton, ButtonGap);
	LockdownUI::AddRow(Column, InstructionsButton, ButtonGap);
	LockdownUI::AddRow(Column, QuitButton, ButtonGap);

	InstructionsPanel = BuildInstructionsPanel();
	InstructionsPanel->SetVisibility(ESlateVisibility::Collapsed);
	LockdownUI::AddRow(Column, InstructionsPanel, FMargin(0.f, 24.f, 0.f, 0.f), HAlign_Center);
}

UBorder* UMainMenuWidget::BuildInstructionsPanel()
{
	UBorder* Panel = LockdownUI::MakePanel(WidgetTree, FMargin(32.f, 20.f), LockdownUI::PanelColor());
	UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->AddChild(Lines);

	static const TCHAR* const Instructions[] = {
		TEXT("Objective: destroy every terminal before time runs out"),
		TEXT("WASD move   Mouse look   Space jump"),
		TEXT("Left mouse fire   1 / 2 or wheel switch weapon"),
		TEXT("Terminals shut down turrets and open the next gate"),
		TEXT("Esc pause"),
	};
	for (const TCHAR* Line : Instructions)
	{
		UTextBlock* Text = LockdownUI::MakeText(WidgetTree, Line, 16, LockdownUI::TextBright(), 2);
		LockdownUI::AddRow(Lines, Text, FMargin(0.f, 3.f), HAlign_Center);
	}
	return Panel;
}

void UMainMenuWidget::OnStartClicked()
{
	UGameplayStatics::OpenLevel(this, GameMapName);
}

void UMainMenuWidget::OnInstructionsClicked()
{
	if (!InstructionsPanel)
	{
		return;
	}
	const bool bHidden = InstructionsPanel->GetVisibility() == ESlateVisibility::Collapsed;
	InstructionsPanel->SetVisibility(bHidden ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

#include "UI/EndScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/LockdownGameMode.h"
#include "Engine/World.h"
#include "UI/LockdownUIStyle.h"

TSharedRef<SWidget> UEndScreenWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		BuildLayout(Root);
		RefreshTexts();
	}
	return Super::RebuildWidget();
}

void UEndScreenWidget::BuildLayout(UCanvasPanel* Root)
{
	UBorder* Dim = LockdownUI::MakePanel(WidgetTree, FMargin(0.f), FLinearColor(0.f, 0.f, 0.f, 0.7f));
	LockdownUI::Fill(Root, Dim);

	UBorder* Panel = LockdownUI::MakePanel(WidgetTree, FMargin(72.f, 48.f), LockdownUI::PanelColor());
	LockdownUI::Place(Root, Panel, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->AddChild(Column);

	TitleText = LockdownUI::MakeText(WidgetTree, TEXT(""), 56, LockdownUI::Accent(), 12);
	LockdownUI::AddRow(Column, TitleText, FMargin(0.f, 0.f, 0.f, 8.f), HAlign_Center);

	SubtitleText = LockdownUI::MakeText(WidgetTree, TEXT(""), 18, LockdownUI::TextDim(), 4);
	LockdownUI::AddRow(Column, SubtitleText, FMargin(0.f, 0.f, 0.f, 32.f), HAlign_Center);

	RestartButton = LockdownUI::MakeButton(WidgetTree, TEXT("Restart"), LockdownUI::Accent());
	MainMenuButton = LockdownUI::MakeButton(WidgetTree, TEXT("Main Menu"), LockdownUI::Danger());
	RestartButton->OnClicked.AddDynamic(this, &UEndScreenWidget::OnRestartClicked);
	MainMenuButton->OnClicked.AddDynamic(this, &UEndScreenWidget::OnMainMenuClicked);

	const FMargin ButtonGap(0.f, 6.f);
	LockdownUI::AddRow(Column, RestartButton, ButtonGap);
	LockdownUI::AddRow(Column, MainMenuButton, ButtonGap);
}

void UEndScreenWidget::SetResult(EMatchResult InResult)
{
	Result = InResult;
	RefreshTexts();
}

void UEndScreenWidget::RefreshTexts()
{
	if (!TitleText || !SubtitleText)
	{
		return; // Tree not built yet; RebuildWidget calls us again.
	}

	FString Title;
	FString Subtitle;
	FLinearColor Color = LockdownUI::Accent();
	switch (Result)
	{
	case EMatchResult::Won:
		Title = TEXT("Facility Shut Down");
		Subtitle = TEXT("All terminals destroyed. You made it out.");
		Color = LockdownUI::Good();
		break;
	case EMatchResult::LostHealth:
		Title = TEXT("You Died");
		Subtitle = TEXT("The facility defenses got you.");
		Color = LockdownUI::Danger();
		break;
	case EMatchResult::LostTime:
		Title = TEXT("Self-Destruct");
		Subtitle = TEXT("Time ran out. The facility is gone.");
		Color = LockdownUI::Danger();
		break;
	default:
		break;
	}

	TitleText->SetText(FText::FromString(Title.ToUpper()));
	TitleText->SetColorAndOpacity(FSlateColor(Color));
	SubtitleText->SetText(FText::FromString(Subtitle.ToUpper()));
}

void UEndScreenWidget::OnRestartClicked()
{
	if (ALockdownGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ALockdownGameMode>() : nullptr)
	{
		GM->RestartLevel();
	}
}

void UEndScreenWidget::OnMainMenuClicked()
{
	if (ALockdownGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ALockdownGameMode>() : nullptr)
	{
		GM->ReturnToMainMenu();
	}
}

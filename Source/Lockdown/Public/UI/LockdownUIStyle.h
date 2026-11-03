#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"

/**
 * Shared look-and-feel helpers for the C++-built Lockdown widgets.
 * Header-only on purpose: every widget builds its own tree, so these are tiny inline factories.
 */
namespace LockdownUI
{
	// ---- Palette ----
	inline FLinearColor PanelColor()  { return FLinearColor(0.02f, 0.03f, 0.05f, 0.85f); }
	inline FLinearColor Accent()      { return FLinearColor(0.f, 0.85f, 1.f, 1.f); }
	inline FLinearColor Danger()      { return FLinearColor(1.f, 0.15f, 0.12f, 1.f); }
	inline FLinearColor Good()        { return FLinearColor(0.2f, 1.f, 0.4f, 1.f); }
	inline FLinearColor Warning()     { return FLinearColor(1.f, 0.7f, 0.1f, 1.f); }
	inline FLinearColor TextBright()  { return FLinearColor(0.92f, 0.97f, 1.f, 1.f); }
	inline FLinearColor TextDim()     { return FLinearColor(0.55f, 0.65f, 0.72f, 1.f); }

	// ---- Fonts ----
	/** Bold default-engine font with optional letter spacing (gives the sci-fi tracked look). */
	inline FSlateFontInfo MakeFont(int32 Size, int32 LetterSpacing = 0)
	{
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", Size);
		Font.LetterSpacing = LetterSpacing;
		return Font;
	}

	// ---- Factories ----
	/** Uppercase, shadowed text block. */
	inline UTextBlock* MakeText(UWidgetTree* Tree, const FString& Str, int32 Size, const FLinearColor& Color, int32 LetterSpacing = 2)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Str.ToUpper()));
		Text->SetFont(MakeFont(Size, LetterSpacing));
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.f, 1.f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));
		return Text;
	}

	/** Dark translucent panel; add content via AddChild. */
	inline UBorder* MakePanel(UWidgetTree* Tree, const FMargin& Padding, const FLinearColor& Color)
	{
		UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Panel->SetBrushColor(Color);
		Panel->SetPadding(Padding);
		return Panel;
	}

	/** Flat sci-fi button: dark fill, accent-tinted on hover/press, centered tracked label. */
	inline UButton* MakeButton(UWidgetTree* Tree, const FString& Label, const FLinearColor& AccentColor)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());

		FButtonStyle Style;
		Style.SetNormal(FSlateColorBrush(FLinearColor(0.05f, 0.08f, 0.12f, 0.95f)));
		Style.SetHovered(FSlateColorBrush(AccentColor * 0.35f + FLinearColor(0.f, 0.f, 0.f, 0.6f)));
		Style.SetPressed(FSlateColorBrush(AccentColor * 0.55f + FLinearColor(0.f, 0.f, 0.f, 0.6f)));
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f, 1.f, 0.f, 0.f));
		Button->SetStyle(Style);

		UTextBlock* Text = MakeText(Tree, Label, 22, AccentColor, 4);
		Text->SetJustification(ETextJustify::Center);
		if (UButtonSlot* LabelSlot = Cast<UButtonSlot>(Button->AddChild(Text)))
		{
			LabelSlot->SetPadding(FMargin(28.f, 12.f));
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}
		return Button;
	}

	/** Adds a child to a vertical box with padding + horizontal alignment. */
	inline UVerticalBoxSlot* AddRow(UVerticalBox* Box, UWidget* Child, const FMargin& Padding, EHorizontalAlignment HAlign = HAlign_Fill)
	{
		UVerticalBoxSlot* RowSlot = Box->AddChildToVerticalBox(Child);
		RowSlot->SetPadding(Padding);
		RowSlot->SetHorizontalAlignment(HAlign);
		return RowSlot;
	}

	/** Pins an auto-sized child to a canvas anchor point. Position is the pixel offset from the anchor. */
	inline UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Child, FVector2D Anchor, FVector2D Alignment, FVector2D Position)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Child);
		CanvasSlot->SetAnchors(FAnchors(Anchor.X, Anchor.Y, Anchor.X, Anchor.Y));
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetAutoSize(true);
		return CanvasSlot;
	}

	/** Stretches a child over the whole canvas. */
	inline UCanvasPanelSlot* Fill(UCanvasPanel* Canvas, UWidget* Child)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Child);
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		CanvasSlot->SetOffsets(FMargin(0.f));
		return CanvasSlot;
	}
}

#include "UI/HUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/HealthComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/StatusEffectComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/LockdownGameMode.h"
#include "Core/LockdownTypes.h"
#include "Engine/World.h"
#include "Player/LockdownCharacter.h"
#include "UI/LockdownUIStyle.h"

namespace
{
	/** One chip per status effect; chips are created up front and shown/hidden as effects come and go. */
	struct FChipDef
	{
		EStatusEffect Effect;
		const TCHAR* Label;
		FLinearColor Color;
	};

	const TArray<FChipDef>& GetChipDefs()
	{
		static const TArray<FChipDef> Defs = {
			{ EStatusEffect::Slowed,       TEXT("Slowed"),      FLinearColor(0.3f, 1.f, 0.35f, 1.f) },
			{ EStatusEffect::WeaponJammed, TEXT("Jammed"),      FLinearColor(0.75f, 0.35f, 1.f, 1.f) },
			{ EStatusEffect::DoubleDamage, TEXT("Overcharged"), FLinearColor(1.f, 0.6f, 0.1f, 1.f) },
		};
		return Defs;
	}

	constexpr float HealthBarWidth = 280.f;
	constexpr float HealthBarHeight = 18.f;
	constexpr float BannerFadeSeconds = 0.6f;
	constexpr float HitFlashBase = 0.5f;       // Minimum flash for any hit.
	constexpr float HitFlashPerHealthFraction = 2.f; // Extra flash scaled by fraction of max health lost.
}

// -------------------------------------------------------------------------------------------------
// Layout
// -------------------------------------------------------------------------------------------------

TSharedRef<SWidget> UHUDWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		BuildLayout(Root);
	}
	return Super::RebuildWidget();
}

void UHUDWidget::BuildLayout(UCanvasPanel* Root)
{
	// Order matters: later children draw on top, so the vignette goes first.
	BuildVignette(Root);
	BuildHealth(Root);
	BuildTimer(Root);
	BuildObjective(Root);
	BuildWeapon(Root);
	BuildCrosshair(Root);
	BuildEffectChips(Root);
	BuildBanner(Root);
}

void UHUDWidget::BuildVignette(UCanvasPanel* Root)
{
	DamageVignette = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	DamageVignette->SetBrush(FSlateColorBrush(FLinearColor::White));
	DamageVignette->SetColorAndOpacity(FLinearColor(1.f, 0.05f, 0.05f, 0.f));
	LockdownUI::Fill(Root, DamageVignette);
}

void UHUDWidget::BuildHealth(UCanvasPanel* Root)
{
	UBorder* Panel = LockdownUI::MakePanel(WidgetTree, FMargin(16.f, 10.f), LockdownUI::PanelColor());
	LockdownUI::Place(Root, Panel, FVector2D(0.f, 1.f), FVector2D(0.f, 1.f), FVector2D(32.f, -32.f));

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->AddChild(Column);

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	LockdownUI::AddRow(Column, Header, FMargin(0.f, 0.f, 0.f, 6.f));

	UTextBlock* Label = LockdownUI::MakeText(WidgetTree, TEXT("Health"), 14, LockdownUI::TextDim(), 4);
	Header->AddChildToHorizontalBox(Label)->SetVerticalAlignment(VAlign_Center);

	HealthText = LockdownUI::MakeText(WidgetTree, TEXT("100"), 22, LockdownUI::TextBright(), 1);
	UHorizontalBoxSlot* ValueSlot = Header->AddChildToHorizontalBox(HealthText);
	ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ValueSlot->SetHorizontalAlignment(HAlign_Right);

	USizeBox* BarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BarBox->SetWidthOverride(HealthBarWidth);
	BarBox->SetHeightOverride(HealthBarHeight);
	LockdownUI::AddRow(Column, BarBox, FMargin(0.f));

	HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	HealthBar->SetPercent(1.f);
	HealthBar->SetFillColorAndOpacity(LockdownUI::Good());
	BarBox->AddChild(HealthBar);
}

void UHUDWidget::BuildTimer(UCanvasPanel* Root)
{
	UBorder* Panel = LockdownUI::MakePanel(WidgetTree, FMargin(28.f, 6.f), LockdownUI::PanelColor());
	LockdownUI::Place(Root, Panel, FVector2D(0.5f, 0.f), FVector2D(0.5f, 0.f), FVector2D(0.f, 24.f));

	TimerText = LockdownUI::MakeText(WidgetTree, TEXT("00:00"), 40, LockdownUI::TextBright(), 4);
	Panel->AddChild(TimerText);
}

void UHUDWidget::BuildObjective(UCanvasPanel* Root)
{
	UBorder* Panel = LockdownUI::MakePanel(WidgetTree, FMargin(18.f, 8.f), LockdownUI::PanelColor());
	LockdownUI::Place(Root, Panel, FVector2D(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-32.f, 24.f));

	TerminalText = LockdownUI::MakeText(WidgetTree, TEXT("Terminals 0 / 0"), 22, LockdownUI::Accent(), 3);
	Panel->AddChild(TerminalText);
}

void UHUDWidget::BuildWeapon(UCanvasPanel* Root)
{
	UBorder* Panel = LockdownUI::MakePanel(WidgetTree, FMargin(18.f, 10.f), LockdownUI::PanelColor());
	LockdownUI::Place(Root, Panel, FVector2D(1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-32.f, -32.f));

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->AddChild(Column);

	WeaponText = LockdownUI::MakeText(WidgetTree, TEXT("Rifle"), 26, LockdownUI::TextBright(), 3);
	LockdownUI::AddRow(Column, WeaponText, FMargin(0.f, 0.f, 0.f, 2.f), HAlign_Right);

	AmmoText = LockdownUI::MakeText(WidgetTree, TEXT("Grenades 0"), 16, LockdownUI::TextDim(), 3);
	LockdownUI::AddRow(Column, AmmoText, FMargin(0.f), HAlign_Right);
}

void UHUDWidget::BuildCrosshair(UCanvasPanel* Root)
{
	UTextBlock* Crosshair = LockdownUI::MakeText(WidgetTree, TEXT("+"), 32, LockdownUI::Accent(), 0);
	Crosshair->SetOpacity(0.85f);
	LockdownUI::Place(Root, Crosshair, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
}

void UHUDWidget::BuildEffectChips(UCanvasPanel* Root)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	LockdownUI::Place(Root, Row, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -32.f));

	for (const FChipDef& Def : GetChipDefs())
	{
		UBorder* Chip = LockdownUI::MakePanel(WidgetTree, FMargin(14.f, 6.f), LockdownUI::PanelColor());
		UTextBlock* Text = LockdownUI::MakeText(WidgetTree, Def.Label, 16, Def.Color, 3);
		Chip->AddChild(Text);
		Chip->SetVisibility(ESlateVisibility::Collapsed); // Shown only while the effect is active.

		Row->AddChildToHorizontalBox(Chip)->SetPadding(FMargin(6.f, 0.f));
		EffectChips.Add(Chip);
		EffectChipTexts.Add(Text);
	}
}

void UHUDWidget::BuildBanner(UCanvasPanel* Root)
{
	BannerText = LockdownUI::MakeText(WidgetTree, TEXT(""), 54, LockdownUI::Accent(), 10);
	BannerText->SetRenderOpacity(0.f);
	LockdownUI::Place(Root, BannerText, FVector2D(0.5f, 0.3f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
}

// -------------------------------------------------------------------------------------------------
// Lifecycle / binding
// -------------------------------------------------------------------------------------------------

void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// The HUD is display-only; never let it swallow mouse clicks.
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (ALockdownGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ALockdownGameMode>() : nullptr)
	{
		GM->OnTerminalCountChanged.AddUniqueDynamic(this, &UHUDWidget::HandleTerminalCountChanged);
	}
	TryBindPlayer();
}

void UHUDWidget::NativeDestruct()
{
	UnbindPlayer();
	if (ALockdownGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ALockdownGameMode>() : nullptr)
	{
		GM->OnTerminalCountChanged.RemoveDynamic(this, &UHUDWidget::HandleTerminalCountChanged);
	}
	Super::NativeDestruct();
}

void UHUDWidget::TryBindPlayer()
{
	if (BoundCharacter.IsValid())
	{
		return;
	}
	ALockdownCharacter* Character = Cast<ALockdownCharacter>(GetOwningPlayerPawn());
	if (!Character)
	{
		return;
	}
	UHealthComponent* Health = Character->GetHealthComponent();
	if (!Health)
	{
		return;
	}

	BoundCharacter = Character;
	BoundHealth = Health;
	Health->OnHealthChanged.AddUniqueDynamic(this, &UHUDWidget::HandleHealthChanged);

	// Seed the bar without triggering a damage flash.
	LastHealth = -1.f;
	ApplyHealth(Health->GetHealthPercent() * 100.f, 100.f);
}

void UHUDWidget::UnbindPlayer()
{
	if (UHealthComponent* Health = BoundHealth.Get())
	{
		Health->OnHealthChanged.RemoveDynamic(this, &UHUDWidget::HandleHealthChanged);
	}
	BoundHealth.Reset();
	BoundCharacter.Reset();
}

// -------------------------------------------------------------------------------------------------
// Event handlers
// -------------------------------------------------------------------------------------------------

void UHUDWidget::HandleHealthChanged(float Current, float Max)
{
	ApplyHealth(Current, Max);
}

void UHUDWidget::HandleTerminalCountChanged(int32 Remaining, int32 Total)
{
	// The final terminal ends the match, so the win screen replaces the banner.
	if (Remaining > 0)
	{
		ShowBanner(FText::FromString(TEXT("TERMINAL DESTROYED!")));
	}
}

void UHUDWidget::ShowBanner(FText Message)
{
	if (!BannerText)
	{
		return;
	}
	BannerText->SetText(Message.ToUpper());
	BannerTimeLeft = BannerDuration;
}

void UHUDWidget::ApplyHealth(float Current, float Max)
{
	const float Percent = Max > 0.f ? FMath::Clamp(Current / Max, 0.f, 1.f) : 0.f;

	if (HealthBar)
	{
		HealthBar->SetPercent(Percent);
		// Linear red -> green blend reads instantly at a glance.
		HealthBar->SetFillColorAndOpacity(LockdownUI::Danger() + (LockdownUI::Good() - LockdownUI::Danger()) * Percent);
	}
	if (HealthText)
	{
		HealthText->SetText(FText::FromString(FString::FromInt(FMath::CeilToInt(Current))));
	}

	if (LastHealth >= 0.f && Current < LastHealth && Max > 0.f)
	{
		const float LostFraction = (LastHealth - Current) / Max;
		DamageFlash = FMath::Clamp(DamageFlash + HitFlashBase + LostFraction * HitFlashPerHealthFraction, 0.f, 1.f);
	}
	LastHealth = Current;
}

// -------------------------------------------------------------------------------------------------
// Per-frame refresh
// -------------------------------------------------------------------------------------------------

void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	FlashClock += InDeltaTime;
	TryBindPlayer(); // Cheap early-out once bound.

	UpdateTimer(InDeltaTime);
	UpdateObjective();
	UpdateWeapon();
	UpdateEffects();
	UpdateBanner(InDeltaTime);
	UpdateVignette(InDeltaTime);
}

void UHUDWidget::UpdateTimer(float DeltaTime)
{
	ALockdownGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ALockdownGameMode>() : nullptr;
	if (!GM || !TimerText)
	{
		return;
	}

	const float Remaining = GM->GetTimeRemaining();
	const int32 Seconds = FMath::CeilToInt(Remaining);
	if (Seconds != CachedSeconds)
	{
		CachedSeconds = Seconds;
		TimerText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Seconds / 60, Seconds % 60)));
	}

	// Below the warning threshold: blink between red and white twice a second.
	FLinearColor Color = LockdownUI::TextBright();
	if (Remaining < TimerWarningSeconds)
	{
		Color = FMath::Fmod(FlashClock, 1.f) < 0.5f ? LockdownUI::Danger() : LockdownUI::TextBright();
	}
	TimerText->SetColorAndOpacity(FSlateColor(Color));
}

void UHUDWidget::UpdateObjective()
{
	ALockdownGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ALockdownGameMode>() : nullptr;
	if (!GM || !TerminalText)
	{
		return;
	}

	const int32 Remaining = GM->GetTerminalsRemaining();
	const int32 Total = GM->GetTerminalsTotal();
	if (Remaining == CachedRemaining && Total == CachedTotal)
	{
		return;
	}
	CachedRemaining = Remaining;
	CachedTotal = Total;
	TerminalText->SetText(FText::FromString(FString::Printf(TEXT("TERMINALS %d / %d"), Remaining, Total)));
}

void UHUDWidget::UpdateWeapon()
{
	ALockdownCharacter* Character = BoundCharacter.Get();
	if (!Character || !WeaponText || !AmmoText)
	{
		return;
	}

	const EWeaponSlot Weapon = Character->GetCurrentWeapon();
	const int32 Ammo = Character->GetGrenadeAmmo();
	const int32 WeaponId = static_cast<int32>(Weapon);
	if (WeaponId == CachedWeapon && Ammo == CachedAmmo)
	{
		return;
	}
	CachedWeapon = WeaponId;
	CachedAmmo = Ammo;

	const bool bLauncher = (Weapon == EWeaponSlot::GrenadeLauncher);
	WeaponText->SetText(FText::FromString(bLauncher ? TEXT("GRENADE LAUNCHER") : TEXT("RIFLE")));
	AmmoText->SetText(FText::FromString(FString::Printf(TEXT("GRENADES  %d"), Ammo)));

	// Ammo is only the active resource with the launcher out; empty reads as danger.
	const FLinearColor AmmoColor = (bLauncher && Ammo <= 0) ? LockdownUI::Danger()
		: (bLauncher ? LockdownUI::Accent() : LockdownUI::TextDim());
	AmmoText->SetColorAndOpacity(FSlateColor(AmmoColor));
}

void UHUDWidget::UpdateEffects()
{
	ALockdownCharacter* Character = BoundCharacter.Get();
	UStatusEffectComponent* Status = Character ? Character->GetStatusEffectComponent() : nullptr;
	const TArray<FChipDef>& Defs = GetChipDefs();

	for (int32 Index = 0; Index < Defs.Num() && Index < EffectChips.Num(); ++Index)
	{
		const bool bActive = Status && Status->HasEffect(Defs[Index].Effect);
		EffectChips[Index]->SetVisibility(bActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bActive)
		{
			const int32 Seconds = FMath::CeilToInt(Status->GetRemainingTime(Defs[Index].Effect));
			EffectChipTexts[Index]->SetText(FText::FromString(
				FString::Printf(TEXT("%s  %ds"), Defs[Index].Label, Seconds).ToUpper()));
		}
	}
}

void UHUDWidget::UpdateBanner(float DeltaTime)
{
	if (!BannerText)
	{
		return;
	}
	if (BannerTimeLeft <= 0.f)
	{
		BannerText->SetRenderOpacity(0.f);
		return;
	}
	BannerTimeLeft -= DeltaTime;
	// Full opacity, then a short fade over the last BannerFadeSeconds.
	BannerText->SetRenderOpacity(FMath::Clamp(BannerTimeLeft / BannerFadeSeconds, 0.f, 1.f));
}

void UHUDWidget::UpdateVignette(float DeltaTime)
{
	if (!DamageVignette)
	{
		return;
	}
	DamageFlash = FMath::Max(0.f, DamageFlash - VignetteFadePerSecond * DeltaTime);
	DamageVignette->SetColorAndOpacity(FLinearColor(1.f, 0.05f, 0.05f, DamageFlash * MaxVignetteAlpha));
}

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UImage;
class UProgressBar;
class UTextBlock;
class UHealthComponent;
class ALockdownCharacter;

/**
 * In-match HUD. Health arrives via delegate; timer, objective, weapon and effect chips
 * are refreshed in NativeTick because they change continuously and are cheap to read.
 */
UCLASS()
class LOCKDOWN_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows a large centered message that fades out (e.g. "SECTOR UNLOCKED"). */
	UFUNCTION(BlueprintCallable, Category = "Lockdown|HUD")
	void ShowBanner(FText Message);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Timer turns red and flashes below this many seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Lockdown|HUD")
	float TimerWarningSeconds = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Lockdown|HUD")
	float BannerDuration = 2.5f;

	/** Peak opacity of the red damage vignette. */
	UPROPERTY(EditDefaultsOnly, Category = "Lockdown|HUD")
	float MaxVignetteAlpha = 0.35f;

	/** How fast (alpha/sec) the vignette fades after a hit. */
	UPROPERTY(EditDefaultsOnly, Category = "Lockdown|HUD")
	float VignetteFadePerSecond = 1.5f;

private:
	// ---- Layout (one small builder per HUD region) ----
	void BuildLayout(UCanvasPanel* Root);
	void BuildVignette(UCanvasPanel* Root);
	void BuildHealth(UCanvasPanel* Root);
	void BuildTimer(UCanvasPanel* Root);
	void BuildObjective(UCanvasPanel* Root);
	void BuildWeapon(UCanvasPanel* Root);
	void BuildCrosshair(UCanvasPanel* Root);
	void BuildEffectChips(UCanvasPanel* Root);
	void BuildBanner(UCanvasPanel* Root);

	// ---- Binding ----
	/** Pawn may possess after the HUD is created, so this retries from NativeTick. */
	void TryBindPlayer();
	void UnbindPlayer();

	UFUNCTION()
	void HandleHealthChanged(float Current, float Max);

	UFUNCTION()
	void HandleTerminalCountChanged(int32 Remaining, int32 Total);

	// ---- Per-frame refresh ----
	void ApplyHealth(float Current, float Max);
	void UpdateTimer(float DeltaTime);
	void UpdateObjective();
	void UpdateWeapon();
	void UpdateEffects();
	void UpdateBanner(float DeltaTime);
	void UpdateVignette(float DeltaTime);

	// ---- Widgets ----
	UPROPERTY(Transient) TObjectPtr<UImage> DamageVignette;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HealthText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TimerText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TerminalText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WeaponText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> AmmoText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BannerText;
	UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> EffectChips;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> EffectChipTexts;

	// ---- State ----
	TWeakObjectPtr<ALockdownCharacter> BoundCharacter;
	TWeakObjectPtr<UHealthComponent> BoundHealth;

	float LastHealth = -1.f;
	float DamageFlash = 0.f;
	float FlashClock = 0.f;
	float BannerTimeLeft = 0.f;

	// Cached values so text is only rebuilt when something changed.
	int32 CachedSeconds = -1;
	int32 CachedRemaining = -1;
	int32 CachedTotal = -1;
	int32 CachedWeapon = -1;
	int32 CachedAmmo = -1;
};

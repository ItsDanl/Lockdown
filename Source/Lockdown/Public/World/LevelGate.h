#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelGate.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class USoundBase;

/**
 * Two-panel sliding door that seals a section of the facility. Opened by a terminal via Open().
 * Faces along the actor's X axis; panels slide apart along Y. Ticks only while moving.
 */
UCLASS()
class LOCKDOWN_API ALevelGate : public AActor
{
	GENERATED_BODY()

public:
	ALevelGate();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Starts the (eased) opening animation. Safe to call more than once. */
	UFUNCTION(BlueprintCallable, Category = "Lockdown|Gate")
	void Open();

	UFUNCTION(BlueprintPure, Category = "Lockdown|Gate")
	bool IsOpen() const { return bIsOpen; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> LeftPanel;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> RightPanel;

	/** Status light: red while sealed, green once open. */
	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UPointLightComponent> StatusLight;

	/** Total doorway width in cm (each panel covers half). */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Gate", meta = (ClampMin = "50"))
	float GateWidth = 400.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Gate", meta = (ClampMin = "50"))
	float GateHeight = 320.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Gate", meta = (ClampMin = "5"))
	float PanelThickness = 40.f;

	/** Seconds for the doors to fully open. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Gate", meta = (ClampMin = "0.1"))
	float OpenDuration = 2.0f;

	/** Easing exponent (1 = linear, 2+ = smoother start/stop). */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Gate", meta = (ClampMin = "1"))
	float EaseExponent = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Gate")
	FLinearColor ClosedColor = FLinearColor(1.f, 0.05f, 0.05f);

	UPROPERTY(EditAnywhere, Category = "Lockdown|Gate")
	FLinearColor OpenColor = FLinearColor(0.1f, 1.f, 0.25f);

	UPROPERTY(EditAnywhere, Category = "Lockdown|Gate")
	FLinearColor PanelColor = FLinearColor(0.12f, 0.13f, 0.16f);

	UPROPERTY(EditAnywhere, Category = "Lockdown|Gate")
	TObjectPtr<USoundBase> OpenSound;

private:
	/** Sizes/positions panels from the tunables (called from OnConstruction and BeginPlay). */
	void LayoutPanels();
	void ApplyOpenAlpha(float Alpha);

	bool bIsOpen = false;
	/** Linear 0..1 progress; eased only when positioning panels. */
	float OpenProgress = 0.f;
};

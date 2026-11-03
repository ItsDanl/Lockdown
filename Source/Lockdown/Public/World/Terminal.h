#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Terminal.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UHealthComponent;
class UMaterialInstanceDynamic;
class UParticleSystem;
class USoundBase;
class ATurret;
class ALevelGate;

/**
 * Shootable control terminal. When destroyed it disables its turrets, opens its gate and tells the GameMode.
 * Screen colour reflects remaining health (cyan -> orange -> red) and flashes white on hits.
 */
UCLASS()
class LOCKDOWN_API ATerminal : public AActor
{
	GENERATED_BODY()

public:
	ATerminal();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Lockdown|Terminal")
	bool IsDestroyed() const { return bIsDestroyed; }

	/** Turrets that shut down when this terminal dies. */
	UPROPERTY(EditInstanceOnly, Category = "Lockdown|Terminal")
	TArray<TObjectPtr<ATurret>> ControlledTurrets;

	/** Gate opened when this terminal dies. */
	UPROPERTY(EditInstanceOnly, Category = "Lockdown|Terminal")
	TObjectPtr<ALevelGate> GateToOpen;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> ScreenMesh;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> FireballMesh;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UPointLightComponent> ScreenLight;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UHealthComponent> HealthComponent;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Terminal", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Terminal")
	FLinearColor BodyColor = FLinearColor(0.08f, 0.09f, 0.11f);

	UPROPERTY(EditAnywhere, Category = "Lockdown|Terminal")
	FLinearColor HealthyColor = FLinearColor(0.f, 0.9f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Lockdown|Terminal")
	FLinearColor DamagedColor = FLinearColor(1.f, 0.45f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Lockdown|Terminal")
	FLinearColor CriticalColor = FLinearColor(1.f, 0.05f, 0.05f);

	UPROPERTY(EditAnywhere, Category = "Lockdown|Terminal")
	FLinearColor HitFlashColor = FLinearColor(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Lockdown|Terminal")
	float HitFlashDuration = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Terminal")
	float ScreenLightIntensity = 5.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	TObjectPtr<UParticleSystem> ExplosionEffect;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	float ExplosionFlashTime = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	float ExplosionLightIntensity = 150.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	float ExplosionFireballScale = 3.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	FLinearColor ExplosionColor = FLinearColor(1.f, 0.45f, 0.1f);

private:
	UFUNCTION()
	void HandleHealthChanged(float Current, float Max);

	UFUNCTION()
	void HandleDeath();

	/** Screen tint from health fraction; returns the base (non-flash) colour. */
	FLinearColor GetHealthColor(float Percent) const;
	void SetScreenColor(const FLinearColor& Color);
	void EndHitFlash();

	/** Disables turrets, opens the gate and reports to the GameMode. */
	void NotifyWorld();
	void StartExplosion();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ScreenMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FireballMID;

	FTimerHandle HitFlashTimer;
	FLinearColor CurrentBaseColor = FLinearColor::White;
	bool bIsDestroyed = false;
	float ExplosionElapsed = 0.f;
};

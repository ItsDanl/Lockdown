#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/LockdownTypes.h"
#include "Turret.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UHealthComponent;
class UMaterialInstanceDynamic;
class UParticleSystem;
class USoundBase;
class ATurretProjectile;
class ATurretShell;
class ALockdownCharacter;

/** Internal alert state; drives light colour and firing. */
enum class ETurretAlertState : uint8
{
	Idle,        // sweeping, nothing seen
	Suspicious,  // saw the player, still inside ReactionDelay
	Firing       // shooting
};

/**
 * Stationary sentry turret. Scans in a cone, needs line of sight (no seeing through walls),
 * waits ReactionDelay before the first shot, forgets the player after LoseSightTime.
 * Vision runs on a timer; Tick only interpolates the head rotation / light / effects.
 */
UCLASS()
class LOCKDOWN_API ATurret : public AActor
{
	GENERATED_BODY()

public:
	ATurret();

	virtual void Tick(float DeltaSeconds) override;

	/** Called by a terminal: the turret stops scanning/firing, droops its barrel and turns its light off. */
	UFUNCTION(BlueprintCallable, Category = "Lockdown|Turret")
	void Disable();

	UFUNCTION(BlueprintPure, Category = "Lockdown|Turret")
	bool IsDisabled() const { return bIsDisabled; }

protected:
	virtual void BeginPlay() override;

	// ---- Components ----
	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	/** Rotates around Z (yaw). */
	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<USceneComponent> YawPivot;

	/** Child of YawPivot; rotates around Y (pitch). Sight origin and muzzle hang off it. */
	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<USceneComponent> PitchPivot;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> BarrelMesh;

	/** Glowing "eye" tinted with the alert colour. */
	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> EyeMesh;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<USceneComponent> Muzzle;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UPointLightComponent> StatusLight;

	/** Hidden sphere scaled up during the death explosion. */
	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> FireballMesh;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UHealthComponent> HealthComponent;

	// ---- Type presets ----
	UPROPERTY(EditAnywhere, Category = "Lockdown|Turret")
	ETurretType TurretType = ETurretType::Medium;

	/** If true, Small/Medium/Large overwrite health, fire rate, turn speed, scale and weapon on BeginPlay. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Turret")
	bool bUseTypePresets = true;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Turret", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	/** Seconds between shots. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Turret", meta = (ClampMin = "0.05"))
	float FireInterval = 1.0f;

	/** Large turrets lob explosive shells instead of bolts. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Turret")
	bool bFireShells = false;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Turret")
	float ProjectileDamage = 10.f;

	/** Extra upward pitch (degrees) given to shells so they arc onto the target. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Turret")
	float ShellPitchBias = 8.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Turret")
	TSubclassOf<ATurretProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Turret")
	TSubclassOf<ATurretShell> ShellClass;

	// ---- Vision ----
	UPROPERTY(EditAnywhere, Category = "Lockdown|Vision")
	float DetectionRange = 2000.f;

	/** Half-angle of the vision cone in degrees (only used to ACQUIRE the player). */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Vision", meta = (ClampMin = "1", ClampMax = "180"))
	float HalfAngleDegrees = 45.f;

	/** Seconds between sighting the player and the first shot. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Vision")
	float ReactionDelay = 0.5f;

	/** Seconds without line of sight before the turret gives up. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Vision")
	float LoseSightTime = 2.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Vision", meta = (ClampMin = "0.02"))
	float VisionCheckInterval = 0.1f;

	/** Seconds of target velocity added to the aim point (0 = no lead). */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Vision")
	float LeadTime = 0.12f;

	// ---- Aiming ----
	/** RInterpTo speed while tracking. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Aim")
	float TurnSpeed = 5.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Aim")
	float IdleSweepAngle = 60.f;

	/** Idle sweep speed in radians/second of the sine phase. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Aim")
	float IdleSweepSpeed = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Aim")
	float MaxPitchUp = 45.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Aim")
	float MaxPitchDown = 30.f;

	/** How far the barrel droops (degrees) once disabled. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Aim")
	float DisabledDroopPitch = 40.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Aim")
	float DisabledTurnSpeed = 1.5f;

	// ---- Light / FX ----
	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	FLinearColor IdleColor = FLinearColor(0.f, 0.9f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	FLinearColor SuspiciousColor = FLinearColor(1.f, 0.8f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	FLinearColor FiringColor = FLinearColor(1.f, 0.03f, 0.03f);

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	float IdleLightIntensity = 4.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	float AlertLightIntensity = 8.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	float MuzzleFlashBoost = 25.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	float MuzzleFlashDuration = 0.08f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	float LightColorInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	float LightFadeSpeed = 3.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	TObjectPtr<UParticleSystem> ExplosionEffect;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	float ExplosionFlashTime = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	float ExplosionLightIntensity = 150.f;

	/** Final scale (in actor units / 100) of the fireball sphere. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	float ExplosionFireballScale = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|FX")
	FLinearColor ExplosionColor = FLinearColor(1.f, 0.4f, 0.05f);

private:
	// Vision / state
	void VisionCheck();
	APawn* FindPlayerTarget() const;
	bool IsInVisionCone(const FVector& Eye, const FVector& TargetLocation) const;
	bool HasLineOfSight(const FVector& Eye, AActor* Target) const;
	void SetAlertState(ETurretAlertState NewState);
	void ForgetTarget();

	// Firing
	void Fire();
	bool CanFire() const;

	// Per-frame visuals
	FRotator ComputeDesiredAim(float DeltaSeconds);
	void UpdateAim(float DeltaSeconds);
	void UpdateLight(float DeltaSeconds);
	void TickExplosion(float DeltaSeconds);

	// Setup / lifecycle
	void ApplyTypePreset();
	void ConfigureHealth();
	FLinearColor GetStateColor() const;

	UFUNCTION()
	void HandleDeath();

	void Explode();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> EyeMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FireballMID;

	FTimerHandle VisionTimer;
	FTimerHandle FireTimer;

	ETurretAlertState AlertState = ETurretAlertState::Idle;
	bool bIsDisabled = false;
	bool bIsExploding = false;

	TWeakObjectPtr<APawn> TargetPawn;
	FVector LastKnownTargetLocation = FVector::ZeroVector;
	float LastSeenTime = -1000.f;
	float SuspicionStartTime = 0.f;

	/** Current local (relative to actor) aim: Yaw drives YawPivot, Pitch drives PitchPivot. */
	FRotator AimRotation = FRotator::ZeroRotator;
	float IdlePhase = 0.f;

	FLinearColor CurrentLightColor = FLinearColor::Black;
	float CurrentLightIntensity = 0.f;
	float MuzzleFlashRemaining = 0.f;
	float ExplosionElapsed = 0.f;
};

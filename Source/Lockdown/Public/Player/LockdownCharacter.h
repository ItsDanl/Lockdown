#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "LockdownCharacter.generated.h"

class UCameraComponent;
class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UPointLightComponent;
class UHealthComponent;
class UStatusEffectComponent;
class UInputAction;
class UEnhancedInputComponent;
class UInputMappingContext;
class UMaterialInterface;
class USoundBase;
class AGrenadeProjectile;
class UDamageType;
class AController;
struct FInputActionValue;

UENUM(BlueprintType)
enum class EWeaponSlot : uint8
{
	Rifle,
	GrenadeLauncher
};

/** Fires when the equipped weapon changes OR grenade ammo changes, so the HUD can simply re-read state. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponChanged);
/** Fires whenever the player takes damage (drives the HUD damage flash). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDamaged, float, DamageAmount);

/** First-person player: hitscan rifle + grenade launcher, Enhanced Input built entirely in C++. */
UCLASS()
class LOCKDOWN_API ALockdownCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALockdownCharacter();

	// ---- Queries used by HUD / pickups / hazards ----
	UFUNCTION(BlueprintPure, Category = "Lockdown|Weapons")
	EWeaponSlot GetCurrentWeapon() const { return CurrentWeapon; }

	UFUNCTION(BlueprintPure, Category = "Lockdown|Weapons")
	int32 GetGrenadeAmmo() const { return GrenadeAmmo; }

	UFUNCTION(BlueprintPure, Category = "Lockdown|Weapons")
	int32 GetMaxGrenadeAmmo() const { return MaxGrenadeAmmo; }

	UFUNCTION(BlueprintCallable, Category = "Lockdown|Weapons")
	void AddGrenadeAmmo(int32 Amount);

	/** True when the current weapon could fire right now (alive, not jammed, off cooldown, has ammo). */
	UFUNCTION(BlueprintPure, Category = "Lockdown|Weapons")
	bool CanFire() const;

	/** 2 while Overcharged (DoubleDamage), otherwise 1. */
	UFUNCTION(BlueprintPure, Category = "Lockdown|Weapons")
	float GetDamageMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Lockdown|Health")
	bool IsDead() const { return bIsDead; }

	UHealthComponent* GetHealthComponent() const { return HealthComponent; }
	UStatusEffectComponent* GetStatusEffectComponent() const { return StatusEffectComponent; }
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

	UPROPERTY(BlueprintAssignable, Category = "Lockdown|Events")
	FOnWeaponChanged OnWeaponChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lockdown|Events")
	FOnPlayerDamaged OnPlayerDamaged;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ---- Components ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Components")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> ArmMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> GunMesh;

	/** Where shots and flashes come from, parented to the camera. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Components")
	TObjectPtr<USceneComponent> Muzzle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Components")
	TObjectPtr<UPointLightComponent> MuzzleFlashLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Components")
	TObjectPtr<UHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Components")
	TObjectPtr<UStatusEffectComponent> StatusEffectComponent;

	// ---- Movement / look ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Movement")
	float BaseWalkSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Movement")
	float SlowedSpeedMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Movement")
	float JumpVelocity = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Movement")
	float EyeHeight = 64.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Input")
	float LookSensitivity = 0.6f;

	// ---- Rifle ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Rifle")
	float RifleDamage = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Rifle")
	float RifleFireRate = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Rifle")
	float RifleRange = 10000.f;

	/** Degrees the view kicks upward per shot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Rifle|Feel")
	float RifleRecoilPitch = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Rifle|Feel")
	float RecoilYawJitter = 0.15f;

	// ---- Grenade launcher ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Grenade")
	TSubclassOf<AGrenadeProjectile> GrenadeClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Grenade")
	float GrenadeCooldown = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Grenade")
	int32 MaxGrenadeAmmo = 6;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Lockdown|Grenade")
	int32 GrenadeAmmo = 6;

	/** Spawn the grenade this far in front of the camera so it clears the player capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Grenade")
	float GrenadeSpawnForwardOffset = 70.f;

	/** Extra upward pitch (degrees) so grenades lob instead of flying dead flat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Grenade")
	float GrenadeLaunchPitchBias = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Grenade|Feel")
	float GrenadeRecoilPitch = 2.f;

	// ---- Weapon feel ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Weapons|Feel")
	float WeaponSwitchDelay = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Weapons|Feel")
	float MuzzleFlashDuration = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Weapons|Feel")
	float MuzzleFlashIntensity = 6000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Weapons|Feel")
	FLinearColor MuzzleFlashColor = FLinearColor(1.f, 0.7f, 0.3f);

	/** Gun view-model slides back this far per shot, then returns after GunKickRecoverTime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Weapons|Feel")
	float GunKickDistance = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Weapons|Feel")
	float GunKickRecoverTime = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Weapons|Feel")
	FVector GunRestLocation = FVector(38.f, 18.f, -16.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Weapons|Feel")
	FVector RifleGunScale = FVector(0.5f, 0.07f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Weapons|Feel")
	FVector LauncherGunScale = FVector(0.4f, 0.13f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Weapons|Feel")
	FLinearColor RifleColor = FLinearColor(0.05f, 0.08f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Weapons|Feel")
	FLinearColor LauncherColor = FLinearColor(0.08f, 0.2f, 0.1f);

	// ---- Tracer (short-lived thin cylinder; no debug draw so it works in shipping) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Rifle|Tracer")
	float TracerThickness = 0.015f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Rifle|Tracer")
	float TracerLifetime = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Rifle|Tracer")
	FLinearColor TracerColor = FLinearColor(1.f, 0.85f, 0.3f);

	// ---- Player status tuning ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Status")
	float DoubleDamageMultiplier = 2.f;

	// ---- Optional sounds (all null-safe) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Audio")
	TObjectPtr<USoundBase> RifleFireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Audio")
	TObjectPtr<USoundBase> GrenadeFireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Audio")
	TObjectPtr<USoundBase> DryFireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Audio")
	TObjectPtr<USoundBase> WeaponSwitchSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Audio")
	TObjectPtr<USoundBase> HurtSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Audio")
	TObjectPtr<USoundBase> DeathSound;

	// ---- Default shapes/materials (loaded from /Engine in the constructor) ----
	UPROPERTY(EditDefaultsOnly, Category = "Lockdown|Visual")
	TObjectPtr<UStaticMesh> CubeShape;

	UPROPERTY(EditDefaultsOnly, Category = "Lockdown|Visual")
	TObjectPtr<UStaticMesh> CylinderShape;

	UPROPERTY(EditDefaultsOnly, Category = "Lockdown|Visual")
	TObjectPtr<UMaterialInterface> BasicShapeMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Lockdown|Visual")
	TObjectPtr<UMaterialInterface> GlowMaterial;

private:
	// ---- Input assets, created at runtime so no .uasset is needed ----
	void CreateInputAssets();
	void BindInputActions(UEnhancedInputComponent* Input);
	void AddMappingContextToLocalPlayer();
	void MapWasdKeys();

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> FireAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SelectRifleAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SelectGrenadeAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CycleWeaponAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PauseAction;

	UInputAction* MakeAction(const TCHAR* Name, bool bAxis2D);

	// ---- Input handlers ----
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleFireStarted();
	void HandleFireHeld();
	void HandleSelectRifle();
	void HandleSelectGrenade();
	void HandleCycleWeapon();
	void HandlePause();

	// ---- Weapons ----
	void SetWeapon(EWeaponSlot NewWeapon);
	void ApplyWeaponVisuals();
	bool IsFireBlockedByState() const;
	void FireCurrentWeapon();
	void FireRifle();
	void FireGrenade();
	void ApplyRecoil(float PitchDegrees);
	void PlayMuzzleFlash();
	void HideMuzzleFlash();
	void KickGun();
	void ResetGunKick();
	void SpawnTracer(const FVector& Start, const FVector& End);
	void TintMesh(UStaticMeshComponent* TargetMesh, UMaterialInterface* BaseMaterial, const FLinearColor& Color);
	void PlaySound2D(USoundBase* Sound) const;

	// ---- Health / status ----
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleDeath();

	UFUNCTION()
	void HandleStatusEffectsChanged();

	void UpdateMovementSpeed();

	EWeaponSlot CurrentWeapon = EWeaponSlot::Rifle;
	float NextFireTime = 0.f;
	bool bIsDead = false;

	FTimerHandle MuzzleFlashTimer;
	FTimerHandle GunKickTimer;
};

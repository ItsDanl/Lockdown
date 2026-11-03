#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "GrenadeProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UPointLightComponent;
class UParticleSystem;
class USoundBase;
class UMaterialInterface;

/**
 * Player grenade. Bounces once, explodes on the second impact or when the fuse runs out.
 * Deals radial damage (ignoring the instigator) and flashes a brief point light.
 */
UCLASS()
class LOCKDOWN_API AGrenadeProjectile : public AActor
{
	GENERATED_BODY()

public:
	AGrenadeProjectile();

	/** Set by the shooter so the Overcharge pickup also boosts grenades. */
	void SetDamageMultiplier(float NewMultiplier) { DamageMultiplier = NewMultiplier; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Grenade")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Grenade")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Grenade")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Grenade")
	TObjectPtr<UPointLightComponent> ExplosionLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Grenade")
	float Damage = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Grenade")
	float ExplosionRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Grenade")
	float FuseTime = 3.f;

	/** Impacts after which the grenade detonates (1 bounce allowed, explodes on the 2nd impact). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Grenade")
	int32 ImpactsBeforeExplosion = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Grenade|Visual")
	FLinearColor GrenadeColor = FLinearColor(0.1f, 1.f, 0.25f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Grenade|Visual")
	FLinearColor ExplosionLightColor = FLinearColor(1.f, 0.55f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Grenade|Visual")
	float ExplosionLightIntensity = 80000.f;

	/** How long the flash (and the hidden grenade actor) lingers after detonation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Grenade|Visual")
	float ExplosionLightDuration = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Grenade|Visual")
	float ExplosionEffectScale = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Grenade|FX")
	TObjectPtr<UParticleSystem> ExplosionEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Grenade|FX")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Grenade|FX")
	TObjectPtr<USoundBase> BounceSound;

	/** Optional override; defaults to the engine's emissive mesh material when available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Grenade|Visual")
	TObjectPtr<UMaterialInterface> GlowMaterial;

private:
	UFUNCTION()
	void HandleBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	void Explode();

	FTimerHandle FuseTimer;
	float DamageMultiplier = 1.f;
	int32 ImpactCount = 0;
	bool bExploded = false;
};

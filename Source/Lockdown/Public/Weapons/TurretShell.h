#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TurretShell.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UPointLightComponent;
class UParticleSystem;
class USoundBase;
class UMaterialInterface;

/**
 * Arced explosive shell fired by Large turrets. Explodes on impact, hurting the player
 * (with linear falloff) if within the blast radius. Terminals and other turrets are never harmed.
 */
UCLASS()
class LOCKDOWN_API ATurretShell : public AActor
{
	GENERATED_BODY()

public:
	ATurretShell();

	/** Computes a low-arc ballistic velocity that lands on TargetLocation at ShellSpeed. Call right after spawning. */
	void LaunchTowards(const FVector& TargetLocation);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Shell")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Shell")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Shell")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Shell")
	TObjectPtr<UPointLightComponent> ShellLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Shell")
	float Damage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Shell")
	float ExplosionRadius = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Shell")
	float ShellSpeed = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Shell")
	float MaxLifetime = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Shell|Visual")
	FLinearColor ShellColor = FLinearColor(1.f, 0.35f, 0.02f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Shell|Visual")
	float ExplosionLightIntensity = 60000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Shell|Visual")
	float ExplosionLightDuration = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Shell|Visual")
	float ExplosionEffectScale = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Shell|FX")
	TObjectPtr<UParticleSystem> ExplosionEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Shell|FX")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Shell|Visual")
	TObjectPtr<UMaterialInterface> GlowMaterial;

private:
	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);

	void Explode();
	void DamagePlayersInRadius(const FVector& Origin);

	bool bExploded = false;
};

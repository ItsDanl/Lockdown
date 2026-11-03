#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TurretProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UPointLightComponent;
class UParticleSystem;
class USoundBase;
class UMaterialInterface;

/**
 * Fast straight-flying glowing bolt fired by Small/Medium turrets.
 * Spawn with the turret as Owner; it damages only the player and ignores its owner.
 */
UCLASS()
class LOCKDOWN_API ATurretProjectile : public AActor
{
	GENERATED_BODY()

public:
	ATurretProjectile();

	/** Re-aims the bolt after spawning (optional; spawning with the right rotation also works). */
	void FireInDirection(const FVector& Direction);

	/** Damage dealt to the player on hit (set by the firing turret). */
	void SetDamage(float NewDamage) { Damage = NewDamage; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Turret Projectile")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Turret Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Turret Projectile")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lockdown|Turret Projectile")
	TObjectPtr<UPointLightComponent> GlowLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Turret Projectile")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Turret Projectile")
	float Speed = 3500.f;

	/** Safety net so missed shots do not live forever. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Turret Projectile")
	float MaxLifetime = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Turret Projectile|Visual")
	FLinearColor BoltColor = FLinearColor(1.f, 0.1f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Turret Projectile|FX")
	TObjectPtr<UParticleSystem> ImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Turret Projectile|FX")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lockdown|Turret Projectile|Visual")
	TObjectPtr<UMaterialInterface> GlowMaterial;

private:
	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);
};

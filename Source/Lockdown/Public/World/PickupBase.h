#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupBase.generated.h"

class USphereComponent;
class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class USoundBase;
class UPrimitiveComponent;
class ALockdownCharacter;

/**
 * Base class for collectables: spinning, bobbing mesh with a coloured glow and an overlap sphere.
 * Subclasses override ApplyToCharacter() and set their own mesh / GlowColor in their constructor.
 */
UCLASS(Abstract)
class LOCKDOWN_API APickupBase : public AActor
{
	GENERATED_BODY()

public:
	APickupBase();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** Gives the pickup's benefit to the character. Return false to leave the pickup in the world. */
	virtual bool ApplyToCharacter(ALockdownCharacter* Character) PURE_VIRTUAL(APickupBase::ApplyToCharacter, return false;);

	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<USphereComponent> TriggerSphere;

	/** Everything visual hangs off this so spin/bob moves all of it together. */
	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<USceneComponent> VisualRoot;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UPointLightComponent> GlowLight;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Pickup")
	FLinearColor GlowColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Pickup")
	float GlowIntensity = 4.f;

	/** Degrees per second around Z. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Pickup")
	float SpinSpeed = 90.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Pickup")
	float BobAmplitude = 10.f;

	/** Bob cycles per second. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Pickup")
	float BobFrequency = 0.8f;

	/** Resting height of the visuals above the actor origin. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Pickup")
	float HoverHeight = 60.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Pickup")
	bool bRespawns = false;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Pickup", meta = (EditCondition = "bRespawns", ClampMin = "1"))
	float RespawnTime = 20.f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Pickup")
	TObjectPtr<USoundBase> PickupSound;

private:
	void Collect();
	void Respawn();
	void SetActive(bool bActive);

	FTimerHandle RespawnTimer;
	float BobTime = 0.f;
	bool bCollected = false;
};

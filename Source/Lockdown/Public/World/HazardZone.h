#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/LockdownTypes.h"
#include "HazardZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UPrimitiveComponent;

/**
 * Trigger volume that applies a status effect to the player while they stand inside.
 * The effect is applied on entry and refreshed on a timer, so it only starts counting down once the player leaves.
 */
UCLASS()
class LOCKDOWN_API AHazardZone : public AActor
{
	GENERATED_BODY()

public:
	AHazardZone();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	/** Sizes/positions the visual mesh to match the trigger box. Override for different shapes (e.g. flat plane). */
	virtual void UpdateVisualLayout();

	UFUNCTION()
	void OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UPointLightComponent> ZoneLight;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Hazard")
	EStatusEffect Effect = EStatusEffect::Slowed;

	/** Seconds the effect lasts after the last refresh. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Hazard", meta = (ClampMin = "0.1"))
	float Duration = 10.f;

	/** How often the effect is refreshed while the player stays inside. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Hazard", meta = (ClampMin = "0.05"))
	float RefreshInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Hazard")
	FVector BoxExtent = FVector(200.f, 200.f, 100.f);

	/** Colour passed to the "Color" parameter of the visual material (alpha = opacity if the material uses it). */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Hazard")
	FLinearColor ZoneColor = FLinearColor(1.f, 0.f, 0.f, 0.3f);

	UPROPERTY(EditAnywhere, Category = "Lockdown|Hazard")
	float LightIntensity = 3.f;

private:
	/** Re-applies the effect to every overlapping player character. */
	void RefreshOverlappingActors();
	void ApplyEffectTo(AActor* Actor);

	FTimerHandle RefreshTimer;
};

#pragma once

#include "CoreMinimal.h"
#include "World/HazardZone.h"
#include "AcidPuddle.generated.h"

/** Flat green acid pool: slows the player. Visual is a thin plane lying on the floor. */
UCLASS()
class LOCKDOWN_API AAcidPuddle : public AHazardZone
{
	GENERATED_BODY()

public:
	AAcidPuddle();

protected:
	virtual void UpdateVisualLayout() override;

	/** Height of the plane above the bottom of the trigger box (avoids z-fighting with the floor). */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Hazard")
	float SurfaceLift = 3.f;
};

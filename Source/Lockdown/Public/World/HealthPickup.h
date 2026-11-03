#pragma once

#include "CoreMinimal.h"
#include "World/PickupBase.h"
#include "HealthPickup.generated.h"

/** Green medkit: restores the player to full health. Drawn as a 3D plus sign. */
UCLASS()
class LOCKDOWN_API AHealthPickup : public APickupBase
{
	GENERATED_BODY()

public:
	AHealthPickup();

protected:
	virtual bool ApplyToCharacter(ALockdownCharacter* Character) override;

	/** Second bar of the plus sign (the base Mesh is the vertical bar). */
	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> CrossBarMesh;
};

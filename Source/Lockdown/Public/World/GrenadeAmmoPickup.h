#pragma once

#include "CoreMinimal.h"
#include "World/PickupBase.h"
#include "GrenadeAmmoPickup.generated.h"

/** Cyan ammo canister: adds grenades for the launcher. */
UCLASS()
class LOCKDOWN_API AGrenadeAmmoPickup : public APickupBase
{
	GENERATED_BODY()

public:
	AGrenadeAmmoPickup();

protected:
	virtual bool ApplyToCharacter(ALockdownCharacter* Character) override;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Pickup", meta = (ClampMin = "1"))
	int32 AmmoAmount = 3;
};

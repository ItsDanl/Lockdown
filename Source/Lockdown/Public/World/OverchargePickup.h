#pragma once

#include "CoreMinimal.h"
#include "World/PickupBase.h"
#include "OverchargePickup.generated.h"

/** Orange energy core: grants the DoubleDamage status effect for a while. */
UCLASS()
class LOCKDOWN_API AOverchargePickup : public APickupBase
{
	GENERATED_BODY()

public:
	AOverchargePickup();

protected:
	virtual bool ApplyToCharacter(ALockdownCharacter* Character) override;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Pickup")
	float EffectDuration = 30.f;
};

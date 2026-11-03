#pragma once

#include "CoreMinimal.h"
#include "World/HazardZone.h"
#include "SignalJammerZone.generated.h"

/** Translucent purple field that jams the player's weapon while inside. */
UCLASS()
class LOCKDOWN_API ASignalJammerZone : public AHazardZone
{
	GENERATED_BODY()

public:
	ASignalJammerZone();
};

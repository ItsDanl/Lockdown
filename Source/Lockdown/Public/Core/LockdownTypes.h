#pragma once

#include "CoreMinimal.h"
#include "LockdownTypes.generated.h"

/** Timed gameplay effects that can be applied to the player. */
UENUM(BlueprintType)
enum class EStatusEffect : uint8
{
	None,
	Slowed,
	WeaponJammed,
	DoubleDamage
};

/** Turret tiers; each sets its own health, fire rate and weapon. */
UENUM(BlueprintType)
enum class ETurretType : uint8
{
	Small,
	Medium,
	Large
};

/** Outcome of a match. */
UENUM(BlueprintType)
enum class EMatchResult : uint8
{
	None,
	Won,
	LostHealth,
	LostTime
};

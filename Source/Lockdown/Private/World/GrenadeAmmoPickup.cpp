#include "World/GrenadeAmmoPickup.h"

#include "World/LockdownWorldHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Player/LockdownCharacter.h"

namespace
{
	const FVector CanisterScale(0.25f, 0.25f, 0.45f);
}

AGrenadeAmmoPickup::AGrenadeAmmoPickup()
{
	GlowColor = FLinearColor(0.f, 0.9f, 1.f); // cyan
	LockdownWorld::SetBasicMesh(Mesh, TEXT("Cylinder"));
	Mesh->SetRelativeScale3D(CanisterScale);
}

bool AGrenadeAmmoPickup::ApplyToCharacter(ALockdownCharacter* Character)
{
	if (!Character)
	{
		return false;
	}
	Character->AddGrenadeAmmo(AmmoAmount);
	return true;
}

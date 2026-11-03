#include "World/OverchargePickup.h"

#include "World/LockdownWorldHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Components/StatusEffectComponent.h"
#include "Core/LockdownTypes.h"
#include "Player/LockdownCharacter.h"

namespace
{
	const FVector CoreScale(0.45f);
}

AOverchargePickup::AOverchargePickup()
{
	GlowColor = FLinearColor(1.f, 0.45f, 0.f); // orange
	LockdownWorld::SetBasicMesh(Mesh, TEXT("Sphere"));
	Mesh->SetRelativeScale3D(CoreScale);
}

bool AOverchargePickup::ApplyToCharacter(ALockdownCharacter* Character)
{
	UStatusEffectComponent* Effects = Character ? Character->GetStatusEffectComponent() : nullptr;
	if (!Effects)
	{
		return false;
	}
	Effects->ApplyEffect(EStatusEffect::DoubleDamage, EffectDuration);
	return true;
}

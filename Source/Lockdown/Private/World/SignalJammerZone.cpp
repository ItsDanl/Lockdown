#include "World/SignalJammerZone.h"

#include "Components/BoxComponent.h"

namespace
{
	constexpr float JammerDuration = 20.f;
	const FVector JammerBoxExtent(200.f, 200.f, 150.f);
}

ASignalJammerZone::ASignalJammerZone()
{
	Effect = EStatusEffect::WeaponJammed;
	Duration = JammerDuration;
	BoxExtent = JammerBoxExtent;
	ZoneColor = FLinearColor(0.55f, 0.1f, 1.f, 0.25f); // translucent purple
	LightIntensity = 4.f;

	TriggerBox->SetBoxExtent(BoxExtent);
	UpdateVisualLayout(); // base cube fills the box
}

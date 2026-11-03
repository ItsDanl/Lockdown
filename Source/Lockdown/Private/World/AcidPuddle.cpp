#include "World/AcidPuddle.h"

#include "World/LockdownWorldHelpers.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

namespace
{
	constexpr float AcidDuration = 15.f;
	const FVector AcidBoxExtent(200.f, 200.f, 40.f);
	constexpr float EngineShapeSize = 100.f;
}

AAcidPuddle::AAcidPuddle()
{
	Effect = EStatusEffect::Slowed;
	Duration = AcidDuration;
	BoxExtent = AcidBoxExtent;
	ZoneColor = FLinearColor(0.15f, 0.9f, 0.1f, 0.85f);
	LightIntensity = 2.f;

	LockdownWorld::SetBasicMesh(VisualMesh, TEXT("Plane"));
	TriggerBox->SetBoxExtent(BoxExtent);
	UpdateVisualLayout();
}

void AAcidPuddle::UpdateVisualLayout()
{
	// Plane lies flat at the bottom of the box (box origin is its centre).
	VisualMesh->SetRelativeLocation(FVector(0.f, 0.f, -BoxExtent.Z + SurfaceLift));
	VisualMesh->SetRelativeScale3D(FVector(BoxExtent.X * 2.f / EngineShapeSize, BoxExtent.Y * 2.f / EngineShapeSize, 1.f));
}

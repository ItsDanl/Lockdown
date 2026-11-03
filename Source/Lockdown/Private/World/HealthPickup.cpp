#include "World/HealthPickup.h"

#include "World/LockdownWorldHelpers.h"
#include "Components/HealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Player/LockdownCharacter.h"

namespace
{
	const FVector VerticalBarScale(0.15f, 0.15f, 0.55f);
	const FVector HorizontalBarScale(0.15f, 0.55f, 0.15f);
}

AHealthPickup::AHealthPickup()
{
	GlowColor = FLinearColor(0.1f, 1.f, 0.2f); // green

	Mesh->SetRelativeScale3D(VerticalBarScale);

	CrossBarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrossBarMesh"));
	CrossBarMesh->SetupAttachment(VisualRoot);
	LockdownWorld::SetBasicMesh(CrossBarMesh, TEXT("Cube"));
	CrossBarMesh->SetRelativeScale3D(HorizontalBarScale);
	CrossBarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrossBarMesh->SetCastShadow(false);
}

bool AHealthPickup::ApplyToCharacter(ALockdownCharacter* Character)
{
	UHealthComponent* Health = Character ? Character->GetHealthComponent() : nullptr;
	if (!Health)
	{
		return false;
	}
	Health->HealToFull();
	return true;
}

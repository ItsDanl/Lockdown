#include "World/HazardZone.h"

#include "World/LockdownWorldHelpers.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/StatusEffectComponent.h"
#include "Engine/World.h"
#include "Player/LockdownCharacter.h"
#include "TimerManager.h"

namespace
{
	constexpr float EngineShapeSize = 100.f; // BasicShapes are 100uu across
	constexpr float LightRadius = 400.f;
}

AHazardZone::AHazardZone()
{
	PrimaryActorTick.bCanEverTick = false; // timer-driven

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(BoxExtent);
	TriggerBox->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	TriggerBox->SetGenerateOverlapEvents(true);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(TriggerBox);
	LockdownWorld::SetBasicMesh(VisualMesh, TEXT("Cube"));
	LockdownWorld::SetBasicMaterial(VisualMesh);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCastShadow(false);

	ZoneLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ZoneLight"));
	ZoneLight->SetupAttachment(TriggerBox);
	ZoneLight->SetAttenuationRadius(LightRadius);
	ZoneLight->SetCastShadows(false);

	UpdateVisualLayout();
}

void AHazardZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	TriggerBox->SetBoxExtent(BoxExtent);
	UpdateVisualLayout();
	ZoneLight->SetLightColor(ZoneColor);
	ZoneLight->SetIntensity(LightIntensity);
}

void AHazardZone::UpdateVisualLayout()
{
	// Default: a cube that fills the trigger box exactly.
	VisualMesh->SetRelativeLocation(FVector::ZeroVector);
	VisualMesh->SetRelativeScale3D(BoxExtent * 2.f / EngineShapeSize);
}

void AHazardZone::BeginPlay()
{
	Super::BeginPlay();

	LockdownWorld::ApplyTint(VisualMesh, ZoneColor);
	ZoneLight->SetLightColor(ZoneColor);
	ZoneLight->SetIntensity(LightIntensity);

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AHazardZone::OnZoneBeginOverlap);
	GetWorldTimerManager().SetTimer(RefreshTimer, this, &AHazardZone::RefreshOverlappingActors, RefreshInterval, true);
}

void AHazardZone::OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ApplyEffectTo(OtherActor); // instant feedback on entry
}

void AHazardZone::RefreshOverlappingActors()
{
	TArray<AActor*> Overlapping;
	TriggerBox->GetOverlappingActors(Overlapping, ALockdownCharacter::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		ApplyEffectTo(Actor);
	}
}

void AHazardZone::ApplyEffectTo(AActor* Actor)
{
	if (ALockdownCharacter* Character = Cast<ALockdownCharacter>(Actor))
	{
		if (UStatusEffectComponent* Effects = Character->GetStatusEffectComponent())
		{
			Effects->ApplyEffect(Effect, Duration);
		}
	}
}

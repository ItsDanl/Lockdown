#include "World/LevelGate.h"

#include "World/LockdownWorldHelpers.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"

namespace
{
	constexpr float EngineCubeSize = 100.f;  // /Engine/BasicShapes/Cube is 100uu per side
	constexpr float SlideOvershoot = 1.02f;  // slide slightly past the panel width so no sliver remains in the doorway
	constexpr float LightHeightFraction = 1.05f;
	constexpr float LightForwardOffset = 60.f;
}

ALevelGate::ALevelGate()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // only ticks while the doors move

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	LeftPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftPanel"));
	LeftPanel->SetupAttachment(Root);
	LockdownWorld::SetBasicMesh(LeftPanel, TEXT("Cube"));
	LeftPanel->SetMobility(EComponentMobility::Movable);

	RightPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightPanel"));
	RightPanel->SetupAttachment(Root);
	LockdownWorld::SetBasicMesh(RightPanel, TEXT("Cube"));
	RightPanel->SetMobility(EComponentMobility::Movable);

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(Root);
	StatusLight->SetAttenuationRadius(450.f);
	StatusLight->SetIntensity(5.f);
	StatusLight->SetCastShadows(false);
	StatusLight->SetLightColor(ClosedColor);

	LayoutPanels();
}

void ALevelGate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayoutPanels();
	ApplyOpenAlpha(OpenProgress);
}

void ALevelGate::BeginPlay()
{
	Super::BeginPlay();
	LayoutPanels();

	LockdownWorld::ApplyTint(LeftPanel, PanelColor);
	LockdownWorld::ApplyTint(RightPanel, PanelColor);
	StatusLight->SetLightColor(ClosedColor);
	ApplyOpenAlpha(0.f);
}

void ALevelGate::LayoutPanels()
{
	const float PanelWidth = GateWidth * 0.5f;
	const FVector PanelScale(PanelThickness / EngineCubeSize, PanelWidth / EngineCubeSize, GateHeight / EngineCubeSize);
	LeftPanel->SetRelativeScale3D(PanelScale);
	RightPanel->SetRelativeScale3D(PanelScale);
	StatusLight->SetRelativeLocation(FVector(LightForwardOffset, 0.f, GateHeight * LightHeightFraction));
}

void ALevelGate::ApplyOpenAlpha(float Alpha)
{
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Alpha, EaseExponent);
	const float PanelWidth = GateWidth * 0.5f;
	// Closed: panels meet at Y=0. Open: each slides outward by a full panel width (into the wall).
	const float Slide = PanelWidth * SlideOvershoot * Eased;
	const float CentreZ = GateHeight * 0.5f;

	LeftPanel->SetRelativeLocation(FVector(0.f, -(PanelWidth * 0.5f) - Slide, CentreZ));
	RightPanel->SetRelativeLocation(FVector(0.f, (PanelWidth * 0.5f) + Slide, CentreZ));
}

void ALevelGate::Open()
{
	if (bIsOpen)
	{
		return;
	}
	bIsOpen = true;
	StatusLight->SetLightColor(OpenColor);
	if (OpenSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, OpenSound, GetActorLocation());
	}
	SetActorTickEnabled(true);
}

void ALevelGate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	OpenProgress = FMath::Min(1.f, OpenProgress + DeltaSeconds / OpenDuration);
	ApplyOpenAlpha(OpenProgress);

	if (OpenProgress >= 1.f)
	{
		SetActorTickEnabled(false); // done: stop ticking
	}
}

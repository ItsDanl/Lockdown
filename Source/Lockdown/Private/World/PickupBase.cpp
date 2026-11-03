#include "World/PickupBase.h"

#include "World/LockdownWorldHelpers.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/LockdownCharacter.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace
{
	constexpr float TriggerRadius = 90.f;
	const FVector DefaultMeshScale(0.4f);
	constexpr float LightRadius = 350.f;
}

APickupBase::APickupBase()
{
	PrimaryActorTick.bCanEverTick = true;

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	SetRootComponent(TriggerSphere);
	TriggerSphere->InitSphereRadius(TriggerRadius);
	TriggerSphere->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
	TriggerSphere->SetGenerateOverlapEvents(true);

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(TriggerSphere);
	VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, HoverHeight));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(VisualRoot);
	LockdownWorld::SetBasicMesh(Mesh, TEXT("Cube"));
	Mesh->SetRelativeScale3D(DefaultMeshScale);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);

	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(TriggerSphere);
	GlowLight->SetRelativeLocation(FVector(0.f, 0.f, HoverHeight));
	GlowLight->SetAttenuationRadius(LightRadius);
	GlowLight->SetCastShadows(false);
}

void APickupBase::BeginPlay()
{
	Super::BeginPlay();

	// Tint every mesh (subclasses may add extra ones) and the light with the pickup colour.
	TArray<UStaticMeshComponent*> Meshes;
	GetComponents<UStaticMeshComponent>(Meshes);
	for (UStaticMeshComponent* Comp : Meshes)
	{
		LockdownWorld::ApplyTint(Comp, GlowColor);
	}
	GlowLight->SetLightColor(GlowColor);
	GlowLight->SetIntensity(GlowIntensity);

	VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, HoverHeight));
	GlowLight->SetRelativeLocation(FVector(0.f, 0.f, HoverHeight));

	TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &APickupBase::OnPickupOverlap);

	// Desynchronise bobbing between neighbouring pickups.
	BobTime = FMath::FRandRange(0.f, 1.f / FMath::Max(BobFrequency, KINDA_SMALL_NUMBER));
}

void APickupBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bCollected)
	{
		return;
	}

	BobTime += DeltaSeconds;
	const float Bob = BobAmplitude * FMath::Sin(BobTime * BobFrequency * UE_TWO_PI);
	VisualRoot->SetRelativeLocation(FVector(0.f, 0.f, HoverHeight + Bob));
	VisualRoot->AddLocalRotation(FRotator(0.f, SpinSpeed * DeltaSeconds, 0.f));
	GlowLight->SetRelativeLocation(FVector(0.f, 0.f, HoverHeight + Bob));
}

void APickupBase::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bCollected)
	{
		return;
	}
	ALockdownCharacter* Character = Cast<ALockdownCharacter>(OtherActor);
	if (Character && ApplyToCharacter(Character))
	{
		Collect();
	}
}

void APickupBase::Collect()
{
	bCollected = true;
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}

	if (bRespawns)
	{
		SetActive(false);
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &APickupBase::Respawn, RespawnTime, false);
	}
	else
	{
		Destroy();
	}
}

void APickupBase::Respawn()
{
	bCollected = false;
	SetActive(true);
}

void APickupBase::SetActive(bool bActive)
{
	SetActorHiddenInGame(!bActive);
	TriggerSphere->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

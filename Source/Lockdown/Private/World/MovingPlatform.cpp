#include "World/MovingPlatform.h"

#include "World/LockdownWorldHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"

namespace
{
	// Default look: a 400 x 400 x 40 slab (engine cube is 100uu per side).
	const FVector DefaultPlatformScale(4.f, 4.f, 0.4f);
}

AMovingPlatform::AMovingPlatform()
{
	PrimaryActorTick.bCanEverTick = true;
	// Pre-physics so the character's movement base is updated before it simulates this frame.
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	SetRootComponent(PlatformMesh);
	LockdownWorld::SetBasicMesh(PlatformMesh, TEXT("Cube"));
	PlatformMesh->SetRelativeScale3D(DefaultPlatformScale);
	PlatformMesh->SetMobility(EComponentMobility::Movable);
	PlatformMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	PlatformMesh->CanCharacterStepUpOn = ECB_Yes;
}

void AMovingPlatform::BeginPlay()
{
	Super::BeginPlay();

	StartLocation = GetActorLocation();
	EndLocation = GetActorTransform().TransformPosition(TargetOffset);
	PauseRemaining = PauseTime; // brief pause at the start so the player can step on
	bIsMoving = bAutoStart;

	LockdownWorld::ApplyTint(PlatformMesh, PlatformColor);
}

void AMovingPlatform::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Distance = FVector::Dist(StartLocation, EndLocation);
	if (!bIsMoving || Distance < KINDA_SMALL_NUMBER)
	{
		return;
	}

	if (PauseRemaining > 0.f)
	{
		PauseRemaining -= DeltaSeconds;
		return;
	}

	Progress = FMath::Clamp(Progress + Direction * (Speed / Distance) * DeltaSeconds, 0.f, 1.f);

	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Progress, EaseExponent);
	SetActorLocation(FMath::Lerp(StartLocation, EndLocation, Eased));

	// Reached an end: flip direction and wait.
	if ((Direction > 0.f && Progress >= 1.f) || (Direction < 0.f && Progress <= 0.f))
	{
		Direction = -Direction;
		PauseRemaining = PauseTime;
	}
}

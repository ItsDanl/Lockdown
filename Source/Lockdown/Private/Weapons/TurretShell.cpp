#include "Weapons/TurretShell.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Player/LockdownCharacter.h"

namespace
{
	constexpr float ShellCollisionRadius = 18.f;
	// Sphere mesh is 100uu wide: 0.36 -> 36uu shell.
	constexpr float ShellMeshScale = 0.36f;
	constexpr float ShellGlowIntensity = 4000.f;
	constexpr float ShellGlowRadius = 350.f;
	constexpr float ShellFallbackLaunchPitchDeg = 45.f;
	constexpr float ShellMinHorizontalDistance = 1.f;
}

ATurretShell::ATurretShell()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(ShellCollisionRadius);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	SetRootComponent(Collision);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetRelativeScale3D(FVector(ShellMeshScale));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereFinder.Succeeded())
	{
		Mesh->SetStaticMesh(SphereFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowFinder(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial"));
	if (GlowFinder.Succeeded())
	{
		GlowMaterial = GlowFinder.Object;
	}

	ShellLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ShellLight"));
	ShellLight->SetupAttachment(Collision);
	ShellLight->SetIntensity(ShellGlowIntensity);
	ShellLight->SetAttenuationRadius(ShellGlowRadius);
	ShellLight->SetLightColor(ShellColor);
	ShellLight->SetCastShadows(false);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = ShellSpeed;
	Movement->MaxSpeed = ShellSpeed * 2.f;
	Movement->ProjectileGravityScale = 1.f;
	Movement->bShouldBounce = false;
	Movement->bRotationFollowsVelocity = true;
}

void ATurretShell::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Shooter = GetOwner())
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
	}

	if (GlowMaterial)
	{
		Mesh->SetMaterial(0, GlowMaterial);
		if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("EmissiveColor"), ShellColor);
			MID->SetVectorParameterValue(TEXT("Color"), ShellColor);
		}
	}
	ShellLight->SetLightColor(ShellColor);

	Movement->OnProjectileStop.AddDynamic(this, &ATurretShell::HandleProjectileStop);
	SetLifeSpan(MaxLifetime);
}

void ATurretShell::LaunchTowards(const FVector& TargetLocation)
{
	const FVector Start = GetActorLocation();
	const FVector Delta = TargetLocation - Start;
	const FVector2D Horizontal(Delta.X, Delta.Y);
	const float Distance = FMath::Max(Horizontal.Size(), ShellMinHorizontalDistance);
	const float Height = Delta.Z;
	const float Gravity = FMath::Abs(GetWorld()->GetGravityZ() * Movement->ProjectileGravityScale);
	const float V2 = ShellSpeed * ShellSpeed;

	// Classic ballistic solution for the low arc: tan(theta) = (v^2 - sqrt(v^4 - g(g d^2 + 2 h v^2))) / (g d)
	const float Discriminant = V2 * V2 - Gravity * (Gravity * Distance * Distance + 2.f * Height * V2);
	float Pitch = FMath::DegreesToRadians(ShellFallbackLaunchPitchDeg); // target out of range: lob at 45 degrees
	if (Discriminant >= 0.f && Gravity > KINDA_SMALL_NUMBER)
	{
		Pitch = FMath::Atan((V2 - FMath::Sqrt(Discriminant)) / (Gravity * Distance));
	}

	const FVector2D Dir2D = Horizontal.GetSafeNormal();
	const FVector Velocity(Dir2D.X * FMath::Cos(Pitch), Dir2D.Y * FMath::Cos(Pitch), FMath::Sin(Pitch));
	Movement->Velocity = Velocity * ShellSpeed;
	SetActorRotation(Movement->Velocity.Rotation());
}

void ATurretShell::HandleProjectileStop(const FHitResult& ImpactResult)
{
	Explode();
}

void ATurretShell::DamagePlayersInRadius(const FVector& Origin)
{
	// Deliberately manual (instead of ApplyRadialDamage) so the blast never harms terminals or turrets.
	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(this, ALockdownCharacter::StaticClass(), Players);

	for (AActor* Player : Players)
	{
		const float Distance = FVector::Dist(Origin, Player->GetActorLocation());
		if (Distance > ExplosionRadius)
		{
			continue;
		}
		const float Falloff = 1.f - (Distance / ExplosionRadius);
		UGameplayStatics::ApplyDamage(Player, Damage * Falloff, nullptr, this, UDamageType::StaticClass());
	}
}

void ATurretShell::Explode()
{
	if (bExploded)
	{
		return;
	}
	bExploded = true;

	const FVector Location = GetActorLocation();
	DamagePlayersInRadius(Location);

	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, ExplosionEffect, Location, FRotator::ZeroRotator,
			FVector(ExplosionEffectScale));
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, Location);
	}

	// Hide the shell but keep the actor alive briefly so the light flash is visible.
	Mesh->SetVisibility(false);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Movement->StopMovementImmediately();

	ShellLight->SetAttenuationRadius(ExplosionRadius * 1.5f);
	ShellLight->SetIntensity(ExplosionLightIntensity);
	SetLifeSpan(ExplosionLightDuration);
}

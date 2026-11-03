#include "Weapons/GrenadeProjectile.h"

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

namespace
{
	constexpr float GrenCollisionRadius = 12.f;
	// /Engine/BasicShapes/Sphere is 100uu wide, so this gives a ~24uu grenade matching the collision.
	constexpr float GrenMeshScale = 0.24f;
	constexpr float GrenLaunchSpeed = 1800.f;
	constexpr float GrenMaxSpeed = 2500.f;
	constexpr float GrenBounciness = 0.45f;
	constexpr float GrenFriction = 0.35f;
	constexpr float GrenIdleLightIntensity = 0.f;
}

AGrenadeProjectile::AGrenadeProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// Collision: blocks the world but not pawns (so it never detonates on the thrower) and not
	// visibility (so rifle shots are not eaten by flying grenades).
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(GrenCollisionRadius);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	SetRootComponent(Collision);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetRelativeScale3D(FVector(GrenMeshScale));

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

	ExplosionLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ExplosionLight"));
	ExplosionLight->SetupAttachment(Collision);
	ExplosionLight->SetIntensity(GrenIdleLightIntensity);
	ExplosionLight->SetCastShadows(false);
	ExplosionLight->SetVisibility(false);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = GrenLaunchSpeed;
	Movement->MaxSpeed = GrenMaxSpeed;
	Movement->ProjectileGravityScale = 1.f;
	Movement->bShouldBounce = true;
	Movement->Bounciness = GrenBounciness;
	Movement->Friction = GrenFriction;
	Movement->bRotationFollowsVelocity = false;
}

void AGrenadeProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Never collide with the thrower while flying out of the muzzle.
	if (APawn* ThrowerPawn = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(ThrowerPawn, true);
	}

	// Tint so the grenade reads clearly against dark levels.
	if (GlowMaterial)
	{
		Mesh->SetMaterial(0, GlowMaterial);
		if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("EmissiveColor"), GrenadeColor);
			MID->SetVectorParameterValue(TEXT("Color"), GrenadeColor);
		}
	}

	Movement->OnProjectileBounce.AddDynamic(this, &AGrenadeProjectile::HandleBounce);

	GetWorldTimerManager().SetTimer(FuseTimer, this, &AGrenadeProjectile::Explode, FuseTime, false);
}

void AGrenadeProjectile::HandleBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	++ImpactCount;
	if (ImpactCount >= ImpactsBeforeExplosion)
	{
		Explode();
		return;
	}

	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BounceSound, GetActorLocation());
	}
}

void AGrenadeProjectile::Explode()
{
	if (bExploded)
	{
		return;
	}
	bExploded = true;
	GetWorldTimerManager().ClearTimer(FuseTimer);

	const FVector Location = GetActorLocation();

	// The thrower is immune to their own blast.
	TArray<AActor*> IgnoredActors;
	if (GetInstigator())
	{
		IgnoredActors.Add(GetInstigator());
	}
	UGameplayStatics::ApplyRadialDamage(this, Damage * DamageMultiplier, Location, ExplosionRadius,
		UDamageType::StaticClass(), IgnoredActors, this, GetInstigatorController(), true, ECC_Visibility);

	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, ExplosionEffect, Location, FRotator::ZeroRotator,
			FVector(ExplosionEffectScale));
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, Location);
	}

	// Keep the actor alive just long enough to show the flash, then clean up.
	Mesh->SetVisibility(false);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Movement->StopMovementImmediately();

	ExplosionLight->SetLightColor(ExplosionLightColor);
	ExplosionLight->SetAttenuationRadius(ExplosionRadius * 1.5f);
	ExplosionLight->SetIntensity(ExplosionLightIntensity);
	ExplosionLight->SetVisibility(true);

	SetLifeSpan(ExplosionLightDuration);
}

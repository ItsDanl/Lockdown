#include "Weapons/TurretProjectile.h"

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
#include "Player/LockdownCharacter.h"

namespace
{
	constexpr float BoltCollisionRadius = 10.f;
	// Sphere mesh is 100uu wide: 0.2 -> 20uu bolt.
	constexpr float BoltMeshScale = 0.2f;
	constexpr float BoltGlowIntensity = 2500.f;
	constexpr float BoltGlowRadius = 250.f;
}

ATurretProjectile::ATurretProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// Blocks world + pawns (so it can hit the player) but ignores visibility so the player's
	// rifle can shoot through incoming bolts to reach the turret.
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(BoltCollisionRadius);
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
	Mesh->SetRelativeScale3D(FVector(BoltMeshScale));

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

	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlowLight"));
	GlowLight->SetupAttachment(Collision);
	GlowLight->SetIntensity(BoltGlowIntensity);
	GlowLight->SetAttenuationRadius(BoltGlowRadius);
	GlowLight->SetLightColor(BoltColor);
	GlowLight->SetCastShadows(false);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = Speed;
	Movement->MaxSpeed = Speed;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bShouldBounce = false;
	Movement->bRotationFollowsVelocity = true;
}

void ATurretProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Do not collide with the turret that fired us.
	if (AActor* Shooter = GetOwner())
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
	}

	if (GlowMaterial)
	{
		Mesh->SetMaterial(0, GlowMaterial);
		if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("EmissiveColor"), BoltColor);
			MID->SetVectorParameterValue(TEXT("Color"), BoltColor);
		}
	}
	GlowLight->SetLightColor(BoltColor);

	Movement->OnProjectileStop.AddDynamic(this, &ATurretProjectile::HandleProjectileStop);
	SetLifeSpan(MaxLifetime);
}

void ATurretProjectile::FireInDirection(const FVector& Direction)
{
	const FVector Dir = Direction.GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		return;
	}
	SetActorRotation(Dir.Rotation());
	Movement->Velocity = Dir * Speed;
}

void ATurretProjectile::HandleProjectileStop(const FHitResult& ImpactResult)
{
	// Only the player takes damage; turrets must not shoot each other or the terminals.
	if (ALockdownCharacter* Player = Cast<ALockdownCharacter>(ImpactResult.GetActor()))
	{
		UGameplayStatics::ApplyDamage(Player, Damage, GetOwner() ? GetOwner()->GetInstigatorController() : nullptr,
			this, UDamageType::StaticClass());
	}

	const FVector Location = GetActorLocation();
	if (ImpactEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, ImpactEffect, Location);
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Location);
	}

	Destroy();
}

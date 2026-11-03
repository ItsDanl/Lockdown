#include "World/Terminal.h"

#include "World/LockdownWorldHelpers.h"
#include "World/Turret.h"
#include "World/LevelGate.h"
#include "Components/HealthComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LockdownGameMode.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace
{
	// Layout (actor-local; terminal faces +X).
	const FVector BodyScale(0.6f, 1.0f, 1.4f);
	constexpr float BodyCentreZ = 70.f;
	const FVector ScreenScale(0.04f, 0.8f, 0.5f);
	constexpr float ScreenX = 31.f;
	constexpr float ScreenZ = 100.f;
	constexpr float ScreenTiltPitch = -12.f;
	constexpr float LightX = 90.f;
	constexpr float LightZ = 100.f;
	constexpr float FireballStartScale = 0.3f;
	constexpr float HealthyThreshold = 0.6f;   // above this the screen stays "healthy"
	constexpr float CriticalThreshold = 0.25f; // below this it is fully red
}

ATerminal::ATerminal()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // only ticks during the explosion flash

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(Root);
	LockdownWorld::SetBasicMesh(BodyMesh, TEXT("Cube"));
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, BodyCentreZ));
	BodyMesh->SetRelativeScale3D(BodyScale);

	ScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenMesh"));
	ScreenMesh->SetupAttachment(Root);
	LockdownWorld::SetBasicMesh(ScreenMesh, TEXT("Cube"));
	ScreenMesh->SetRelativeLocation(FVector(ScreenX, 0.f, ScreenZ));
	ScreenMesh->SetRelativeRotation(FRotator(ScreenTiltPitch, 0.f, 0.f));
	ScreenMesh->SetRelativeScale3D(ScreenScale);
	ScreenMesh->SetCastShadow(false);

	ScreenLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ScreenLight"));
	ScreenLight->SetupAttachment(Root);
	ScreenLight->SetRelativeLocation(FVector(LightX, 0.f, LightZ));
	ScreenLight->SetAttenuationRadius(450.f);
	ScreenLight->SetCastShadows(false);

	FireballMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FireballMesh"));
	FireballMesh->SetupAttachment(Root);
	LockdownWorld::SetBasicMesh(FireballMesh, TEXT("Sphere"));
	FireballMesh->SetRelativeLocation(FVector(0.f, 0.f, BodyCentreZ));
	FireballMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FireballMesh->SetCastShadow(false);
	FireballMesh->SetVisibility(false);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}

void ATerminal::BeginPlay()
{
	// Configure health before components run BeginPlay (that is when CurrentHealth is initialised).
	if (HealthComponent)
	{
		HealthComponent->SetMaxHealth(MaxHealth);
	}

	Super::BeginPlay();

	LockdownWorld::ApplyTint(BodyMesh, BodyColor);
	ScreenMID = LockdownWorld::ApplyTint(ScreenMesh, HealthyColor);
	FireballMID = LockdownWorld::ApplyTint(FireballMesh, ExplosionColor);

	CurrentBaseColor = HealthyColor;
	SetScreenColor(HealthyColor);

	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &ATerminal::HandleHealthChanged);
		HealthComponent->OnDeath.AddDynamic(this, &ATerminal::HandleDeath);
	}
}

// ---------------------------------------------------------------- screen feedback

FLinearColor ATerminal::GetHealthColor(float Percent) const
{
	if (Percent >= HealthyThreshold)
	{
		return HealthyColor;
	}
	if (Percent >= CriticalThreshold)
	{
		// healthy-threshold..critical-threshold maps cyan->orange
		const float T = (HealthyThreshold - Percent) / (HealthyThreshold - CriticalThreshold);
		return FMath::Lerp(HealthyColor, DamagedColor, T);
	}
	return CriticalColor;
}

void ATerminal::SetScreenColor(const FLinearColor& Color)
{
	if (ScreenMID)
	{
		ScreenMID->SetVectorParameterValue(LockdownWorld::ColorParamName, Color);
	}
	ScreenLight->SetLightColor(Color);
	ScreenLight->SetIntensity(ScreenLightIntensity);
}

void ATerminal::HandleHealthChanged(float Current, float Max)
{
	if (bIsDestroyed)
	{
		return;
	}
	CurrentBaseColor = GetHealthColor(Max > 0.f ? Current / Max : 0.f);

	// Brief white flash so every hit is readable, then settle on the health colour.
	SetScreenColor(HitFlashColor);
	GetWorldTimerManager().SetTimer(HitFlashTimer, this, &ATerminal::EndHitFlash, HitFlashDuration, false);
}

void ATerminal::EndHitFlash()
{
	if (!bIsDestroyed)
	{
		SetScreenColor(CurrentBaseColor);
	}
}

// ---------------------------------------------------------------- destruction

void ATerminal::HandleDeath()
{
	if (bIsDestroyed)
	{
		return;
	}
	bIsDestroyed = true;
	GetWorldTimerManager().ClearTimer(HitFlashTimer);

	NotifyWorld();
	StartExplosion();
}

void ATerminal::NotifyWorld()
{
	for (ATurret* Turret : ControlledTurrets)
	{
		if (IsValid(Turret))
		{
			Turret->Disable();
		}
	}
	if (IsValid(GateToOpen))
	{
		GateToOpen->Open();
	}
	if (ALockdownGameMode* GameMode = Cast<ALockdownGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GameMode->OnTerminalDestroyed(this);
	}
}

void ATerminal::StartExplosion()
{
	// Screen goes dark, a fireball + light flash sell the blast; then the actor is removed.
	ScreenMesh->SetVisibility(false);
	FireballMesh->SetVisibility(true);
	FireballMesh->SetRelativeScale3D(FVector(FireballStartScale));
	ScreenLight->SetLightColor(ExplosionColor);
	ScreenLight->SetIntensity(ExplosionLightIntensity);

	const FVector Location = GetActorLocation() + FVector(0.f, 0.f, BodyCentreZ);
	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, ExplosionEffect, Location);
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, Location);
	}

	BodyMesh->SetVisibility(false);
	SetActorEnableCollision(false);
	SetActorTickEnabled(true);
	SetLifeSpan(ExplosionFlashTime);
}

void ATerminal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsDestroyed)
	{
		return;
	}
	ExplosionElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(ExplosionElapsed / FMath::Max(ExplosionFlashTime, KINDA_SMALL_NUMBER), 0.f, 1.f);
	const float Grow = 1.f - FMath::Square(1.f - Alpha);
	FireballMesh->SetRelativeScale3D(FVector(FMath::Lerp(FireballStartScale, ExplosionFireballScale, Grow)));
	ScreenLight->SetIntensity(FMath::Lerp(ExplosionLightIntensity, 0.f, Alpha));
}

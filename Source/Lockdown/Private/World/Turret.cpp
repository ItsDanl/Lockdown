#include "World/Turret.h"

#include "World/LockdownWorldHelpers.h"
#include "Components/HealthComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Particles/ParticleSystem.h"
#include "Player/LockdownCharacter.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Weapons/TurretProjectile.h"
#include "Weapons/TurretShell.h"

namespace
{
	/** Per-type tuning, applied when bUseTypePresets is true. */
	struct FTurretPreset
	{
		float Health;
		float FireInterval;
		float TurnSpeed;
		float Scale;
		bool bShells;
	};

	FTurretPreset GetPreset(ETurretType Type)
	{
		switch (Type)
		{
		case ETurretType::Small:  return { 50.f,  0.4f, 8.f, 0.75f, false };
		case ETurretType::Large:  return { 200.f, 2.5f, 3.f, 1.5f,  true  };
		case ETurretType::Medium:
		default:                  return { 100.f, 1.0f, 5.f, 1.0f,  false };
		}
	}

	// Layout constants (actor-local, before preset scaling).
	constexpr float BaseHeight = 20.f;        // base cylinder centre Z
	constexpr float PivotHeight = 60.f;       // head pivot Z
	constexpr float BarrelCentreX = 70.f;
	constexpr float MuzzleX = 125.f;
	constexpr float EyeX = 52.f;
	constexpr float LightZ = 45.f;
	constexpr float FireballStartScale = 0.2f;
	constexpr float TwoPi = 6.2831853f;
}

ATurret::ATurret()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Base: squat cylinder (engine cylinder is 100uu tall, 100uu wide).
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(Root);
	LockdownWorld::SetBasicMesh(BaseMesh, TEXT("Cylinder"));
	BaseMesh->SetRelativeLocation(FVector(0.f, 0.f, BaseHeight));
	BaseMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.4f));

	YawPivot = CreateDefaultSubobject<USceneComponent>(TEXT("YawPivot"));
	YawPivot->SetupAttachment(Root);
	YawPivot->SetRelativeLocation(FVector(0.f, 0.f, PivotHeight));

	PitchPivot = CreateDefaultSubobject<USceneComponent>(TEXT("PitchPivot"));
	PitchPivot->SetupAttachment(YawPivot);

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(PitchPivot);
	LockdownWorld::SetBasicMesh(HeadMesh, TEXT("Cube"));
	HeadMesh->SetRelativeScale3D(FVector(0.6f, 0.5f, 0.4f));

	// Barrel: cylinder rotated so its long axis (Z) points along +X.
	BarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
	BarrelMesh->SetupAttachment(PitchPivot);
	LockdownWorld::SetBasicMesh(BarrelMesh, TEXT("Cylinder"));
	BarrelMesh->SetRelativeLocation(FVector(BarrelCentreX, 0.f, 0.f));
	BarrelMesh->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	BarrelMesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 1.f));

	EyeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EyeMesh"));
	EyeMesh->SetupAttachment(PitchPivot);
	LockdownWorld::SetBasicMesh(EyeMesh, TEXT("Sphere"));
	EyeMesh->SetRelativeLocation(FVector(EyeX, 0.f, 10.f));
	EyeMesh->SetRelativeScale3D(FVector(0.14f));
	EyeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EyeMesh->SetCastShadow(false);

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(PitchPivot);
	Muzzle->SetRelativeLocation(FVector(MuzzleX, 0.f, 0.f));

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(PitchPivot);
	StatusLight->SetRelativeLocation(FVector(0.f, 0.f, LightZ));
	StatusLight->SetAttenuationRadius(500.f);
	StatusLight->SetCastShadows(false); // many turrets + cheap lights = good perf

	FireballMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FireballMesh"));
	FireballMesh->SetupAttachment(Root);
	LockdownWorld::SetBasicMesh(FireballMesh, TEXT("Sphere"));
	FireballMesh->SetRelativeLocation(FVector(0.f, 0.f, PivotHeight));
	FireballMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FireballMesh->SetCastShadow(false);
	FireballMesh->SetVisibility(false);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	ProjectileClass = ATurretProjectile::StaticClass();
	ShellClass = ATurretShell::StaticClass();
}

void ATurret::BeginPlay()
{
	// Health must be configured BEFORE Super::BeginPlay, which is when components initialise CurrentHealth.
	if (bUseTypePresets)
	{
		ApplyTypePreset();
	}
	ConfigureHealth();

	Super::BeginPlay();

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &ATurret::HandleDeath);
	}

	EyeMID = LockdownWorld::ApplyTint(EyeMesh, IdleColor);
	FireballMID = LockdownWorld::ApplyTint(FireballMesh, ExplosionColor);

	CurrentLightColor = IdleColor;
	CurrentLightIntensity = IdleLightIntensity;
	StatusLight->SetLightColor(CurrentLightColor);
	StatusLight->SetIntensity(CurrentLightIntensity);

	// Randomise phases so a room of turrets does not sweep in lockstep or poll on the same frame.
	IdlePhase = FMath::FRandRange(0.f, TwoPi);
	GetWorldTimerManager().SetTimer(VisionTimer, this, &ATurret::VisionCheck, VisionCheckInterval, true,
		FMath::FRandRange(0.f, VisionCheckInterval));
}

// ---------------------------------------------------------------- presets / setup

void ATurret::ApplyTypePreset()
{
	const FTurretPreset Preset = GetPreset(TurretType);
	MaxHealth = Preset.Health;
	FireInterval = Preset.FireInterval;
	TurnSpeed = Preset.TurnSpeed;
	bFireShells = Preset.bShells;
	SetActorScale3D(GetActorScale3D() * Preset.Scale);
}

void ATurret::ConfigureHealth()
{
	if (HealthComponent)
	{
		HealthComponent->SetMaxHealth(MaxHealth);
	}
}

// ---------------------------------------------------------------- vision

void ATurret::VisionCheck()
{
	if (bIsDisabled || bIsExploding)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const FVector Eye = PitchPivot->GetComponentLocation();
	APawn* Player = FindPlayerTarget();

	bool bCanSee = false;
	if (Player)
	{
		const FVector PlayerLoc = Player->GetActorLocation();
		const bool bTracking = AlertState != ETurretAlertState::Idle;
		// The cone only matters for acquiring the player; once alerted we keep tracking in 360 degrees.
		const bool bInRange = FVector::DistSquared(Eye, PlayerLoc) <= FMath::Square(DetectionRange);
		const bool bInCone = bTracking || IsInVisionCone(Eye, PlayerLoc);
		bCanSee = bInRange && bInCone && HasLineOfSight(Eye, Player);
	}

	if (bCanSee)
	{
		TargetPawn = Player;
		LastSeenTime = Now;
		LastKnownTargetLocation = Player->GetActorLocation() + Player->GetVelocity() * LeadTime;

		if (AlertState == ETurretAlertState::Idle)
		{
			SuspicionStartTime = Now;
			SetAlertState(ETurretAlertState::Suspicious);
		}
		else if (AlertState == ETurretAlertState::Suspicious && Now - SuspicionStartTime >= ReactionDelay)
		{
			SetAlertState(ETurretAlertState::Firing);
		}
	}
	else if (AlertState != ETurretAlertState::Idle && Now - LastSeenTime >= LoseSightTime)
	{
		ForgetTarget();
	}
}

APawn* ATurret::FindPlayerTarget() const
{
	ALockdownCharacter* Character = Cast<ALockdownCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Character)
	{
		return nullptr;
	}
	const UHealthComponent* TargetHealth = Character->GetHealthComponent();
	return (TargetHealth && TargetHealth->IsDead()) ? nullptr : Character;
}

bool ATurret::IsInVisionCone(const FVector& Eye, const FVector& TargetLocation) const
{
	const FVector ToTarget = (TargetLocation - Eye).GetSafeNormal();
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(HalfAngleDegrees));
	return FVector::DotProduct(PitchPivot->GetForwardVector(), ToTarget) >= CosHalfAngle;
}

bool ATurret::HasLineOfSight(const FVector& Eye, AActor* Target) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TurretLOS), false, this);

	// Trace to body centre and to the upper body, so a player peeking over cover is still seen.
	const FVector Centre = Target->GetActorLocation();
	FVector Origin, Extent;
	Target->GetActorBounds(true, Origin, Extent);
	const FVector Points[2] = { Centre, FVector(Centre.X, Centre.Y, Centre.Z + Extent.Z * 0.8f) };

	for (const FVector& Point : Points)
	{
		FHitResult Hit;
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Eye, Point, ECC_Visibility, Params);
		// Player capsules usually ignore Visibility (no hit); a hit on the player also counts as seen.
		if (!bBlocked || Hit.GetActor() == Target)
		{
			return true;
		}
	}
	return false;
}

void ATurret::SetAlertState(ETurretAlertState NewState)
{
	if (AlertState == NewState)
	{
		return;
	}
	AlertState = NewState;

	if (NewState == ETurretAlertState::Firing)
	{
		// First shot happens next frame; the ReactionDelay was already served in Suspicious.
		GetWorldTimerManager().SetTimer(FireTimer, this, &ATurret::Fire, FireInterval, true, 0.f);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(FireTimer);
	}
}

void ATurret::ForgetTarget()
{
	TargetPawn.Reset();
	SetAlertState(ETurretAlertState::Idle);
}

// ---------------------------------------------------------------- firing

bool ATurret::CanFire() const
{
	if (bIsDisabled || bIsExploding || AlertState != ETurretAlertState::Firing)
	{
		return false;
	}
	// Never shoot at a player we have not actually seen very recently (no firing through walls).
	return GetWorld()->GetTimeSeconds() - LastSeenTime <= VisionCheckInterval * 2.5f;
}

void ATurret::Fire()
{
	if (!CanFire())
	{
		return;
	}

	FRotator SpawnRotation = Muzzle->GetComponentRotation();
	if (bFireShells)
	{
		SpawnRotation.Pitch += ShellPitchBias;
	}
	const FVector SpawnLocation = Muzzle->GetComponentLocation();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (bFireShells)
	{
		if (ShellClass)
		{
			if (ATurretShell* Shell = GetWorld()->SpawnActor<ATurretShell>(ShellClass, SpawnLocation, SpawnRotation, Params))
			{
				Shell->LaunchTowards(LastKnownTargetLocation);
			}
		}
	}
	else if (ProjectileClass)
	{
		if (ATurretProjectile* Bolt = GetWorld()->SpawnActor<ATurretProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, Params))
		{
			Bolt->SetDamage(ProjectileDamage);
		}
	}

	MuzzleFlashRemaining = MuzzleFlashDuration;
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, SpawnLocation);
	}
}

// ---------------------------------------------------------------- per-frame visuals

void ATurret::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsExploding)
	{
		TickExplosion(DeltaSeconds);
		return;
	}
	UpdateAim(DeltaSeconds);
	UpdateLight(DeltaSeconds);
}

FRotator ATurret::ComputeDesiredAim(float DeltaSeconds)
{
	if (bIsDisabled)
	{
		return FRotator(-DisabledDroopPitch, AimRotation.Yaw, 0.f);
	}

	if (AlertState != ETurretAlertState::Idle)
	{
		// Aim in actor space so the turret works when mounted on a rotated actor.
		const FVector ToTarget = LastKnownTargetLocation - PitchPivot->GetComponentLocation();
		const FVector LocalDir = GetActorTransform().InverseTransformVectorNoScale(ToTarget.GetSafeNormal());
		FRotator Desired = LocalDir.Rotation();
		Desired.Pitch = FMath::Clamp(Desired.Pitch, -MaxPitchDown, MaxPitchUp);
		Desired.Roll = 0.f;
		return Desired;
	}

	// Idle: slow sine sweep across +-IdleSweepAngle.
	IdlePhase += DeltaSeconds * IdleSweepSpeed;
	return FRotator(0.f, IdleSweepAngle * FMath::Sin(IdlePhase), 0.f);
}

void ATurret::UpdateAim(float DeltaSeconds)
{
	const FRotator Desired = ComputeDesiredAim(DeltaSeconds);
	const float Speed = bIsDisabled ? DisabledTurnSpeed : TurnSpeed;
	AimRotation = FMath::RInterpTo(AimRotation, Desired, DeltaSeconds, Speed);

	YawPivot->SetRelativeRotation(FRotator(0.f, AimRotation.Yaw, 0.f));
	PitchPivot->SetRelativeRotation(FRotator(AimRotation.Pitch, 0.f, 0.f));
}

FLinearColor ATurret::GetStateColor() const
{
	switch (AlertState)
	{
	case ETurretAlertState::Suspicious: return SuspiciousColor;
	case ETurretAlertState::Firing:     return FiringColor;
	default:                            return IdleColor;
	}
}

void ATurret::UpdateLight(float DeltaSeconds)
{
	float TargetIntensity = 0.f; // disabled = lights out
	if (!bIsDisabled)
	{
		TargetIntensity = (AlertState == ETurretAlertState::Idle) ? IdleLightIntensity : AlertLightIntensity;
	}

	CurrentLightColor = FMath::CInterpTo(CurrentLightColor, bIsDisabled ? FLinearColor::Black : GetStateColor(), DeltaSeconds, LightColorInterpSpeed);
	CurrentLightIntensity = FMath::FInterpTo(CurrentLightIntensity, TargetIntensity, DeltaSeconds, bIsDisabled ? LightFadeSpeed : LightColorInterpSpeed);

	float Flash = 0.f;
	if (MuzzleFlashRemaining > 0.f)
	{
		MuzzleFlashRemaining = FMath::Max(0.f, MuzzleFlashRemaining - DeltaSeconds);
		Flash = MuzzleFlashBoost * (MuzzleFlashRemaining / FMath::Max(MuzzleFlashDuration, KINDA_SMALL_NUMBER));
	}

	StatusLight->SetLightColor(CurrentLightColor);
	StatusLight->SetIntensity(CurrentLightIntensity + Flash);
	if (EyeMID)
	{
		EyeMID->SetVectorParameterValue(LockdownWorld::ColorParamName, CurrentLightColor);
	}
}

// ---------------------------------------------------------------- disable / death

void ATurret::Disable()
{
	if (bIsDisabled || bIsExploding)
	{
		return;
	}
	bIsDisabled = true;
	GetWorldTimerManager().ClearTimer(VisionTimer);
	GetWorldTimerManager().ClearTimer(FireTimer);
	TargetPawn.Reset();
	AlertState = ETurretAlertState::Idle;
}

void ATurret::HandleDeath()
{
	Explode();
}

void ATurret::Explode()
{
	if (bIsExploding)
	{
		return;
	}
	bIsExploding = true;
	bIsDisabled = true;
	GetWorldTimerManager().ClearTimer(VisionTimer);
	GetWorldTimerManager().ClearTimer(FireTimer);
	SetActorEnableCollision(false);

	// Hide the body; the light + fireball sell the blast until the actor is destroyed.
	BaseMesh->SetVisibility(false);
	HeadMesh->SetVisibility(false);
	BarrelMesh->SetVisibility(false);
	EyeMesh->SetVisibility(false);
	FireballMesh->SetVisibility(true);
	FireballMesh->SetRelativeScale3D(FVector(FireballStartScale));

	StatusLight->SetLightColor(ExplosionColor);
	StatusLight->SetIntensity(ExplosionLightIntensity);

	const FVector Location = GetActorLocation() + FVector(0.f, 0.f, PivotHeight);
	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, ExplosionEffect, Location);
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, Location);
	}

	SetLifeSpan(ExplosionFlashTime);
}

void ATurret::TickExplosion(float DeltaSeconds)
{
	ExplosionElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(ExplosionElapsed / FMath::Max(ExplosionFlashTime, KINDA_SMALL_NUMBER), 0.f, 1.f);

	// Fireball expands fast then slows (ease-out); light fades linearly.
	const float Grow = 1.f - FMath::Square(1.f - Alpha);
	FireballMesh->SetRelativeScale3D(FVector(FMath::Lerp(FireballStartScale, ExplosionFireballScale, Grow)));
	StatusLight->SetIntensity(FMath::Lerp(ExplosionLightIntensity, 0.f, Alpha));
}

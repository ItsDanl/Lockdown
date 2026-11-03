#include "Player/LockdownCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/HealthComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StatusEffectComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/LockdownGameMode.h"
#include "Core/LockdownTypes.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "Player/LockdownPlayerController.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapons/GrenadeProjectile.h"

// Enhanced Input
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

namespace
{
	constexpr float CharCapsuleRadius = 42.f;
	constexpr float CharCapsuleHalfHeight = 96.f;
	constexpr float CharAirControl = 0.35f;
	constexpr float CharFlashLightRadius = 450.f;

	// Engine cube/cylinder are 100uu; the cylinder is 100uu tall along Z.
	constexpr float CharCylinderHeight = 100.f;

	// First-person view-model layout, relative to the camera.
	const FVector CharMuzzleOffset(65.f, 18.f, -14.f);
	const FVector CharArmLocation(14.f, 24.f, -30.f);
	const FRotator CharArmRotation(80.f, 0.f, 0.f);
	const FVector CharArmScale(0.09f, 0.09f, 0.55f);
	const FLinearColor CharArmColor(0.03f, 0.03f, 0.04f);
}

// ---------------------------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------------------------

ALockdownCharacter::ALockdownCharacter()
{
	// Everything is event/timer driven.
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(CharCapsuleRadius, CharCapsuleHalfHeight);

	// Body follows the camera yaw; pitch/roll stay on the camera only.
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = BaseWalkSpeed;
	Move->JumpZVelocity = JumpVelocity;
	Move->AirControl = CharAirControl;

	// Default engine shapes so the character works without a Blueprint subclass.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMatFinder(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial"));
	if (CubeFinder.Succeeded()) { CubeShape = CubeFinder.Object; }
	if (CylinderFinder.Succeeded()) { CylinderShape = CylinderFinder.Object; }
	if (BasicMatFinder.Succeeded()) { BasicShapeMaterial = BasicMatFinder.Object; }
	if (GlowMatFinder.Succeeded()) { GlowMaterial = GlowMatFinder.Object; }

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, EyeHeight));
	FirstPersonCamera->bUsePawnControlRotation = true;

	// Simple blocky arm + gun view-model, attached to the camera so it follows the view.
	ArmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArmMesh"));
	ArmMesh->SetupAttachment(FirstPersonCamera);
	ArmMesh->SetStaticMesh(CylinderShape);
	ArmMesh->SetRelativeLocation(CharArmLocation);
	ArmMesh->SetRelativeRotation(CharArmRotation);
	ArmMesh->SetRelativeScale3D(CharArmScale);
	ArmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArmMesh->SetCastShadow(false);

	GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
	GunMesh->SetupAttachment(FirstPersonCamera);
	GunMesh->SetStaticMesh(CubeShape);
	GunMesh->SetRelativeLocation(GunRestLocation);
	GunMesh->SetRelativeScale3D(RifleGunScale);
	GunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GunMesh->SetCastShadow(false);

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(FirstPersonCamera);
	Muzzle->SetRelativeLocation(CharMuzzleOffset);

	// Muzzle flash = a point light that is only visible for a few frames per shot.
	MuzzleFlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleFlashLight"));
	MuzzleFlashLight->SetupAttachment(Muzzle);
	MuzzleFlashLight->SetAttenuationRadius(CharFlashLightRadius);
	MuzzleFlashLight->SetCastShadows(false);
	MuzzleFlashLight->SetVisibility(false);

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	StatusEffectComponent = CreateDefaultSubobject<UStatusEffectComponent>(TEXT("StatusEffectComponent"));

	GrenadeClass = AGrenadeProjectile::StaticClass();
}

void ALockdownCharacter::BeginPlay()
{
	Super::BeginPlay();

	GrenadeAmmo = MaxGrenadeAmmo;

	HealthComponent->OnDeath.AddDynamic(this, &ALockdownCharacter::HandleDeath);
	OnTakeAnyDamage.AddDynamic(this, &ALockdownCharacter::HandleTakeAnyDamage);
	StatusEffectComponent->OnStatusEffectsChanged.AddDynamic(this, &ALockdownCharacter::HandleStatusEffectsChanged);

	TintMesh(ArmMesh, BasicShapeMaterial, CharArmColor);
	ApplyWeaponVisuals();
	UpdateMovementSpeed();
}

// ---------------------------------------------------------------------------------------------
// Input setup (all assets created in code)
// ---------------------------------------------------------------------------------------------

UInputAction* ALockdownCharacter::MakeAction(const TCHAR* Name, bool bAxis2D)
{
	UInputAction* Action = NewObject<UInputAction>(this, MakeUniqueObjectName(this, UInputAction::StaticClass(), FName(Name)));
	Action->ValueType = bAxis2D ? EInputActionValueType::Axis2D : EInputActionValueType::Boolean;
	return Action;
}

void ALockdownCharacter::MapWasdKeys()
{
	// Move action value convention: X = strafe right, Y = forward.
	// W/S need a swizzle to move the key's X value onto Y; S and A also need negating.
	auto MapMoveKey = [this](const FKey& Key, bool bSwizzleToY, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(MoveAction, Key);
		if (bSwizzleToY)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(MappingContext);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext));
		}
	};

	MapMoveKey(EKeys::W, true, false);
	MapMoveKey(EKeys::S, true, true);
	MapMoveKey(EKeys::D, false, false);
	MapMoveKey(EKeys::A, false, true);
}

void ALockdownCharacter::CreateInputAssets()
{
	if (MappingContext)
	{
		return; // Already built (e.g. re-possessed).
	}

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Lockdown"));

	MoveAction = MakeAction(TEXT("IA_Move"), true);
	LookAction = MakeAction(TEXT("IA_Look"), true);
	JumpAction = MakeAction(TEXT("IA_Jump"), false);
	FireAction = MakeAction(TEXT("IA_Fire"), false);
	SelectRifleAction = MakeAction(TEXT("IA_SelectRifle"), false);
	SelectGrenadeAction = MakeAction(TEXT("IA_SelectGrenade"), false);
	CycleWeaponAction = MakeAction(TEXT("IA_CycleWeapon"), false);
	PauseAction = MakeAction(TEXT("IA_Pause"), false);

	// The pause key must keep working while the game is paused so it can also close the menu.
	PauseAction->bTriggerWhenPaused = true;

	MapWasdKeys();
	MappingContext->MapKey(LookAction, EKeys::Mouse2D);
	MappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	MappingContext->MapKey(FireAction, EKeys::LeftMouseButton);
	MappingContext->MapKey(SelectRifleAction, EKeys::One);
	MappingContext->MapKey(SelectGrenadeAction, EKeys::Two);
	// With only two weapons, either wheel direction simply toggles between them.
	MappingContext->MapKey(CycleWeaponAction, EKeys::MouseScrollUp);
	MappingContext->MapKey(CycleWeaponAction, EKeys::MouseScrollDown);
	MappingContext->MapKey(PauseAction, EKeys::Escape);
}

void ALockdownCharacter::AddMappingContextToLocalPlayer()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	if (!LocalPlayer)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		Subsystem->AddMappingContext(MappingContext, 0);
	}
}

void ALockdownCharacter::BindInputActions(UEnhancedInputComponent* Input)
{
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALockdownCharacter::HandleMove);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALockdownCharacter::HandleLook);

	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

	// Started = first click always attempts a shot; Triggered = held button repeats for the rifle only.
	Input->BindAction(FireAction, ETriggerEvent::Started, this, &ALockdownCharacter::HandleFireStarted);
	Input->BindAction(FireAction, ETriggerEvent::Triggered, this, &ALockdownCharacter::HandleFireHeld);

	Input->BindAction(SelectRifleAction, ETriggerEvent::Started, this, &ALockdownCharacter::HandleSelectRifle);
	Input->BindAction(SelectGrenadeAction, ETriggerEvent::Started, this, &ALockdownCharacter::HandleSelectGrenade);
	Input->BindAction(CycleWeaponAction, ETriggerEvent::Started, this, &ALockdownCharacter::HandleCycleWeapon);

	Input->BindAction(PauseAction, ETriggerEvent::Started, this, &ALockdownCharacter::HandlePause);
}

void ALockdownCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	CreateInputAssets();
	AddMappingContextToLocalPlayer();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		BindInputActions(EnhancedInput);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Lockdown: input component is not an EnhancedInputComponent. "
			"Set DefaultInputComponentClass=/Script/EnhancedInput.EnhancedInputComponent in DefaultInput.ini."));
	}
}

// ---------------------------------------------------------------------------------------------
// Input handlers
// ---------------------------------------------------------------------------------------------

void ALockdownCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Axis.Y);
	AddMovementInput(GetActorRightVector(), Axis.X);
}

void ALockdownCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X * LookSensitivity);
	// Mouse Y is positive when moving up; pitch input is positive looking down, hence the negation.
	AddControllerPitchInput(-Axis.Y * LookSensitivity);
}

void ALockdownCharacter::HandleFireStarted()
{
	if (CanFire())
	{
		FireCurrentWeapon();
	}
	else if (!bIsDead && IsFireBlockedByState())
	{
		// Clicked while jammed / out of ammo: give audible feedback (not while merely on cooldown).
		PlaySound2D(DryFireSound);
	}
}

void ALockdownCharacter::HandleFireHeld()
{
	if (CurrentWeapon == EWeaponSlot::Rifle && CanFire())
	{
		FireCurrentWeapon();
	}
}

void ALockdownCharacter::HandleSelectRifle()
{
	SetWeapon(EWeaponSlot::Rifle);
}

void ALockdownCharacter::HandleSelectGrenade()
{
	SetWeapon(EWeaponSlot::GrenadeLauncher);
}

void ALockdownCharacter::HandleCycleWeapon()
{
	SetWeapon(CurrentWeapon == EWeaponSlot::Rifle ? EWeaponSlot::GrenadeLauncher : EWeaponSlot::Rifle);
}

void ALockdownCharacter::HandlePause()
{
	if (bIsDead)
	{
		return; // The end screen owns input after death.
	}
	if (ALockdownPlayerController* PC = Cast<ALockdownPlayerController>(GetController()))
	{
		PC->TogglePause();
	}
}

// ---------------------------------------------------------------------------------------------
// Weapons
// ---------------------------------------------------------------------------------------------

bool ALockdownCharacter::IsFireBlockedByState() const
{
	if (bIsDead)
	{
		return true;
	}
	if (StatusEffectComponent && StatusEffectComponent->HasEffect(EStatusEffect::WeaponJammed))
	{
		return true;
	}
	return CurrentWeapon == EWeaponSlot::GrenadeLauncher && GrenadeAmmo <= 0;
}

bool ALockdownCharacter::CanFire() const
{
	const UWorld* World = GetWorld();
	return World && !IsFireBlockedByState() && World->GetTimeSeconds() >= NextFireTime;
}

float ALockdownCharacter::GetDamageMultiplier() const
{
	const bool bOvercharged = StatusEffectComponent && StatusEffectComponent->HasEffect(EStatusEffect::DoubleDamage);
	return bOvercharged ? DoubleDamageMultiplier : 1.f;
}

void ALockdownCharacter::AddGrenadeAmmo(int32 Amount)
{
	GrenadeAmmo = FMath::Clamp(GrenadeAmmo + Amount, 0, MaxGrenadeAmmo);
	OnWeaponChanged.Broadcast();
}

void ALockdownCharacter::SetWeapon(EWeaponSlot NewWeapon)
{
	if (bIsDead || NewWeapon == CurrentWeapon)
	{
		return;
	}

	CurrentWeapon = NewWeapon;

	// Brief lock-out so switching feels weighty and cannot be used to dodge fire-rate limits.
	NextFireTime = FMath::Max(NextFireTime, GetWorld()->GetTimeSeconds() + WeaponSwitchDelay);

	HideMuzzleFlash();
	ApplyWeaponVisuals();
	PlaySound2D(WeaponSwitchSound);
	OnWeaponChanged.Broadcast();
}

void ALockdownCharacter::ApplyWeaponVisuals()
{
	const bool bRifle = CurrentWeapon == EWeaponSlot::Rifle;
	GunMesh->SetRelativeScale3D(bRifle ? RifleGunScale : LauncherGunScale);
	GunMesh->SetRelativeLocation(GunRestLocation);
	TintMesh(GunMesh, BasicShapeMaterial, bRifle ? RifleColor : LauncherColor);
}

void ALockdownCharacter::FireCurrentWeapon()
{
	if (CurrentWeapon == EWeaponSlot::Rifle)
	{
		FireRifle();
	}
	else
	{
		FireGrenade();
	}
}

void ALockdownCharacter::FireRifle()
{
	UWorld* World = GetWorld();
	NextFireTime = World->GetTimeSeconds() + RifleFireRate;

	// Hitscan from the camera so the crosshair is always accurate.
	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FVector Direction = FirstPersonCamera->GetForwardVector();
	const FVector End = Start + Direction * RifleRange;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LockdownRifleTrace), true, this);
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	if (bHit && Hit.GetActor())
	{
		UGameplayStatics::ApplyPointDamage(Hit.GetActor(), RifleDamage * GetDamageMultiplier(), Direction, Hit,
			GetController(), this, UDamageType::StaticClass());
	}

	SpawnTracer(Muzzle->GetComponentLocation(), bHit ? FVector(Hit.ImpactPoint) : End);
	PlayMuzzleFlash();
	KickGun();
	ApplyRecoil(RifleRecoilPitch);
	PlaySound2D(RifleFireSound);
}

void ALockdownCharacter::FireGrenade()
{
	UWorld* World = GetWorld();
	NextFireTime = World->GetTimeSeconds() + GrenadeCooldown;
	--GrenadeAmmo;

	const FVector SpawnLocation = FirstPersonCamera->GetComponentLocation()
		+ FirstPersonCamera->GetForwardVector() * GrenadeSpawnForwardOffset;
	FRotator SpawnRotation = FirstPersonCamera->GetComponentRotation();
	SpawnRotation.Pitch += GrenadeLaunchPitchBias;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UClass* ClassToSpawn = GrenadeClass ? GrenadeClass.Get() : AGrenadeProjectile::StaticClass();
	if (AGrenadeProjectile* Grenade = World->SpawnActor<AGrenadeProjectile>(ClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams))
	{
		Grenade->SetDamageMultiplier(GetDamageMultiplier());
	}

	PlayMuzzleFlash();
	KickGun();
	ApplyRecoil(GrenadeRecoilPitch);
	PlaySound2D(GrenadeFireSound);
	OnWeaponChanged.Broadcast(); // Ammo count changed.
}

// ---------------------------------------------------------------------------------------------
// Weapon feel
// ---------------------------------------------------------------------------------------------

void ALockdownCharacter::ApplyRecoil(float PitchDegrees)
{
	// A little random yaw keeps sustained fire from feeling like a laser.
	AddControllerPitchInput(-PitchDegrees);
	AddControllerYawInput(FMath::FRandRange(-RecoilYawJitter, RecoilYawJitter));
}

void ALockdownCharacter::PlayMuzzleFlash()
{
	MuzzleFlashLight->SetLightColor(MuzzleFlashColor);
	MuzzleFlashLight->SetIntensity(MuzzleFlashIntensity);
	MuzzleFlashLight->SetVisibility(true);
	GetWorldTimerManager().SetTimer(MuzzleFlashTimer, this, &ALockdownCharacter::HideMuzzleFlash, MuzzleFlashDuration, false);
}

void ALockdownCharacter::HideMuzzleFlash()
{
	GetWorldTimerManager().ClearTimer(MuzzleFlashTimer);
	MuzzleFlashLight->SetVisibility(false);
}

void ALockdownCharacter::KickGun()
{
	// Slide the view-model back; a short timer returns it (no Tick needed).
	GunMesh->SetRelativeLocation(GunRestLocation - FVector(GunKickDistance, 0.f, 0.f));
	GetWorldTimerManager().SetTimer(GunKickTimer, this, &ALockdownCharacter::ResetGunKick, GunKickRecoverTime, false);
}

void ALockdownCharacter::ResetGunKick()
{
	GunMesh->SetRelativeLocation(GunRestLocation);
}

void ALockdownCharacter::SpawnTracer(const FVector& Start, const FVector& End)
{
	const FVector Delta = End - Start;
	const float Length = Delta.Size();
	if (!CylinderShape || Length < KINDA_SMALL_NUMBER)
	{
		return;
	}

	// A thin stretched cylinder that self-destructs: works in shipping builds, unlike DrawDebugLine.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Midpoint = Start + Delta * 0.5f;
	const FRotator Rotation = FRotationMatrix::MakeFromZ(Delta).Rotator(); // Cylinder runs along local Z.

	AStaticMeshActor* Tracer = GetWorld()->SpawnActor<AStaticMeshActor>(Midpoint, Rotation, SpawnParams);
	if (!Tracer)
	{
		return;
	}

	UStaticMeshComponent* TracerMesh = Tracer->GetStaticMeshComponent();
	TracerMesh->SetMobility(EComponentMobility::Movable); // Must be movable before assigning a mesh at runtime.
	TracerMesh->SetStaticMesh(CylinderShape);
	TracerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TracerMesh->SetCastShadow(false);
	TracerMesh->SetWorldScale3D(FVector(TracerThickness, TracerThickness, Length / CharCylinderHeight));
	TintMesh(TracerMesh, GlowMaterial ? GlowMaterial.Get() : BasicShapeMaterial.Get(), TracerColor);

	Tracer->SetLifeSpan(TracerLifetime);
}

void ALockdownCharacter::TintMesh(UStaticMeshComponent* TargetMesh, UMaterialInterface* BaseMaterial, const FLinearColor& Color)
{
	if (!TargetMesh || !BaseMaterial)
	{
		return;
	}

	// Only swap in the base material the first time; afterwards reuse the existing dynamic instance.
	if (!Cast<UMaterialInstanceDynamic>(TargetMesh->GetMaterial(0)))
	{
		TargetMesh->SetMaterial(0, BaseMaterial);
	}
	if (UMaterialInstanceDynamic* MID = TargetMesh->CreateDynamicMaterialInstance(0))
	{
		// Set both common parameter names; whichever the material lacks is ignored.
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetVectorParameterValue(TEXT("EmissiveColor"), Color);
	}
}

void ALockdownCharacter::PlaySound2D(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

// ---------------------------------------------------------------------------------------------
// Health / status
// ---------------------------------------------------------------------------------------------

void ALockdownCharacter::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if (bIsDead || Damage <= 0.f)
	{
		return;
	}
	OnPlayerDamaged.Broadcast(Damage);
	PlaySound2D(HurtSound);
}

void ALockdownCharacter::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;

	HideMuzzleFlash();
	PlaySound2D(DeathSound);
	GetCharacterMovement()->DisableMovement();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	if (ALockdownGameMode* GameMode = GetWorld()->GetAuthGameMode<ALockdownGameMode>())
	{
		GameMode->OnPlayerDied();
	}
}

void ALockdownCharacter::HandleStatusEffectsChanged()
{
	UpdateMovementSpeed();
}

void ALockdownCharacter::UpdateMovementSpeed()
{
	const bool bSlowed = StatusEffectComponent && StatusEffectComponent->HasEffect(EStatusEffect::Slowed);
	GetCharacterMovement()->MaxWalkSpeed = bSlowed ? BaseWalkSpeed * SlowedSpeedMultiplier : BaseWalkSpeed;
}

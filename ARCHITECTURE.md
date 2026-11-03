# Lockdown (Shutdown) — C++ Architecture Contract

UE 5.6.1, module name `Lockdown`, API macro `LOCKDOWN_API`. Source lives in `Source/Lockdown/{Public,Private}/<Folder>/`.
All gameplay is C++; Blueprints only subclass to assign meshes/sounds/VFX. Everything must also work with NO Blueprint subclass
(sensible default meshes loaded via ConstructorHelpers from /Engine/BasicShapes/ and /Engine/EngineMaterials/).

## Style rules
- Unreal naming (A/U/F/E/I prefixes, bPrefix bools). One class per file. `#pragma once`, minimal includes in headers (forward declare).
- UPROPERTY(EditAnywhere/EditDefaultsOnly, Category="Lockdown|...") for every tunable. No magic numbers in .cpp.
- Short functions, comment the *why*. No Tick unless needed (use timers). Delegates for events, not polling.
- Enhanced Input. Input assets created at runtime in C++ (UInputAction/UInputMappingContext NewObject) so no .uasset is needed.

## Shared types (Core/LockdownTypes.h)
```cpp
UENUM(BlueprintType) enum class EStatusEffect : uint8 { None, Slowed, WeaponJammed, DoubleDamage };
UENUM(BlueprintType) enum class ETurretType : uint8 { Small, Medium, Large };
UENUM(BlueprintType) enum class EMatchResult : uint8 { None, Won, LostHealth, LostTime };
```

## Components
- `UHealthComponent` (Components/): MaxHealth, CurrentHealth, `ApplyDamage(float)`, `Heal(float)`, `HealToFull()`, `IsDead()`,
  `GetHealthPercent()`. Delegates: `FOnHealthChanged(float Current, float Max)`, `FOnDeath()`. Hooks owner's OnTakeAnyDamage.
- `UStatusEffectComponent` (Components/): `ApplyEffect(EStatusEffect, float Duration)` (refreshes timer if active), `HasEffect`,
  `GetRemainingTime(EStatusEffect)`, delegate `FOnStatusEffectsChanged()`. Timer per effect.

## Player
- `ALockdownCharacter` (Player/): first-person ACharacter, camera, simple gun arm mesh (cube/cylinder ok). WASD, mouse look, jump,
  LMB fire, 1/2 or mouse wheel to switch weapon, Esc pause. Health 100. Owns Health + StatusEffect components.
  Slowed → MaxWalkSpeed * 0.5. WeaponJammed → cannot fire. DoubleDamage → damage multiplier 2.
  Weapons: enum `EWeaponSlot { Rifle, GrenadeLauncher }`.
  Rifle: hitscan line trace, 25 base damage, FireRate 0.15s, tracer/muzzle flash optional.
  GrenadeLauncher: spawns `AGrenadeProjectile`, cooldown 0.8s, Ammo 6 (MaxAmmo 6).
  Exposes `GetCurrentWeapon()`, `GetGrenadeAmmo()`, `AddGrenadeAmmo(int)`, `GetHealthComponent()`, `GetStatusEffectComponent()`.
  Death → notifies GameMode.
- `ALockdownPlayerController` (Player/): sets up input mapping context, owns HUD widget creation, pause menu toggle, game-over screen.

## Projectiles (Weapons/)
- `AGrenadeProjectile`: kinematic arc via UProjectileMovementComponent (gravity scale 1, bShouldBounce). Bounces ONCE, explodes on
  second impact or after 3s fuse. Radial damage (UGameplayStatics::ApplyRadialDamage) 60 dmg, radius 300, ignores instigator.
- `ATurretProjectile`: straight, fast, 10 damage to player on hit, destroys self. Visible glowing sphere.
- `ATurretShell` (Large turret): arced explosive, radial 20 damage radius 250.

## World
- `ATurret` (World/): base pawn-less AActor. ETurretType sets defaults (Small: 50 HP, fast fire 0.4s, turn fast;
  Medium: 100 HP, 1.0s; Large: 200 HP, 2.5s fires ATurretShell). Base damage per rifle hit = 25 (so medium = 4 hits).
  Vision cone: DetectionRange 2000, HalfAngle 45°, line-of-sight trace, ReactionDelay 0.5s before first shot, forgets player
  after LoseSightTime 2s. Idle sweep rotation when no target. `bIsDisabled` → stops; `Disable()` called by terminal.
  Has UHealthComponent; on death explode + destroy. Visual: base cylinder + rotating head cube + barrel; light that turns red when alerted.
  `UPROPERTY ATerminal* ControllingTerminal` not needed — terminals hold turret refs.
- `ATerminal` (World/): has HealthComponent (100). `UPROPERTY(EditInstanceOnly) TArray<ATurret*> ControlledTurrets;`
  `UPROPERTY(EditInstanceOnly) ALevelGate* GateToOpen;` On destroyed: Disable all turrets, open gate, notify GameMode.
- `ALevelGate` (World/): door that slides open (timeline-free: interp in Tick only while opening).
- `AMovingPlatform` (World/): moves between start and `FVector TargetOffset` (MakeEditWidget), speed, pause at ends.
- `APickupBase` (World/) with overlap sphere, bob/rotate, respawn optional; subclasses `AHealthPickup` (HealToFull),
  `AOverchargePickup` (DoubleDamage 30s), `AGrenadeAmmoPickup` (+3).
- `AHazardZone` (World/) box trigger with `EStatusEffect Effect` + `Duration`; presets subclasses `AAcidPuddle` (Slowed 15s, green decal/plane),
  `ASignalJammerZone` (WeaponJammed 20s, purple translucent box).

## Game flow (Core/)
- `ALockdownGameMode`: TimeLimit 300s (timer), counts ATerminal actors at BeginPlay, `OnTerminalDestroyed()`, `OnPlayerDied()`.
  `GetTimeRemaining()`, `GetTerminalsRemaining()`, `GetTerminalsTotal()`, `GetMatchResult()`. Delegate `FOnMatchEnded(EMatchResult)`.
  On end: pause input, controller shows end screen. `RestartLevel()`, `ReturnToMainMenu()` (opens map named by `MainMenuMapName`, default "MainMenu").
- `AMainMenuGameMode`: shows main menu widget, shows mouse.

## UI (UI/) — UUserWidget subclasses that BUILD THEIR OWN WIDGET TREE IN C++ (override `NativeOnInitialized`/`RebuildWidget`
via WidgetTree->ConstructWidget) so no WBP assets are required. Clean dark sci-fi look: dark translucent panels, cyan accent, red danger.
- `UHUDWidget`: health bar, timer (MM:SS, flashes red under 30s), "Terminals X / Y", weapon name + grenade ammo, crosshair, active
  effect icons (text chips w/ remaining seconds: SLOWED / JAMMED / OVERCHARGED), damage flash vignette.
- `UPauseMenuWidget`: Resume, Restart, Main Menu.
- `UMainMenuWidget`: title "SHUTDOWN", Start Game, Instructions (toggle panel), Quit.
- `UEndScreenWidget`: "FACILITY SHUT DOWN" / "YOU DIED" / "SELF-DESTRUCT", Restart, Main Menu.
- `ULockdownButton` helper optional.

## Build / dependencies
Build.cs public deps: Core, CoreUObject, Engine, InputCore, EnhancedInput, UMG, Slate, SlateCore. UE 5.6 APIs only
(e.g. use `TObjectPtr<>` for UPROPERTY pointers, `GetWorldTimerManager()`, `UEnhancedInputLocalPlayerSubsystem`).
We CANNOT compile in the sandbox: be conservative, double-check every include path and API signature you use.
Each agent owns only its listed files; if you need something from another agent's class, use only the API in this doc.

## Levels
Single map `/Game/Maps/Facility` with 3 sections ("levels"), each sealed by an ALevelGate opened by that section's terminal(s).
Main menu map `/Game/Maps/MainMenu`. Both built by an editor Python script `Scripts/build_levels.py` (Editor Scripting).
Lighting: dark industrial, point lights, emissive accents. World Outliner folders: Geometry, Turrets, Terminals, Pickups, Hazards, Platforms, Gates, Lighting.

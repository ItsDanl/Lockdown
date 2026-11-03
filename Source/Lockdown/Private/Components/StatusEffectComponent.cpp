#include "Components/StatusEffectComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UStatusEffectComponent::UStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStatusEffectComponent::ApplyEffect(EStatusEffect Effect, float Duration)
{
	UWorld* World = GetWorld();
	if (!World || Effect == EStatusEffect::None || Duration <= 0.f)
	{
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();

	// Re-using the same handle makes SetTimer replace the old timer, i.e. "refresh".
	FTimerHandle& Handle = EffectTimers.FindOrAdd(Effect);
	TimerManager.SetTimer(Handle,
		FTimerDelegate::CreateUObject(this, &UStatusEffectComponent::HandleEffectExpired, Effect),
		Duration, false);

	OnStatusEffectsChanged.Broadcast();
}

bool UStatusEffectComponent::HasEffect(EStatusEffect Effect) const
{
	const UWorld* World = GetWorld();
	const FTimerHandle* Handle = EffectTimers.Find(Effect);
	return World && Handle && World->GetTimerManager().IsTimerActive(*Handle);
}

float UStatusEffectComponent::GetRemainingTime(EStatusEffect Effect) const
{
	const UWorld* World = GetWorld();
	const FTimerHandle* Handle = EffectTimers.Find(Effect);
	if (!World || !Handle || !World->GetTimerManager().IsTimerActive(*Handle))
	{
		return 0.f;
	}
	return FMath::Max(0.f, World->GetTimerManager().GetTimerRemaining(*Handle));
}

void UStatusEffectComponent::HandleEffectExpired(EStatusEffect Effect)
{
	EffectTimers.Remove(Effect);
	OnStatusEffectsChanged.Broadcast();
}

void UStatusEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		for (TPair<EStatusEffect, FTimerHandle>& Pair : EffectTimers)
		{
			World->GetTimerManager().ClearTimer(Pair.Value);
		}
	}
	EffectTimers.Empty();

	Super::EndPlay(EndPlayReason);
}

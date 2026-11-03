#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "Core/LockdownTypes.h"
#include "StatusEffectComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatusEffectsChanged);

UCLASS(ClassGroup = (Lockdown), meta = (BlueprintSpawnableComponent)) // one timer per effect
class LOCKDOWN_API UStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatusEffectComponent();

	UFUNCTION(BlueprintCallable, Category = "Lockdown|Status")
	void ApplyEffect(EStatusEffect Effect, float Duration);

	UFUNCTION(BlueprintPure, Category = "Lockdown|Status")
	bool HasEffect(EStatusEffect Effect) const;

	UFUNCTION(BlueprintPure, Category = "Lockdown|Status")
	float GetRemainingTime(EStatusEffect Effect) const;

	UPROPERTY(BlueprintAssignable, Category = "Lockdown|Status")
	FOnStatusEffectsChanged OnStatusEffectsChanged;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleEffectExpired(EStatusEffect Effect);

	TMap<EStatusEffect, FTimerHandle> EffectTimers;
};

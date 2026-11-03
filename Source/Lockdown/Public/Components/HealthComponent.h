#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class UDamageType;
class AController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, Current, float, Max);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

/** Simple health pool. Hooks the owner's OnTakeAnyDamage so ApplyDamage/ApplyPointDamage/ApplyRadialDamage all work. */
UCLASS(ClassGroup = (Lockdown), meta = (BlueprintSpawnableComponent))
class LOCKDOWN_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	/** Subtracts health (ignored when dead or Amount <= 0). Fires OnHealthChanged and, at zero, OnDeath once. */
	UFUNCTION(BlueprintCallable, Category = "Lockdown|Health")
	void ApplyDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Lockdown|Health")
	void Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Lockdown|Health")
	void HealToFull();

	UFUNCTION(BlueprintPure, Category = "Lockdown|Health")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "Lockdown|Health")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Lockdown|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Lockdown|Health")
	float GetMaxHealth() const { return MaxHealth; }

	/** Change the maximum (used by owners that set per-type health) and refill. */
	void SetMaxHealth(float NewMax, bool bRefill = true);

	UPROPERTY(BlueprintAssignable, Category = "Lockdown|Health")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Lockdown|Health")
	FOnDeath OnDeath;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lockdown|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Lockdown|Health")
	float CurrentHealth = 100.f;

private:
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	bool bIsDead = false;
};

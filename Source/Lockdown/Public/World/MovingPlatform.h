#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MovingPlatform.generated.h"

class UStaticMeshComponent;

/**
 * Kinematic platform that ping-pongs between its start and start + TargetOffset with easing and a pause at each end.
 * Moved with plain SetActorLocation in the pre-physics tick group so characters standing on it ride along
 * (CharacterMovement follows its movement base).
 */
UCLASS()
class LOCKDOWN_API AMovingPlatform : public AActor
{
	GENERATED_BODY()

public:
	AMovingPlatform();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Lockdown|Components")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	/** Destination relative to the placed location (actor space). Drag the widget in the viewport. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Platform", meta = (MakeEditWidget = "true"))
	FVector TargetOffset = FVector(0.f, 0.f, 400.f);

	/** Average travel speed in cm/s. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Platform", meta = (ClampMin = "1"))
	float Speed = 200.f;

	/** Seconds to wait at each end. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Platform", meta = (ClampMin = "0"))
	float PauseTime = 1.5f;

	/** Easing exponent (1 = constant speed, 2+ = slow in/out at the ends). */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Platform", meta = (ClampMin = "1"))
	float EaseExponent = 2.f;

	/** If false, the platform waits until StartMoving() is called. */
	UPROPERTY(EditAnywhere, Category = "Lockdown|Platform")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, Category = "Lockdown|Platform")
	FLinearColor PlatformColor = FLinearColor(0.15f, 0.17f, 0.2f);

public:
	UFUNCTION(BlueprintCallable, Category = "Lockdown|Platform")
	void StartMoving() { bIsMoving = true; }

	UFUNCTION(BlueprintCallable, Category = "Lockdown|Platform")
	void StopMoving() { bIsMoving = false; }

private:
	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;
	/** 0 = at start, 1 = at end (linear progress; eased when converting to a position). */
	float Progress = 0.f;
	/** +1 heading to end, -1 heading back. */
	float Direction = 1.f;
	float PauseRemaining = 0.f;
	bool bIsMoving = true;
};

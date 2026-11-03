#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

/**
 * Small shared helpers for the World/ classes (placeholder meshes + colour tinting).
 * Header-only on purpose so every actor can build itself from engine primitives with no content assets.
 */
namespace LockdownWorld
{
	/** Name of the colour parameter exposed by /Engine/BasicShapes/BasicShapeMaterial. */
	static const FName ColorParamName(TEXT("Color"));

	/** CONSTRUCTOR ONLY. Assigns /Engine/BasicShapes/<ShapeName> (Cube, Cylinder, Sphere, Plane) to the component. */
	inline void SetBasicMesh(UStaticMeshComponent* Comp, const TCHAR* ShapeName)
	{
		if (!Comp)
		{
			return;
		}
		const FString Path = FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), ShapeName, ShapeName);
		ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(*Path);
		if (Finder.Succeeded())
		{
			Comp->SetStaticMesh(Finder.Object);
		}
	}

	/** CONSTRUCTOR ONLY. Forces the BasicShapeMaterial (which has the "Color" param) on slot 0. */
	inline void SetBasicMaterial(UStaticMeshComponent* Comp)
	{
		if (!Comp)
		{
			return;
		}
		ConstructorHelpers::FObjectFinder<UMaterialInterface> Finder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (Finder.Succeeded())
		{
			Comp->SetMaterial(0, Finder.Object);
		}
	}

	/** RUNTIME. Creates a dynamic material on slot 0 and sets its "Color" parameter. Returns null-safe result. */
	inline UMaterialInstanceDynamic* ApplyTint(UStaticMeshComponent* Comp, const FLinearColor& Color)
	{
		if (!Comp)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* MID = Comp->CreateAndSetMaterialInstanceDynamic(0);
		if (MID)
		{
			MID->SetVectorParameterValue(ColorParamName, Color);
		}
		return MID;
	}
}

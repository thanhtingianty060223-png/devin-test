// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AnimatedDecal.generated.h"

UCLASS(Blueprintable)
class READYORNOT_API AAnimatedDecal : public AActor
{
	GENERATED_BODY()

public:
	AAnimatedDecal();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blood and Gore")
	UDecalComponent* Decal;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float AnimationTimescale = 0.25f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FRuntimeFloatCurve AnimationCurve;

	UFUNCTION(BlueprintImplementableEvent)
	void SetAnimatedDecalMaterial(UMaterialInterface* Material);
	
	UFUNCTION(BlueprintPure)
	static float GetRuntimeFloatCurveValue(const FRuntimeFloatCurve& Curve, float Time);
};

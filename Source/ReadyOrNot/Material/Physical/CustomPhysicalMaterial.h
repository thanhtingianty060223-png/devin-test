// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"

#include "CustomPhysicalMaterial.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = Physics)
class READYORNOT_API UCustomPhysicalMaterial : public UPhysicalMaterial
{
public:

	GENERATED_BODY()
	
	UCustomPhysicalMaterial();

	/* Depth at which the material fully occludes sound. (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion")
	float FullOcclusionDepth = 100.0f;
	
};

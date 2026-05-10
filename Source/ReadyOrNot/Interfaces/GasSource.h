// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GasSource.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UGasSource : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class READYORNOT_API IGasSource
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interface|Is Releasing Gas")
	bool IsReleasingGas() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interface|Get Gas Radius")
	float GetGasRadius() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interface|Get Gas Radius")
	int32 GetMaximumGasPoints() const;

	/** Allow the gas source to adjust its source location if it's off the navmesh or something*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interface|Get Gas Radius")
	bool GetGasReleaseLocation(FVector& OutLocation) const;
};

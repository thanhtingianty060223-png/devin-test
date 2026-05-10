// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "ForceLowReadyVolume.generated.h"

UCLASS(HideCategories=("Collision", "Navigation", "Tags", "Cooking"))
class READYORNOT_API AForceLowReadyVolume : public AVolume
{
	GENERATED_BODY()

public:
	AForceLowReadyVolume();
	
	UPROPERTY(EditAnywhere, Category="Force Low Ready Volume")
	bool bForceLowReadyWhileAimingAt = true;

	UPROPERTY(EditAnywhere, Category="Force Low Ready Volume")
	bool bForceLowReadyWhileInside = true;

private:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* BillboardComponent;
#endif
};

// © Void Interactive, 2017

#pragma once

#include "Referendum.h"
#include "MapReferendum.generated.h"

// Map Referendums target specific maps (for e.g, next map, current map, ..)
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API AMapReferendum : public AReferendum
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString MapURL;
};
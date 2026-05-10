// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ReadyOrNotAISenseConfig_Sight.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UReadyOrNotAISenseConfig_Sight : public UAISenseConfig_Sight
{
	GENERATED_BODY()

	public:
	virtual TSubclassOf<UAISense> GetSenseImplementation() const override;
	
};

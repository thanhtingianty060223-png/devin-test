// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PersonalizationWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UPersonalizationWidget : public UUserWidget
{
	GENERATED_BODY()


	public:
	UPROPERTY(BlueprintReadOnly)
	class ACharacterCustomizationPortal* SpawnedFromPortal;
	
};

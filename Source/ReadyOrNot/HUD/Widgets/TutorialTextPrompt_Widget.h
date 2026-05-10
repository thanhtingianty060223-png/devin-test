// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"

#include "Enums.h"
#include "Blueprint/UserWidget.h"
#include "TutorialTextPrompt_Widget.generated.h"

/**
 * base class for Blueprint class that provides instructional text for tutorial levels. 
 *  Thomas Louden 17/06/2021
 */
UCLASS()
class READYORNOT_API UTutorialTextPrompt_Widget : public UUserWidget
{
	GENERATED_BODY()




	
private:

FText FormatPromptText(const FKey& InKey, const EInputEvent& InInputEvent, const FText& InActionText, const FString& InColorLabel = "Red");

	
};

USTRUCT(BlueprintType)
struct FPromptInfo
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TutorialText")
	ETutorialMessageContext PromptContext = ETutorialMessageContext::Movement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TutorialText")
	FKey ActionKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TutorialText")
	TEnumAsByte<EInputEvent> InputType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TutorialText")
	FText ActionText;
	
};









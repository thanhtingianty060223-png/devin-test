// Void Interactive, 2020

#pragma once

#include "Actors/Environment/ReadyOrNotTriggerVolume.h"
#include "TutorialMessageVolume.generated.h"

USTRUCT(BlueprintType)
struct FTutorialActionPromptSlot
{
	GENERATED_BODY()

	FTutorialActionPromptSlot()
	{
		
		bUseCustomActionText = false;
	}

	void Init(const FName& InInputActionName, const TEnumAsByte<EInputEvent>& InInputEvent, const FText& InActionText)
	{
		InputActionName = InInputActionName;
		InputEvent = InInputEvent;
		ActionText = InActionText;
	
		

		
		bUseCustomActionText = false;
	}

	bool IsValid() const { return (!ActionText.EqualToCaseIgnored(FText::FromString("None")) || !InputActionName.IsNone() || !CustomActionPromptText.EqualToCaseIgnored(FText::FromString("None"))); }
	
	bool IsCustomTextValid() const { return bUseCustomActionText && !CustomActionPromptText.EqualToCaseIgnored(FText::FromString("None")); }
	
	// Refer to Project Settings -> Input to get the exact input action name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bUseCustomActionText"))
    FName InputActionName = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bUseCustomActionText"))
    TEnumAsByte<EInputEvent> InputEvent = IE_Pressed;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bUseCustomActionText"))
    FText ActionText = FText::FromString("None");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bUseCustomActionText"))
	bool bIsAxisEvent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bUseCustomActionText"))
	int32 KeyIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 bUseCustomActionText : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUseCustomActionText"))
	FText CustomActionPromptText = FText::FromString("None");
	
	
};


UCLASS()
class READYORNOT_API ATutorialMessageVolume : public AReadyOrNotTriggerVolume
{
	GENERATED_BODY()
	
public:	
	ATutorialMessageVolume();

	//Functions

	UFUNCTION()
	void TutorialBoxBeginOverlap(AActor* ThisActor, AActor* OtherActor);

	UFUNCTION()
	void TutorialBoxEndOverlap(AActor* ThisActor, AActor* OtherActor);

	UFUNCTION()
	void GenerateMessageContent();

	//Variables

	UPROPERTY(BlueprintReadWrite, Category= "MessageDetails")
	FString MessageMapID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category= "MessageDetails")
	bool bIsBigPopUp;

	UPROPERTY(BlueprintReadWrite, Category= "MessageDetails")
	bool bHasDisplayedMessage;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category= "MessageDetails")
	FText MessageTitle;

	UPROPERTY(BlueprintReadOnly, Category= "MessageDetails")
	TArray<FText> MessageContent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category= "MessageDetails")
	TArray<FTutorialActionPromptSlot> MessageActions;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	FText FormatPromptText(const FKey& InKey, const EInputEvent& InInputEvent, const FText& InActionText);
};

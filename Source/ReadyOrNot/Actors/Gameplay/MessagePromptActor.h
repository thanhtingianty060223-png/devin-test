// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MessagePromptActor.generated.h"

USTRUCT(BlueprintType)
struct FMessagePromptContent
{
	GENERATED_BODY()

	FMessagePromptContent()
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
class READYORNOT_API AMessagePromptActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMessagePromptActor();


	UFUNCTION()
	void GenerateMessageContent();

	UFUNCTION(BlueprintCallable, Category="Display")
	void ShowMessageThroughPopUp();

	UFUNCTION(BlueprintCallable, Category="Display")
	void HideMessagePopUp();

	
	

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
	TArray<FMessagePromptContent> MessageActions;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	FText FormatPromptText(const FKey& InKey, const EInputEvent& InInputEvent, const FText& InActionText);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};

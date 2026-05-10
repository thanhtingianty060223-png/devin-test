// Void Interactive, 2020


#include "MessagePromptActor.h"


#include "HUD/Widgets/HumanCharacterHUD_V2.h"
#include "lib/ReadyOrNotFunctionLibrary.h"

// Sets default values
AMessagePromptActor::AMessagePromptActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	MessageMapID = GetName();

}

void AMessagePromptActor::GenerateMessageContent()
{

	MessageContent.Empty();

	for (int32 i = 0; i <= MessageActions.Num()-1; i++)
	{
		FText TextToAdd;

		if (MessageActions[i].bUseCustomActionText)
		{
			TextToAdd = MessageActions[i].CustomActionPromptText;
		}
		else
		{
			if (MessageActions[i].bIsAxisEvent)
			{
				TextToAdd = FormatPromptText(UReadyOrNotFunctionLibrary::GetKeyFromInputAxisName(MessageActions[i].InputActionName, false, MessageActions[i].KeyIndex), MessageActions[i].InputEvent, MessageActions[i].ActionText);
			}
			else
			{
				TextToAdd = FormatPromptText(UReadyOrNotFunctionLibrary::GetKeyFromInputActionName(MessageActions[i].InputActionName, false, MessageActions[i].KeyIndex), MessageActions[i].InputEvent, MessageActions[i].ActionText);
			}
		}

		MessageContent.Insert(TextToAdd, i);
	}
}

void AMessagePromptActor::ShowMessageThroughPopUp()
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

	if(Player)
	{
		if(Player->HumanCharacterWidget_V2)
		{
			GenerateMessageContent();
			if(bIsBigPopUp)
			{
				if(!bHasDisplayedMessage)
				{
					Player->HumanCharacterWidget_V2->ShowTutorialOverview(MessageMapID, MessageTitle, MessageContent);
				}
				
			}
			else
			{
				Player->HumanCharacterWidget_V2->ShowTutorialPrompt(MessageMapID,!bHasDisplayedMessage, MessageTitle, MessageContent);
			}
			
			bHasDisplayedMessage = true;
		}
	}
	
}

void AMessagePromptActor::HideMessagePopUp()
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

	if(Player)
	{
		if(Player->HumanCharacterWidget_V2)
		{
			Player->HumanCharacterWidget_V2->HideTutorialPrompt(MessageMapID);
		}
	}
	
}


// Called when the game starts or when spawned
void AMessagePromptActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMessagePromptActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}



FText AMessagePromptActor::FormatPromptText(const FKey& InKey, const EInputEvent& InInputEvent, const FText& InActionText)
{	
	FString InputType = "Press";
	
	switch (InInputEvent)
	{
	case IE_Pressed:
		InputType = "Press";
		break;

	case IE_Released:
		InputType = "Release";
		break;

	case IE_Repeat:
		InputType = "Hold";
		break;

	case IE_DoubleClick:
		InputType = "Double Tap";
		break;

	case IE_Axis:
		InputType = "Move";
		break;

	case IE_MAX:
		InputType = "ruh roh";
		break;

	default:
		break;
	}

	// Format: {InputType} <Red>{Key}</> to <Red>{Action}</>
	// Example Output: Press C to Crouch
	return FText::FromString(InputType + " " + (InKey.IsValid() ? InKey.GetDisplayName().ToString() : "Unbound") + " to " +  InActionText.ToString());
}
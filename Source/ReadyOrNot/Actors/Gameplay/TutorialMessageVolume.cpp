// Void Interactive, 2020

#include "TutorialMessageVolume.h"

#include "HUD/Widgets/HumanCharacterHUD_V2.h"
#include "lib/ReadyOrNotFunctionLibrary.h"

ATutorialMessageVolume::ATutorialMessageVolume()
{
	PrimaryActorTick.bCanEverTick = true;
	MessageMapID = GetName();
	
}

FText ATutorialMessageVolume::FormatPromptText(const FKey& InKey, const EInputEvent& InInputEvent, const FText& InActionText)
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

void ATutorialMessageVolume::TutorialBoxBeginOverlap(AActor* ThisActor, AActor* OtherActor)
{
	APlayerCharacter* TargetPlayer = Cast<APlayerCharacter>(OtherActor);
	if (TargetPlayer)
	{
		if (TargetPlayer->HumanCharacterWidget_V2)
		{
			GenerateMessageContent();
			if(bIsBigPopUp)
			{
				if(!bHasDisplayedMessage)
				{
					TargetPlayer->HumanCharacterWidget_V2->ShowTutorialOverview(MessageMapID, MessageTitle, MessageContent);
				}
					
			}
			else
			{
                TargetPlayer->HumanCharacterWidget_V2->ShowTutorialPrompt(MessageMapID,!bHasDisplayedMessage, MessageTitle, MessageContent);
			}
			bHasDisplayedMessage = true;
		}
	}
}

void ATutorialMessageVolume::TutorialBoxEndOverlap(AActor* ThisActor, AActor* OtherActor)
{
	APlayerCharacter* TargetPlayer = Cast<APlayerCharacter>(OtherActor);
	if (TargetPlayer)
	{
		if (TargetPlayer->HumanCharacterWidget_V2)
		{
			TargetPlayer->HumanCharacterWidget_V2->HideTutorialPrompt(MessageMapID);
		}
	}
}

void ATutorialMessageVolume::GenerateMessageContent()
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

/*
void ATutorialMessageVolume::ShowPopUpMessage(const FText& MessageTitle, const FText& MessageContent)
{
}
*/

void ATutorialMessageVolume::BeginPlay()
{
	Super::BeginPlay();
	
	OnActorBeginOverlap.AddDynamic(this, &ATutorialMessageVolume::TutorialBoxBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &ATutorialMessageVolume::TutorialBoxEndOverlap);
}

void ATutorialMessageVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


// Copyright Void Interactive, 2023
#include "HUD/Widgets/Console/CommandWheel.h"

#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Info/SWATManager.h"
#include "lib/ReadyOrNotCommandFunctionLibrary.h"
#include "Enums.h"
#include "Components/Overlay.h"
#include "Components/RichTextBlock.h"

void UCommandWheel::NativeConstruct()
{
	Super::NativeConstruct();
	
	CommandLibrary = NewObject<UReadyOrNotCommandFunctionLibrary>();
	CommandLibrary->IsGamePad = true;
	CommandLibrary->Flashbang = Flashbang;
	CommandLibrary->Stinger = Stinger;
	CommandLibrary->CSGas = CSGas;
	CommandLibrary->World = GetWorld();
	CommandLibrary->SwatManager = USWATManager::Get(this);

	SetRenderOpacity(0.0f);
	
	for(const auto it : CommandWheelIcons->GetRowMap())
	{
		const FCommandWheelIconMapping* mapping = reinterpret_cast<FCommandWheelIconMapping*>(it.Value);
		CommandIconMap.Add(mapping->SwatCommand, mapping->Icon);
	}

	if (ThumbstickImage)
	{
		ThumbstickImage->SetBrush(UReadyOrNotFunctionLibrary::GetIconFromInputKeyName("Gamepad_RightThumbstick"));
		ThumbstickImage->SetVisibility(ESlateVisibility::Hidden);
	}

	if (OuterWheel)
		OuterWheel->SetVisibility(ESlateVisibility::Hidden);
}

void UCommandWheel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red, FString::Printf(TEXT("History: %d"),  CommandLibrary->CommandHistory.Num()));
	}
#endif
	
	if (!CommandWheelActive)
	{
		Disable();
		return;
	}
	
	if(bIsInFreeView)
	{
		return;
	}
	
	CommandLibrary->UpdateCommandPageData(GetOwningPlayerPawn());

	const TArray<FSwatCommand> CurrentCommandOptions = CommandLibrary->GetCurrentCommandOptions();

	
	if((CurrentWheelColor == WhiteColor) != CommandLibrary->IsIndividualCommands())
	{
		UpdateWheelColor();
		SetHeaderText(FText::GetEmpty());
	}
	
	ThumbstickImage->SetVisibility(ESlateVisibility::Hidden);

	if(CurrentCommandOptions != PreviousCommandOptions)
	{
		CurrentIndex = -1;
		SetInnerSegments(0);
		SetInnerCommands(CurrentCommandOptions);
	}
	PreviousCommandOptions =CurrentCommandOptions;

	const AReadyOrNotPlayerController* ReadyOrNotPlayerController = Cast<AReadyOrNotPlayerController>(
		GetOwningPlayerPawn()->GetController());
	float x = 0.0f, y = 0.0f;
	ReadyOrNotPlayerController->GetInputAnalogStickState(EControllerAnalogStick::CAS_RightStick, x, y);
	InputCommandVector(FVector(x, y, 0));

}

void UCommandWheel::Enable()
{
	bIsInFreeView = false;
	CommandLibrary->ShouldQueue = false;
	CurrentIndex = -1;
	WheelInputHistory.Empty();

	PreviousCommandOptions.Empty();
	
	FreeViewInstructionsCanvasPanel->SetVisibility(ESlateVisibility::Hidden);
	
	ThumbstickImage->SetVisibility(ESlateVisibility::Visible);

	CommandLibrary->Reset();
	CommandLibrary->UpdateCommandPageData(GetOwningPlayerPawn());

	if(CommandLibrary->CommandHistory.Num() == 0 || CommandLibrary->CommandHistory.Last().Num() == 0)
	{
		Disable();
		return;
	}

	// if(APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	// {
	// 	LocalPlayerCharacter->StopFreeLook();
	// }

	ResetWheel();
	SetInnerWheelDirection(FVector::ZeroVector);
	SetVisibility(ESlateVisibility::Visible);
	InnerWheel->SetVisibility(ESlateVisibility::Visible);
	InnerWheel->SetRenderOpacity(1);
	OuterWheel->SetVisibility(ESlateVisibility::Hidden);
	SetRenderOpacity(1.0f);
	UpdateWheelColor();
	CommandWheelActive = true;
}

void UCommandWheel::Disable()
{
	// if(APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	// {
	// 	LocalPlayerCharacter->StopFreeLook();
	// }
	bIsInFreeView = false;
	CommandWheelActive = false;
	CommandLibrary->Reset();
	SetVisibility(ESlateVisibility::Hidden);
	InnerWheel->SetVisibility(ESlateVisibility::Hidden);
	OuterWheel->SetVisibility(ESlateVisibility::Hidden);
	FreeViewInstructionsCanvasPanel->SetVisibility(ESlateVisibility::Hidden);
	CurrentIndex = -1;
	SetRenderOpacity(0.0f);
}

void UCommandWheel::InputCommandVector(const FVector& InputVector)
{
	if (!CommandWheelActive || bIsInFreeView)
		return;
	
	const TArray<FSwatCommand> CommandOptions = CommandLibrary->GetCurrentCommandOptions();

	if(CommandOptions.Num() == 0)
		return;
	
	if(CurrentIndex > CommandOptions.Num()-1)
	{
		CurrentIndex = -1;
	}
	
	const int PreviousPreSelectionIndex = CurrentIndex;
	
	if(InputVector.Size() < 0.1f)
	{
		CurrentIndex = -1;
	}
	else
	{
		SetInnerWheelDirection(InputVector.GetSafeNormal());
	}

	if (CurrentIndex > -1)
	{
		if(!CommandOptions[CurrentIndex].bEnabled)
		{
			SetInnerWheelDirection(FVector::ZeroVector);
			CurrentIndex = -1;
		}
	}
	
	else if (PreviousPreSelectionIndex >= 0)
	{
		
		
		if (CommandOptions[PreviousPreSelectionIndex].SubCommands.Num() != 0)
		{
			OuterWheel->SetVisibility(ESlateVisibility::Visible);
			UpdateOuterWheel(PreviousInputVector, CommandOptions, PreviousPreSelectionIndex);
			WheelInputHistory.Add(MakeTuple(PreviousInputVector, PreviousPreSelectionIndex));
			CommandLibrary->SelectCommand(PreviousPreSelectionIndex);
			SetInnerWheelDirection(FVector::ZeroVector);
		}
		else
		{
			// If no subcommands, keep it selected
			SetInnerWheelDirection(PreviousInputVector.GetSafeNormal());
			return;
		}
	}
	
	if(PreviousPreSelectionIndex != CurrentIndex)
	{
		if(CurrentIndex == -1 || !CommandOptions[CurrentIndex].bEnabled)
		{
			SetHeaderText(FText::GetEmpty());
		}
		else
		{
			SetHeaderText(CommandOptions[CurrentIndex].CommandText);
		}
	}

	PreviousInputVector = InputVector;
}

void UCommandWheel::ConfirmCommand()
{
	ConfirmCommand(CurrentIndex);
}

void UCommandWheel::ConfirmCommand(int CommandIndex)
{
	if(CommandIndex < 0 || CommandLibrary->CommandHistory.Last()[CommandIndex].SubCommands.Num() > 0)
	{
		Disable();
		return;
	}

	const FSwatCommand Command = CommandLibrary->GetCurrentCommandOptions()[CommandIndex];

	if(FreeViewCommands.Contains(Command.CommandType))
	{
		SelectedFreeViewCommand = Command;
		bIsInFreeView = true;
		ResetWheel();
		InnerWheel->SetVisibility(ESlateVisibility::Hidden);
		OuterWheel->SetVisibility(ESlateVisibility::Hidden);
		ThumbstickImage->SetVisibility(ESlateVisibility::Hidden);
		FreeViewInstructionsCanvasPanel->SetVisibility(ESlateVisibility::Visible);
		// if(APlayerCharacter* LocalPlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
		// {
		// 	LocalPlayerCharacter->StartFreeLook();
		// }
		return;
	}
	CommandLibrary->SelectCommand(CommandIndex);
	Disable(); 
}

void UCommandWheel::FreeViewConfirm()
{
	CommandLibrary->SelectCommand(SelectedFreeViewCommand);
	Disable(); 
}

void UCommandWheel::Cancel()
{
	CommandLibrary->CancelCommand();

	CurrentIndex = -1;
	PreviousInputVector = FVector::ZeroVector;
	SetInnerWheelDirection(FVector::ZeroVector);
	SetHeaderText(FText::GetEmpty());
	
	if(CommandLibrary->CommandHistory.Num() < 2)
	{
		OuterWheel->SetVisibility(ESlateVisibility::Hidden);
	}
	else {
		WheelInputHistory.Pop(); // Remove the last which is the current outer wheel input
		const TTuple<FVector, int> WheelInput = WheelInputHistory.Last();
		const TArray<FSwatCommand> CommandOptions =  CommandLibrary->CommandHistory[CommandLibrary->CommandHistory.Num()-2];
		UpdateOuterWheel( WheelInput.Get<0>(), CommandOptions,  WheelInput.Get<1>());
	}
}

void UCommandWheel::ToggleQueueing() const
{
	CommandLibrary->ShouldQueue = !CommandLibrary->ShouldQueue;
	UpdateQueueText();
}

void UCommandWheel::UpdateQueueText() const
{
	if(CommandLibrary->IsIndividualCommands() || CommandLibrary->HasQueuedCommandForTeam(CommandLibrary->ActiveTeamType))
	{
		QueueTextContainer->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	QueueTextContainer->SetVisibility(ESlateVisibility::Visible);

	const FKey QueueKey = EKeys::Gamepad_DPad_Down;
	constexpr EInputEvent InInputEvent = IE_Pressed;
	const FText ActionText = FText::FromStringTable("SwatCommandTable", CommandLibrary->ShouldQueue ? "CommandWheelPressToStopQueueing" : "CommandWheelPressToQueue");
	QueueText->SetText(UReadyOrNotFunctionLibrary::FormatPlayerActionText(QueueKey, InInputEvent, ActionText, "White"));
}

void UCommandWheel::CycleSwatElement(bool Next)
{
	if(CommandLibrary->IsIndividualCommands())
	{
		return;
	}
	
	const ETeamType PreviousTeam = CommandLibrary->ActiveTeamType;
	CommandLibrary->CycleSwatElement(Next);
	if(PreviousTeam != CommandLibrary->ActiveTeamType)
	{
		Cancel();
		CommandLibrary->UpdateCommandPageData(GetOwningPlayerPawn());
		OuterWheel->SetVisibility(ESlateVisibility::Hidden);
	}
	
	UpdateWheelColor();
}

FSlateBrush& UCommandWheel::GetIconForCommand(FSwatCommand Command)
{
	FSlateBrush& iconBrush = CommandIconMap[ESwatCommand::SC_KillMe];
	
	if (CommandIconMap.Contains(Command.CommandType) && IsValid(CommandIconMap[Command.CommandType].GetResourceObject()))
	{
		return  CommandIconMap[Command.CommandType];
	}
	
	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red,FString::Printf(TEXT("No icon matching command: %s (%s)"), *Command.CommandText.ToString(), *RON_ENUM_TO_STRING(ESwatCommand, Command.CommandType)));
	return iconBrush;
}


void UCommandWheel::ResetWheel()
{
	SetHeaderText(FText::GetEmpty());
	UpdateWheelColor();
}

void UCommandWheel::SetHeaderText(FText text) const
{
	if(text.IsEmpty() && CommandLibrary->IsIndividualCommands())
	{
		text = FText::FromString(CommandLibrary->CurrentOfficersName());
	}

	HeaderText->SetText(text);
	HeaderText->SetOpacity(1.00f);
	
	
	if(text.IsEmpty())
	{
		HeaderOverlay->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	HeaderOverlay->SetVisibility(ESlateVisibility::Visible);
}

void UCommandWheel::UpdateWheelColor()
{
	switch(CommandLibrary->ActiveTeamType)
	{
		case ETeamType::TT_SERT_RED:
			CurrentWheelColor = RedColor;
			break;
		case ETeamType::TT_SERT_BLUE:
			CurrentWheelColor = BlueColor;
			break;
		default:
			CurrentWheelColor = GoldColor;
	}

	if(CommandLibrary->IsIndividualCommands())
	{
		CurrentWheelColor = WhiteColor;
	}
	
	HeaderText->SetColorAndOpacity(CurrentWheelColor);
	InnerWheel->SetColorAndOpacity(CurrentWheelColor);
	OuterWheel->SetColorAndOpacity(CurrentWheelColor);
	OuterWheel->SetRenderOpacity(0.3f);
	ThumbstickImage->SetColorAndOpacity(CurrentWheelColor);
	
	UpdateQueueText();
}
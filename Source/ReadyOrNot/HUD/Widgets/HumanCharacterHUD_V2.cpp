// Void Interactive, 2020

#include "HumanCharacterHUD_V2.h"

#include "Characters/PlayerCharacter.h"
#include "Characters/ReadyOrNotPlayerController.h"

#include "Components/RetainerBox.h"

#include "HUD/PlayerHUD.h"

#include "HUD/Widgets/PlayerActionPromptWidget.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

UHumanCharacterHUD_V2::UHumanCharacterHUD_V2()
{
	MaxHUDSwayMovement = {6.0f, 1.5f};
	SwayStrength = {8.0f, 8.0f};
	SwaySpeed = {5.0f, 5.0f};
	
	MouseAxisDeltaThreshold = {10.0f, 10.0f};

	bEnableHUDSway = true;
	bEnableCurvedHUD = true;
}

void UHumanCharacterHUD_V2::ShowHUD()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ActivateWidget();
	PlayAnimation(Anim_FadeInHUD);
	// CanvasPanel_Main->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UHumanCharacterHUD_V2::HideHUD()
{
	SetVisibility(ESlateVisibility::Collapsed);
	DeactivateWidget();
	StopAnimation(Anim_FadeInHUD);
	// CanvasPanel_Main->SetVisibility(ESlateVisibility::Collapsed);
}

UItemWheel* UHumanCharacterHUD_V2::GetItemWheel()
{
	return ItemWheel;
}

UCommandWheel* UHumanCharacterHUD_V2::GetCommandWheel()
{
	return CommandWheel;
}

void UHumanCharacterHUD_V2::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	PlayerController = Cast<AReadyOrNotPlayerController>(GetOwningPlayer());
	PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	
	if (PlayerCharacter)
		PlayerCharacter->SetHumanCharacterWidget_V2(this);
}

void UHumanCharacterHUD_V2::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsDesignTime())
	{
		
	}
	else
	{
		PlayerController = Cast<AReadyOrNotPlayerController>(GetOwningPlayer());
		PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
		
		if (PlayerCharacter)
			PlayerCharacter->SetHumanCharacterWidget_V2(this);
	}
}

void UHumanCharacterHUD_V2::NativeConstruct()
{
	Super::NativeConstruct();

	if (!GetWorld() || (GetWorld() && GetWorld()->bIsTearingDown))
		return;

	PlayerController = Cast<AReadyOrNotPlayerController>(GetOwningPlayer());
	if (PlayerController)
	{
		PlayerCharacter = Cast<APlayerCharacter>(GetOwningPlayerPawn());
		if (PlayerCharacter)
			PlayerCharacter->SetHumanCharacterWidget_V2(this);

		//RetainerBox_SwayableElements->GetEffectMaterial()->GetScalarParameterValue({"Distortion Amount"}, CurvedHUDStrength);

		if (AReadyOrNotGameState* GS = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState()))
		{
			GS->bInPlanningMenu = false;

			TArray<FRChatMessage> SavedMessages = GS->SavedChatMessages;
			GS->SavedChatMessages.Empty();
			
			for (const FRChatMessage& Msg : SavedMessages)
			{
				GS->Multicast_BroadcastChatMessage_Implementation(Msg);
			}
		}

		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UHumanCharacterHUD_V2::ReflectHUDSettings, 0.5f, true, true);
	}
}

void UHumanCharacterHUD_V2::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	//ProcessHUDSway(InDeltaTime);
}

void UHumanCharacterHUD_V2::ReflectHUDSettings_Implementation()
{
	
}

void UHumanCharacterHUD_V2::ChatPressed_Implementation()
{
}

void UHumanCharacterHUD_V2::TeamChatPressed_Implementation()
{
}

void UHumanCharacterHUD_V2::AddScorePopup_Implementation(const FText& ScoreText, int32 ScoreValue, bool bGive)
{
}

void UHumanCharacterHUD_V2::AddObjectivePopup_Implementation(const FText& PopupText)
{
}

void UHumanCharacterHUD_V2::ShowTutorialPrompt_Implementation(const FString& MessageID,bool bFirstShowing, const FText& MessageTitle, const TArray<FText>& MessageContent)
{
	
}

void UHumanCharacterHUD_V2::HideTutorialPrompt_Implementation(const FString& MessageID)
{

}


class UMapActorWidget* UHumanCharacterHUD_V2::AddMapActor_Implementation(class UMapActorComponent* MapActorComponent, TSubclassOf<class UMapActorWidget> MapActorIconWidgetClass, const FSlateBrush& IconBrush, const FLinearColor& IconColor, const FLinearColor& IconTextColor, float RotationOffset)
{
	return nullptr;
}

void UHumanCharacterHUD_V2::RemoveMapActor_Implementation(UMapActorComponent* MapActorComponent)
{
}

void UHumanCharacterHUD_V2::UpdateMapFloors_Implementation(const TArray<FBuildingFloor>& Floors)
{
}

void UHumanCharacterHUD_V2::ShowTutorialOverview_Implementation(const FString& MessageID, const FText& MessageTitle,const TArray<FText>& MessageContent)
{
}

void UHumanCharacterHUD_V2::HideTutorialOverview_Implementation(const FString& MessageID)
{
}

void UHumanCharacterHUD_V2::AssignPlayerActionPromptToFreeSlot(const FKey InputKey, const EInputEvent InputEvent, const FText InActionText, const FString InColorLabel, const bool bAnimate, const bool bLoopAnimation)
{
	if (InActionText.IsEmpty() || InActionText.ToString() == "None")
		return;
	
	if (!ActionSlot1InUse)
	{
		OnSlot1Updated.Broadcast(UReadyOrNotFunctionLibrary::FormatPlayerActionText(InputKey, InputEvent, InActionText, InColorLabel), false, bAnimate, bLoopAnimation);
		ActionSlot1InUse = true;
		return;
	}
	
	if (!ActionSlot2InUse)
	{
		OnSlot2Updated.Broadcast(UReadyOrNotFunctionLibrary::FormatPlayerActionText(InputKey, InputEvent, InActionText, InColorLabel), false, bAnimate, bLoopAnimation);
		ActionSlot2InUse = true;
		return;
	}

	if (!ActionSlot3InUse)
	{
		OnSlot3Updated.Broadcast(UReadyOrNotFunctionLibrary::FormatPlayerActionText(InputKey, InputEvent, InActionText, InColorLabel), false, bAnimate, bLoopAnimation);
		ActionSlot3InUse = true;
		return;
	}

	if (!ActionSlot4InUse)
	{
		OnSlot4Updated.Broadcast(UReadyOrNotFunctionLibrary::FormatPlayerActionText(InputKey, InputEvent, InActionText, InColorLabel), false, bAnimate, bLoopAnimation);
		ActionSlot4InUse = true;
		return;
	}
}

void UHumanCharacterHUD_V2::AssignPlayerActionPrompt(const int32 InSlot, const FKey InputKey, const EInputEvent InputEvent, const FText InActionText, const FString InColorLabel, const bool bAnimate, const bool bLoopAnimation)
{
	if (InActionText.IsEmpty() || InActionText.ToString() == "None")
		return;
		
	switch (InSlot)
	{
		case 0:
			OnSlot1Updated.Broadcast(UReadyOrNotFunctionLibrary::FormatPlayerActionText(InputKey, InputEvent, InActionText, InColorLabel), false, bAnimate, bLoopAnimation);
			ActionSlot1InUse = true;
			break;

		case 1:
			OnSlot2Updated.Broadcast(UReadyOrNotFunctionLibrary::FormatPlayerActionText(InputKey, InputEvent, InActionText, InColorLabel), false, bAnimate, bLoopAnimation);
			ActionSlot2InUse = true;
			break;

		case 2:
			OnSlot3Updated.Broadcast(UReadyOrNotFunctionLibrary::FormatPlayerActionText(InputKey, InputEvent, InActionText, InColorLabel), false, bAnimate, bLoopAnimation);
			ActionSlot3InUse = true;
			break;

		case 3:
			OnSlot4Updated.Broadcast(UReadyOrNotFunctionLibrary::FormatPlayerActionText(InputKey, InputEvent, InActionText, InColorLabel), false, bAnimate, bLoopAnimation);
			ActionSlot4InUse = true;
		break;
		
		default:
		break;
	}
}

int32 UHumanCharacterHUD_V2::AssignPlayerActionPromptToFreeSlot_Reserved(const FKey InputKey, const EInputEvent InputEvent, const FText InActionText, const FString InColorLabel, const bool bAnimate, const bool bLoopAnimation)
{
	if (InActionText.IsEmpty() || InActionText.ToString() == "None")
		return -1;

	const FText inActionTextKeyboard = UReadyOrNotFunctionLibrary::FormatPlayerActionText(UReadyOrNotFunctionLibrary::GetKeyFromInputActionName("Use", false), InputEvent, InActionText, InColorLabel);
	const FText inActionTextGamepad = UReadyOrNotFunctionLibrary::FormatPlayerActionText(UReadyOrNotFunctionLibrary::GetKeyFromInputActionName("Use", true), InputEvent, InActionText, InColorLabel);
	if (!ActionSlotReserved1InUse)
	{
		ActionSlotReserved1InUse = true;
		OnSlotReserved1Updated.Broadcast(inActionTextKeyboard, inActionTextGamepad, false, bAnimate, bLoopAnimation);
		return 0;
	}
	
	if (!ActionSlotReserved2InUse)
	{
		ActionSlotReserved2InUse = true;
		OnSlotReserved2Updated.Broadcast(inActionTextKeyboard, inActionTextGamepad, false, bAnimate, bLoopAnimation);
		return 1;
	}
	
	return -1;
}

void UHumanCharacterHUD_V2::AssignPlayerActionPrompt_Reserved(const int32 InSlot, const EInputEvent InputEvent, const FText InActionText, const FString InColorLabel, const bool bAnimate, const bool bLoopAnimation)
{
	if (InActionText.IsEmpty() || InActionText.ToString() == "None")
		return;

	const FText inActionTextKeyboard = UReadyOrNotFunctionLibrary::FormatPlayerActionText(UReadyOrNotFunctionLibrary::GetKeyFromInputActionName("Use", false), InputEvent, InActionText, InColorLabel);
	const FText inActionTextGamepad = UReadyOrNotFunctionLibrary::FormatPlayerActionText(UReadyOrNotFunctionLibrary::GetKeyFromInputActionName("Use", true), InputEvent, InActionText, InColorLabel);
	switch (InSlot)
	{
		case 0:
			ActionSlotReserved1InUse = true;
			OnSlotReserved1Updated.Broadcast(inActionTextKeyboard, inActionTextGamepad, false, bAnimate, bLoopAnimation);
		break;

		case 1:
			ActionSlotReserved2InUse = true;
			OnSlotReserved2Updated.Broadcast(inActionTextKeyboard, inActionTextGamepad, false, bAnimate, bLoopAnimation);
		break;
		
		default:
			break;
	}
}

void UHumanCharacterHUD_V2::AssignPlayerActionPrompt_Custom(const int32 InSlot, const FText InCustomPromptText, const bool bAnimate, const bool bLoopAnimation)
{
	switch (InSlot)
	{
		case 0:
			OnSlot1Updated.Broadcast(InCustomPromptText,false, bAnimate, bLoopAnimation);
			ActionSlot1InUse = true;
		break;

		case 1:
			OnSlot2Updated.Broadcast(InCustomPromptText, false, bAnimate, bLoopAnimation);
			ActionSlot2InUse = true;
		break;

		case 2:
			OnSlot3Updated.Broadcast(InCustomPromptText, false, bAnimate, bLoopAnimation);
			ActionSlot3InUse = true;
		break;

		case 3:
			OnSlot4Updated.Broadcast(InCustomPromptText, false, bAnimate, bLoopAnimation);
			ActionSlot4InUse = true;
		break;
		
		default:
        break;
	}
}

void UHumanCharacterHUD_V2::RemovePlayerActionPrompt(const int32 InSlot)
{
	FText sak = FText();
	switch (InSlot)
	{
		case 0:
			OnSlot1Updated.Broadcast(sak, true, false, false);
			ActionSlot2InUse = false;
		break;

		case 1:
			OnSlot2Updated.Broadcast(sak, true, false, false);
			ActionSlot2InUse = false;
		break;

		case 2:
			OnSlot3Updated.Broadcast(sak, true, false, false);
			ActionSlot3InUse = false;
			break;

		case 3:
			OnSlot4Updated.Broadcast(sak, true, false, false);
			ActionSlot4InUse = false;
		break;
		
		default:
        break;
	}
}

void UHumanCharacterHUD_V2::RemovePlayerActionPrompt_Reserved(const int32 InSlot)
{
	FText sak = FText();
	switch (InSlot)
	{
		case 0:
			OnSlotReserved1Updated.Broadcast(sak, sak, true, false, false);
			ActionSlotReserved1InUse = false;
				break;

		case 1:
			OnSlotReserved2Updated.Broadcast(sak, sak, true, false, false);
			ActionSlotReserved2InUse = false;
			PlayerActionText_Slot_Reserved_2->ClearText();
		break;

		default:
			break;
	}
}

void UHumanCharacterHUD_V2::ClearAllPlayerActionPrompts()
{
	FText sak = FText();
	OnSlot1Updated.Broadcast(sak, true, false, false);
	OnSlot2Updated.Broadcast(sak, true, false, false);
	OnSlot3Updated.Broadcast(sak, true, false, false);
	OnSlot4Updated.Broadcast(sak, true, false, false);
	ActionSlot1InUse = false;
	ActionSlot2InUse = false;
	ActionSlot3InUse = false;
	ActionSlot4InUse = false;
}

bool UHumanCharacterHUD_V2::IsActionSlotInUse(const int32 InSlot) const
{
	FText sak = FText();
	switch (InSlot)
	{
		case 0:
			return ActionSlot1InUse;
			break;

		case 1:
			return ActionSlot2InUse;
			break;

		case 2:
			return ActionSlot3InUse;
			break;

		case 3:
			return ActionSlot4InUse;
		break;
		
		default:
			break;
	}

	return false;
}

bool UHumanCharacterHUD_V2::IsActionSlotInUse_Reserved(const int32 InSlot) const
{
	switch (InSlot)
	{
		case 0:
			return ActionSlotReserved1InUse;
		break;

		case 1:
			return ActionSlotReserved2InUse;
		break;

		default:
			break;
	}

	return false;
}

void UHumanCharacterHUD_V2::ProcessHUDSway(const float InDeltaTime)
{
	if (bEnableHUDSway)
	{
		const FVector2D MouseDelta = GetMouseDelta();
		
		AccumulatedMouseDelta = (AccumulatedMouseDelta + MouseDelta).ClampAxes(-MaxHUDSwayMovement.X, MaxHUDSwayMovement.X);

		if (IsMouseAxisBeyondThreshold(MouseDelta))
		{
			const FVector2D CurrentRenderTranslation = RetainerBox_SwayableElements->RenderTransform.Translation;
			const FVector2D NewTranslation = {FMath::Clamp(FMath::Lerp(CurrentRenderTranslation.X, AccumulatedMouseDelta.X * SwayStrength.X, 0.85f * InDeltaTime), -MaxHUDSwayMovement.X, MaxHUDSwayMovement.X),
											FMath::Clamp(FMath::Lerp(CurrentRenderTranslation.Y, -AccumulatedMouseDelta.Y * SwayStrength.Y, 0.85f * InDeltaTime), -MaxHUDSwayMovement.Y, MaxHUDSwayMovement.Y)};
			
			RetainerBox_SwayableElements->SetRenderTranslation(NewTranslation);
		}
		else
		{
			const FVector2D CurrentRenderTranslation = RetainerBox_SwayableElements->RenderTransform.Translation;
			
			if (CurrentRenderTranslation != FVector2D::ZeroVector)
			{
				const FVector2D NewTranslation = {FMath::Lerp(CurrentRenderTranslation.X, 0.0f, SwaySpeed.X * InDeltaTime),
	                                            FMath::Lerp(CurrentRenderTranslation.Y, 0.0f, SwaySpeed.Y * InDeltaTime)};
				
				RetainerBox_SwayableElements->SetRenderTranslation(NewTranslation);
			}
		}
	
		AccumulatedMouseDelta = FMath::Lerp(AccumulatedMouseDelta, FVector2D::ZeroVector, 3.0f * InDeltaTime);
	}
}

void UHumanCharacterHUD_V2::LocalReadyStateChanged_Implementation(bool bReady)
{
}

void UHumanCharacterHUD_V2::ReadiedPlayersChanged_Implementation()
{
}

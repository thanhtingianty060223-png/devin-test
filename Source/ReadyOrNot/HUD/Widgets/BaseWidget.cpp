// Copyright Void Interactive, 2021

#include "BaseWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.H"

void UBaseWidget::ToggleWidgetVisibility(const bool bNotHitTestable)
{
	IsVisible() ? SetVisibility(ESlateVisibility::Hidden) : ShowWidget(bNotHitTestable);
}

void UBaseWidget::ShowWidget(const bool bNotHitTestable)
{
	if (bNotHitTestable)
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	else
		SetVisibility(ESlateVisibility::Visible);
}

void UBaseWidget::HideWidget()
{
	SetVisibility(ESlateVisibility::Hidden);
}

void UBaseWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RONGS = GetWorld()->GetGameState<AReadyOrNotGameState>();
}

void UBaseWidget::PlayWidgetAnimation_Internal(UWidgetAnimation* InWidgetAnimation, const bool bRestartIfAlreadyPlaying)
{
	if (!IsAnimationPlaying(InWidgetAnimation))
	{
		PlayAnimation(InWidgetAnimation);
		return;
	}
	
	if (bRestartIfAlreadyPlaying)
	{
		StopAnimation(InWidgetAnimation);
		PlayAnimation(InWidgetAnimation);
	}
}

void UBaseWidget::PauseWidgetAnimation_Internal(UWidgetAnimation* InWidgetAnimation)
{
	if (IsAnimationPlaying(InWidgetAnimation))
		PauseAnimation(InWidgetAnimation);
}

void UBaseWidget::StopWidgetAnimation_Internal(UWidgetAnimation* InWidgetAnimation)
{
	if (IsAnimationPlaying(InWidgetAnimation))
		StopAnimation(InWidgetAnimation);
}

bool UBaseWidget::UpdateDebugInfo_Implementation()
{
	return true;
}

FVector2D UBaseWidget::GetMouseDelta() const
{
	float DeltaX = 0.0f, DeltaY = 0.0f;
	
	if (GetOwningPlayer())
		GetOwningPlayer()->GetInputMouseDelta(DeltaX, DeltaY);

	return {DeltaX, DeltaY};
}

FVector2D UBaseWidget::GetMousePosition() const
{
	float MouseX = 0.0f, MouseY = 0.0f;
	
	if (GetOwningPlayer())
		GetOwningPlayer()->GetMousePosition(MouseX, MouseY);
	
	return {MouseX, MouseY};
}

bool UBaseWidget::HasMouseMoved() const
{
	const FVector2D MouseDelta = GetMouseDelta();

	return MouseDelta.X != 0.0f || MouseDelta.Y != 0.0f;
}

FVector2D UBaseWidget::GetCenterScreenPosition()
{
	return UWidgetLayoutLibrary::GetViewportSize(this) / 2;
}

bool UBaseWidget::IsBlockingAnimationPlaying_Implementation()
{
	APlayerCharacter* PCOwner = Cast<APlayerCharacter>(GetOwningPlayerPawn());
	if (PCOwner)
	{
		if (PCOwner->GetEquippedItem())
		{
			return PCOwner->GetEquippedItem()->IsBlockingAnimationPlaying({});
		}
	}
	
	return false;
}

bool UBaseWidget::IsMouseAxisBeyondThreshold(const FVector2D& InMouseDelta)
{
	return InMouseDelta.X > MouseAxisDeltaThreshold.X || InMouseDelta.X < -MouseAxisDeltaThreshold.X ||
		   InMouseDelta.Y > MouseAxisDeltaThreshold.Y || InMouseDelta.Y < -MouseAxisDeltaThreshold.Y; 
}

bool UBaseWidget::IsGamepadAxisBeyondThreshold(const FVector2D& InGamepadAxis)
{
	return InGamepadAxis.X > GamepadAxisDeltaThreshold.X || InGamepadAxis.X < -GamepadAxisDeltaThreshold.X ||
		   InGamepadAxis.Y > GamepadAxisDeltaThreshold.Y || InGamepadAxis.Y < -GamepadAxisDeltaThreshold.Y;
}

void UBaseWidget::PlaySoundEffect(UFMODEvent* SoundEffectToPlay)
{
	if (GetOwningPlayer())
		UFMODBlueprintStatics::PlayEvent2D(GetOwningPlayer()->GetWorld(), SoundEffectToPlay, true);
}

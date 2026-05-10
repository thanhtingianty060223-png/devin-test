// Copyright Void Interactive, 2023

#include "ActivatableBaseWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UActivatableBaseWidget::ToggleWidgetVisibility(bool bNotHitTestable)
{
	IsVisible() ? HideWidget() : ShowWidget(bNotHitTestable);
}

void UActivatableBaseWidget::ShowWidget(const bool bNotHitTestable)
{
	if (bNotHitTestable)
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	else
		SetVisibility(ESlateVisibility::Visible);

	ActivateWidget();
}

void UActivatableBaseWidget::HideWidget()
{
	SetVisibility(ESlateVisibility::Hidden);

	DeactivateWidget();
}

void UActivatableBaseWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RONGS = GetWorld()->GetGameState<AReadyOrNotGameState>();
}

void UActivatableBaseWidget::PlayWidgetAnimation_Internal(UWidgetAnimation* InWidgetAnimation, const bool bRestartIfAlreadyPlaying)
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

void UActivatableBaseWidget::PauseWidgetAnimation_Internal(UWidgetAnimation* InWidgetAnimation)
{
	if (IsAnimationPlaying(InWidgetAnimation))
		PauseAnimation(InWidgetAnimation);
}

void UActivatableBaseWidget::StopWidgetAnimation_Internal(UWidgetAnimation* InWidgetAnimation)
{
	if (IsAnimationPlaying(InWidgetAnimation))
		StopAnimation(InWidgetAnimation);
}

bool UActivatableBaseWidget::UpdateDebugInfo_Implementation()
{
	return true;
}

FVector2D UActivatableBaseWidget::GetMouseDelta() const
{
	float DeltaX = 0.0f, DeltaY = 0.0f;
	
	if (GetOwningPlayer())
		GetOwningPlayer()->GetInputMouseDelta(DeltaX, DeltaY);

	return {DeltaX, DeltaY};
}

FVector2D UActivatableBaseWidget::GetMousePosition() const
{
	float MouseX = 0.0f, MouseY = 0.0f;
	
	if (GetOwningPlayer())
		GetOwningPlayer()->GetMousePosition(MouseX, MouseY);
	
	return {MouseX, MouseY};
}

bool UActivatableBaseWidget::HasMouseMoved() const
{
	const FVector2D MouseDelta = GetMouseDelta();

	return MouseDelta.X != 0.0f || MouseDelta.Y != 0.0f;
}

FVector2D UActivatableBaseWidget::GetCenterScreenPosition()
{
	return UWidgetLayoutLibrary::GetViewportSize(this) / 2;
}

bool UActivatableBaseWidget::IsBlockingAnimationPlaying_Implementation()
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

bool UActivatableBaseWidget::IsMouseAxisBeyondThreshold(const FVector2D& InMouseDelta)
{
	return InMouseDelta.X > MouseAxisDeltaThreshold.X || InMouseDelta.X < -MouseAxisDeltaThreshold.X ||
		   InMouseDelta.Y > MouseAxisDeltaThreshold.Y || InMouseDelta.Y < -MouseAxisDeltaThreshold.Y; 
}

bool UActivatableBaseWidget::IsGamepadAxisBeyondThreshold(const FVector2D& InGamepadAxis)
{
	return InGamepadAxis.X > GamepadAxisDeltaThreshold.X || InGamepadAxis.X < -GamepadAxisDeltaThreshold.X ||
		   InGamepadAxis.Y > GamepadAxisDeltaThreshold.Y || InGamepadAxis.Y < -GamepadAxisDeltaThreshold.Y;
}

void UActivatableBaseWidget::PlaySoundEffect(UFMODEvent* SoundEffectToPlay)
{
	if (GetOwningPlayer())
		UFMODBlueprintStatics::PlayEvent2D(GetOwningPlayer()->GetWorld(), SoundEffectToPlay, true);
}

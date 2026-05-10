// Copyright Void Interactive, 2023

#include "AnimatedIconWidget_Imprint.h"

#include "Components/Image.h"

void UAnimatedIconWidget_Imprint::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);
	
	SetRenderOpacity(0.0f);
	SetVisibility(ESlateVisibility::Collapsed);
	StopAllAnimations();
}

void UAnimatedIconWidget_Imprint::Init(const FVector InWorldLocation, UTexture2D* InIconImage)
{
	SetIconImage(InIconImage);
	SetRenderOpacity(1.0f);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	
	FVector2D ScreenPosition;
	UGameplayStatics::ProjectWorldToScreen(GetOwningPlayer(), InWorldLocation, ScreenPosition, false);
	
	SetPositionInViewport(ScreenPosition, true);
	PlayAnimation(ImprintAnimation);
}

void UAnimatedIconWidget_Imprint::SetIconImage(UTexture2D* NewIconImage)
{
	FSlateBrush Brush;
	Brush.SetResourceObject(NewIconImage);
	
	Icon_Image->SetBrush(Brush);
}

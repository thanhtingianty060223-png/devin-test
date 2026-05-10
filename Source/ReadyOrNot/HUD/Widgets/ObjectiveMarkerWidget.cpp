// Void Interactive, 2020

#include "ObjectiveMarkerWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

#include "Components/ObjectiveMarkerComponent.h"

void UObjectiveMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UObjectiveMarkerWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!ParentComponent)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		RemoveFromParent();
		ConditionalBeginDestroy();
		return;
	}

	if (!ParentComponent->GetOwner())
	{
		SetVisibility(ESlateVisibility::Collapsed);

		ParentComponent->SetVisibility(false);
		ParentComponent->SetHiddenInGame(true);
		ParentComponent = nullptr;
		
		RemoveFromParent();
		ConditionalBeginDestroy();
		return;
	}

	if (!ParentComponent->bEnabled)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	
	if (!RootCanvasPanel->IsVisible())
		return;
	
	if (GetOwningPlayerPawn())
	{
		DistanceToLocalPlayer = FVector::Distance(Location, GetOwningPlayerPawn()->GetActorLocation());
		
		if (ParentComponent->bDistanceScaleIcon)
		{
			// ##UE5UPGRADE## FMath
			float IconSize = FMath::GetMappedRangeValueClamped<float,float>({0.0f, 2000.0f}, {40.0f, 28.0f}, DistanceToLocalPlayer);
			Icon_SizeBox->SetWidthOverride(IconSize);
			Icon_SizeBox->SetHeightOverride(IconSize);
			
			DirectionalArrow_Image->SetDesiredSizeOverride(ParentComponent->IsObjectiveMarkerOffscreen() ? FVector2D(64.0f) : FVector2D(IconSize + 4.0f));
		}
	}
	
	// ##UE5UPGRADE## FMath
	const FVector2D NewScale = FVector2D(FMath::GetMappedRangeValueClamped<float,float>({1500.0f, 5000.0f}, {1.0f, 0.6f}, DistanceToLocalPlayer));
	const FVector2D TextScale = FVector2D(FMath::GetMappedRangeValueClamped<float,float>({1500.0f, 5000.0f}, {1.0f, 0.7f}, DistanceToLocalPlayer));
	
	MarkerName_Text->SetRenderScale(TextScale);
	MarkerName_Text->SetRenderOpacity(NewScale.X);
	
	if (ParentComponent->GetOffscreenMarkerWidget() == this)
		MarkerName_Text->SetVisibility(ESlateVisibility::Collapsed);

	if (Icon_Image->Brush.GetResourceObject() && Icon_Image->IsVisible())
	{
		UWidgetLayoutLibrary::SlotAsCanvasSlot(MarkerName_Text)->bAutoSize = false;
		UWidgetLayoutLibrary::SlotAsCanvasSlot(MarkerName_Text)->SetAlignment({-0.125f, 0.0f});
		MarkerName_Text->SetJustification(ETextJustify::Left);
	}
	else
	{
		UWidgetLayoutLibrary::SlotAsCanvasSlot(MarkerName_Text)->bAutoSize = true;
		UWidgetLayoutLibrary::SlotAsCanvasSlot(MarkerName_Text)->SetAlignment({0.5f, 0.5f});
		MarkerName_Text->SetJustification(ETextJustify::Center);
	}

	ESlateVisibility DirectionalArrowVisibility = ParentComponent->IsObjectiveMarkerOffscreen() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (ParentComponent->bHideDirectionalArrow)
	{
		DirectionalArrowVisibility = ESlateVisibility::Collapsed;
	}
	
	DirectionalArrow_Image->SetVisibility(DirectionalArrowVisibility);
	DirectionalArrow_Image->SetRenderTransformAngle(DirectionAngle + 90.0f);

	bHideDistance = ParentComponent->bHideDistanceInfo ? (ParentComponent->HideDistanceInfoAtDistance <= 0.0f ? true : DistanceToLocalPlayer < ParentComponent->HideDistanceInfoAtDistance) : false;
	
	if (bHideDistance)
	{
		if (ParentComponent->HideDistanceInfoAtDistance <= 0.0f)
			DistanceInMeters_Text->SetRenderOpacity(0.0f);
		else
			DistanceInMeters_Text->SetRenderOpacity(FMath::FInterpTo(DistanceInMeters_Text->GetRenderOpacity(), 0.0f, InDeltaTime, 8.0f));
	}
	else
	{
		DistanceInMeters_Text->SetRenderOpacity(FMath::FInterpTo(DistanceInMeters_Text->GetRenderOpacity(), ParentComponent->IsObjectiveMarkerOffscreen() ? 0.85f : NewScale.X, InDeltaTime, 8.0f));
	}

	DistanceInMeters_Text->SetRenderScale(ParentComponent->IsObjectiveMarkerOffscreen() ? FVector2D(0.85f) : TextScale);
}

void UObjectiveMarkerWidget::ShowIcon()
{
	if (Icon_Image->Brush.GetResourceObject() != nullptr)
	{
		Icon_Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		Icon_Image->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UObjectiveMarkerWidget::HideIcon()
{
	Icon_Image->SetVisibility(ESlateVisibility::Collapsed);
}

void UObjectiveMarkerWidget::ShowAll()
{
	RootCanvasPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ShowIcon();
	MarkerName_Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	DistanceInMeters_Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UObjectiveMarkerWidget::HideAll()
{
	HideIcon();
	MarkerName_Text->SetVisibility(ESlateVisibility::Collapsed);
	DistanceInMeters_Text->SetVisibility(ESlateVisibility::Collapsed);
}

void UObjectiveMarkerWidget::ShowMarkerText()
{
	MarkerName_Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UObjectiveMarkerWidget::HideMarkerText()
{
	MarkerName_Text->SetVisibility(ESlateVisibility::Collapsed);
}

void UObjectiveMarkerWidget::SetMarkerNameText(const FText NewMarkerNameText)
{
	MarkerName_Text->SetText(NewMarkerNameText);
}

void UObjectiveMarkerWidget::SetMarkerNameTextFontSize(const int32 NewFontSize)
{
	FSlateFontInfo CachedFont = MarkerName_Text->Font;
	CachedFont.Size = NewFontSize;
	
	MarkerName_Text->SetFont(CachedFont);
}

void UObjectiveMarkerWidget::SetIconImage(const FSlateBrush& InBrush)
{
	Icon_Image->SetBrush(InBrush);
}

void UObjectiveMarkerWidget::SetIconSize(const FVector2D NewIconSize)
{
	Icon_SizeBox->SetWidthOverride(NewIconSize.X);
	Icon_SizeBox->SetHeightOverride(NewIconSize.Y);
}

void UObjectiveMarkerWidget::SetIconColorAndOpacity(const FLinearColor& InColor)
{
	Icon_Image->SetColorAndOpacity(InColor);
}

void UObjectiveMarkerWidget::SetMarkerNameTextColorAndOpacity(const FLinearColor& InColor)
{
	MarkerName_Text->SetColorAndOpacity(InColor);
}

void UObjectiveMarkerWidget::SetTargetLocation(const FVector NewLocation)
{
	Location = NewLocation;
}

void UObjectiveMarkerWidget::SetDirectionAngle(const float Angle)
{
	DirectionAngle = Angle;
}

void UObjectiveMarkerWidget::OnMarkerVisibilityEnabled_Implementation()
{
}

void UObjectiveMarkerWidget::OnMarkerVisibilityDisabled_Implementation()
{
}

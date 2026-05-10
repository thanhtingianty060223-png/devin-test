// Copyright Void Interactive, 2021

#include "RadialSectorWidget.h"
#include "ReadyOrNot.h"

#include "Components/Image.h"

#include "Components/OverlaySlot.h"

#include "Blueprint/WidgetLayoutLibrary.h"

#include "Kismet/KismetMaterialLibrary.h"

#if !UE_BUILD_SHIPPING
#include "Log.h"
#endif

bool URadialSectorWidget::InitializeSectorWidget_Implementation(const float Angle, const float Percentage, const float InSectorInnerRadius, const float InSectorOuterRadius, class UMaterialInterface* InSectorMaterial, const FLinearColor& UnselectedColor, UImage* InSectorImage)
{
	UOverlaySlot* OverlaySlot = UWidgetLayoutLibrary::SlotAsOverlaySlot(this);
	OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
	OverlaySlot->SetVerticalAlignment(VAlign_Fill);

	UImage* SectorImageToUse;
	if (InSectorImage)
	{
		SectorImageToUse = InSectorImage;
	}
	else
	{
		SectorImageToUse = SectorImage;
	}

	UMaterialInterface* SectorMaterialToUse;
	if (InSectorMaterial)
	{
		SectorMaterialToUse = InSectorMaterial;
	}
	else
	{
		SectorMaterialToUse = Cast<UMaterialInterface>(SectorImageToUse->Brush.GetResourceObject());
	}

	UMaterialInstanceDynamic* MID_Sector = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, SectorMaterialToUse);
	if (MID_Sector)
	{
		MID_Sector->SetScalarParameterValue("Start Point", Angle);
		MID_Sector->SetScalarParameterValue("Percentage", Percentage);
		MID_Sector->SetScalarParameterValue("Inner Radius", InSectorInnerRadius);
		MID_Sector->SetScalarParameterValue("Outer Radius", InSectorOuterRadius);

		FSlateBrush SlateBrush;
		SlateBrush.ImageSize = {512.0f, 512.0f};
		SlateBrush.TintColor = FSlateColor(UnselectedColor);
		SlateBrush.SetResourceObject(MID_Sector);

		SectorImageToUse->SetBrush(SlateBrush);

		return true;
	}

	return false;
}

bool URadialSectorWidget::SetSectorColor_Implementation(const FLinearColor& NewColor, UImage* InSectorImage)
{
	if (InSectorImage)
	{
		InSectorImage->GetDynamicMaterial()->SetVectorParameterValue("Selected Color", NewColor);
		InSectorImage->SetBrushTintColor(NewColor);

		return true;
	}

	SectorImage->GetDynamicMaterial()->SetVectorParameterValue("Selected Color", NewColor);
	SectorImage->SetBrushTintColor(NewColor);

	return true;
}

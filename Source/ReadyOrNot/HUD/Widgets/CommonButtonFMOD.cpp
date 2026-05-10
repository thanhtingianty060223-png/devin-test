// Copyright Void Interactive, 2023

#include "HUD/Widgets/CommonButtonFMOD.h"

void UCommonButtonFMOD::NativeOnHovered()
{
	if (OnHoveredEvent)
	{
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), OnHoveredEvent, true);
	}
	Super::NativeOnHovered();
}

void UCommonButtonFMOD::NativeOnClicked()
{
	if (OnClickedEvent)
	{
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), OnClickedEvent, true);
	}
	Super::NativeOnClicked();
}

void UCommonButtonFMOD::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	if (OnHoveredEvent)
	{
		UFMODBlueprintStatics::PlayEvent2D(GetWorld(), OnHoveredEvent, true);
	}
	Super::NativeOnAddedToFocusPath(InFocusEvent);
}

void UCommonButtonFMOD::NativeOnCurrentTextStyleChanged()
{
	if (UCommonButtonStyle* MyStyle = GetStyle())
	{
		if (const UCommonButtonStyleFMOD* StyleFMOD = Cast<UCommonButtonStyleFMOD>(MyStyle))
		{
			OnHoveredEvent = StyleFMOD->OnHoveredEvent;
			OnClickedEvent = StyleFMOD->OnClickedEvent;
		}
	}
	Super::NativeOnCurrentTextStyleChanged();
}

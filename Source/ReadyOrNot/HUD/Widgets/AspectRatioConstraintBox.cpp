// Copyright Void Interactive, 2023

#include "AspectRatioConstraintBox.h"

#include "Components/SizeBoxSlot.h"
#include "HUD/Slate/SAspectRatioConstraintBox.h"

UAspectRatioConstraintBox::UAspectRatioConstraintBox()
{
	bIsVariable = false;
	Visibility = ESlateVisibility::SelfHitTestInvisible;
}

TSharedRef<SWidget> UAspectRatioConstraintBox::RebuildWidget()
{
	ConstraintBox = SNew(SAspectRatioConstraintBox);

	if (GetChildrenCount() > 0)
	{
		Cast<USizeBoxSlot>(GetContentSlot())->BuildSlot(ConstraintBox.ToSharedRef());
	}
	
	return ConstraintBox.ToSharedRef();
}

void UAspectRatioConstraintBox::OnWidgetRebuilt()
{
	Super::OnWidgetRebuilt();

	UReadyOrNotGameUserSettings* GameUserSettings = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (GameUserSettings && !bUseFixedConstraint)
	{
		GameUserSettings->OnSettingsSaved.RemoveAll(this);
		GameUserSettings->OnSettingsSaved.AddUObject(this, &UAspectRatioConstraintBox::OnSettingsUpdated);
	}
	OnSettingsUpdated();

	if (bUseFixedConstraint && ConstraintBox.IsValid())
	{
		ConstraintBox->SetDesiredAspectRatio(FixedAspectRatio);
	}
}

UClass* UAspectRatioConstraintBox::GetSlotClass() const
{
	return USizeBoxSlot::StaticClass();
}

void UAspectRatioConstraintBox::OnSlotAdded(UPanelSlot* InSlot)
{
	// Add the child to the live slot if it already exists
	if (ConstraintBox.IsValid())
	{
		CastChecked<USizeBoxSlot>(Slot)->BuildSlot(ConstraintBox.ToSharedRef());
	}
}

void UAspectRatioConstraintBox::OnSlotRemoved(UPanelSlot* InSlot)
{
	// Remove the widget from the live slot if it exists
	if (ConstraintBox.IsValid())
	{
		ConstraintBox->SetContent(SNullWidget::NullWidget);
	}
}

void UAspectRatioConstraintBox::OnSettingsUpdated()
{
	UReadyOrNotGameUserSettings* GameUserSettings = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (!GameUserSettings)
		return;
	
	if (!ConstraintBox.IsValid())
		return;

	if (bUseFixedConstraint)
	{
		ConstraintBox->SetDesiredAspectRatio(FixedAspectRatio);
		return;
	}
		
	
	if (GameUserSettings->InterfaceAspectRatio >= 1.0f)
	{
		ConstraintBox->SetDesiredAspectRatio(GameUserSettings->InterfaceAspectRatio);
	}
	else
	{
		ConstraintBox->SetDesiredAspectRatio(FOptionalSize());
	}
}

void UAspectRatioConstraintBox::SetFixedAspectRatio(float NewAspectRatio)
{
	if (NewAspectRatio > 0)
	{
		FixedAspectRatio = NewAspectRatio;
		bUseFixedConstraint = true;
	}
	else
	{
		bUseFixedConstraint = false;
	}
		
	OnSettingsUpdated();
	
}

void UAspectRatioConstraintBox::EnableConstraint()
{
	bUseFixedConstraint = true;

	OnSettingsUpdated();
}

void UAspectRatioConstraintBox::DisableConstraint()
{
	bUseFixedConstraint = false;

	OnSettingsUpdated();
}

void UAspectRatioConstraintBox::SynchronizeProperties()
{
	Super::SynchronizeProperties();

#if WITH_EDITOR
	if (!ConstraintBox.IsValid())
		return;
	
	if (IsDesignTime())
	{
		PreviewAspectRatio = FMath::Max(PreviewAspectRatio, 1.0f);
		ConstraintBox->SetDesiredAspectRatio(bEnablePreview ? PreviewAspectRatio : FOptionalSize());
	}
#endif
}

void UAspectRatioConstraintBox::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	ConstraintBox.Reset();
}

#if WITH_EDITOR
const FText UAspectRatioConstraintBox::GetPaletteCategory()
{
	return NSLOCTEXT("UMG", "Panel", "Panel");
}
#endif
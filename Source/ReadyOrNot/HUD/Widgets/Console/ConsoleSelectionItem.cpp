// Copyright Void Interactive, 2022

#include "HUD/Widgets/Console/ConsoleSelectionItem.h"

#include "Components/Image.h"

PRAGMA_DISABLE_OPTIMIZATION

FString UConsoleSelectionItem::GetItemName()
{
	return Name;
}

void UConsoleSelectionItem::SetImage(const FSlateBrush& Brush) const
{
	this->Icon_Image->SetBrush(Brush);
}

void UConsoleSelectionItem::SetSelected(bool Selected)
{
	if (Selected)
	{
		SetRenderOpacity(1.0f);
		Selected_Image->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		SetRenderOpacity(0.5f);
		Selected_Image->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UConsoleSelectionItem::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UConsoleSelectionItem::NativeConstruct()
{
	Super::NativeConstruct();
}

bool UConsoleSelectionItem::Initialize()
{
	const bool Initialized = Super::Initialize();
	this->SetRenderOpacity(0.5f);
	this->SetSelected(false);
	return Initialized;
}

PRAGMA_ENABLE_OPTIMIZATION

// Copyright Void Interactive, 2022

#include "HUD/Widgets/Console/ConsoleHotkey.h"

#include "Components/Image.h"

void UConsoleHotkey::SetIconBrush(FSlateBrush Brush)
{
	if (Icon_Image != nullptr)
	{
		Icon_Image->SetBrush(Brush);
		InvalidateLayoutAndVolatility();
	}
}

bool UConsoleHotkey::Initialize()
{
	const bool Initialized = Super::Initialize();
	SetIconBrush(Icon_Brush);
	return Initialized;
}

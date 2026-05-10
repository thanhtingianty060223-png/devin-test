// Copyright Void Interactive, 2022

#pragma once

#include "Blueprint/UserWidget.h"
#include "ConsoleHotkey.generated.h"

UCLASS()
class READYORNOT_API UConsoleHotkey : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetIconBrush(FSlateBrush Brush);

protected:

	UPROPERTY(EditAnywhere, Category = "Required Widgets", meta = (BindWidget))
	FSlateBrush Icon_Brush;

	UPROPERTY(EditAnywhere, Category = "Required Widgets", meta = (BindWidget))
	UImage* Icon_Image = nullptr;

	virtual bool Initialize() override;
};

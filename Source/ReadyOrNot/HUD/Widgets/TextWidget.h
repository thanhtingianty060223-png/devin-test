// Copyright Void Interactive, 2023

#pragma once

#include "Blueprint/UserWidget.h"
#include "TextWidget.generated.h"

UCLASS()
class READYORNOT_API UTextWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetText(const FText& NewText);
	void SetTextColor(const FLinearColor& NewColor);
	void SetFont(const FSlateFontInfo& NewFont);

	FText GetText() const;
	FSlateFontInfo GetFont() const;
	
protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UCommonTextBlock* txt_Text = nullptr;
	
	UPROPERTY(BlueprintReadWrite)
	FText CurrentText;
	
	UPROPERTY(BlueprintReadWrite)
	FSlateFontInfo CurrentFont;
};

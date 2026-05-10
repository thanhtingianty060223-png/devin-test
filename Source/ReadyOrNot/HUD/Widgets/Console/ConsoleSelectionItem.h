// Copyright Void Interactive, 2022

#pragma once

#include "Blueprint/UserWidget.h"
#include "ConsoleSelectionItem.generated.h"

UCLASS()

class READYORNOT_API UConsoleSelectionItem : public UUserWidget
{
	GENERATED_BODY()

public:
	FString GetItemName();
	void SetImage(const FSlateBrush& Brush) const;
	void SetSelected(bool Selected);

	UPROPERTY()
	FString Name;

	UPROPERTY()
	int Count = 0;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget), Transient)
	UImage* Icon_Image = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Required Widgets", meta = (BindWidget), Transient)
	UImage* Selected_Image = nullptr;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual bool Initialize() override;
};

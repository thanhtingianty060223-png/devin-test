// Copyright Void Interactive, 2021

#pragma once

#include "Engine/DataTable.h"
#include "Blueprint/UserWidget.h"
#include "WidgetDataTable.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FWidgetLookupData : public FTableRowBase
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UUserWidget> WidgetClass;

	// describe the widget here
	UPROPERTY(EditAnywhere)
	FString Description;
	
	// show the mouse cursor here
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bShowMouseCursor = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bMouseUIOnly = false;
	
	// show the mouse cursor here
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bAddToWidgetStack = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bCloseOnRespawn = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESlateVisibility VisibilityUponCreation = ESlateVisibility::SelfHitTestInvisible;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ZOrder = -1;

	FWidgetLookupData()
	{
		WidgetClass = nullptr;
		Description = "";
	}
};

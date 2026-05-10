// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RealtimeWidget.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API URealtimeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * This is called after a widget is constructed and properties are synchronized.
	 * It can also be called by the editor to update modified state.
	 * Override this event in blueprint to update the widget after a default property is modified.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "User Interface")
		void OnSynchronizeProperties();

	// Overriding this with a custom event allows us to update the editor window.
	virtual void SynchronizeProperties() override;
};

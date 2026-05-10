// Void Interactive, 2020

#pragma once

#include "Blueprint/UserWidget.h"
#include "CursorWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UCursorWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};

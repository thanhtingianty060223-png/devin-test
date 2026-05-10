// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AISelectionDebugWidget.generated.h"


UCLASS()
class READYORNOT_API UAISelectionDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "AI Selection Widget", meta = (BindWidget), Transient)
	class UTextBlock* AIName_TextBlock = nullptr;
};

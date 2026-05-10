// Copyright Void Interactive, 2023

#pragma once

#include "Loadout_Carousel_V3.generated.h"

UCLASS()
class READYORNOT_API ULoadout_Carousel_V3 : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetText(FText InputText);
protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UTextBlock* Text;
};

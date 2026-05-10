// Copyright Void Interactive, 2017

#pragma once

#include "Blueprint/UserWidget.h"
#include "TabletDisplay.generated.h"

/*
 *	Tablet Display HUD.
 */
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UTabletDisplay : public UUserWidget
{
	GENERATED_BODY()

public:
	//UPROPERTY(BlueprintReadOnly, Category = Tablet)
	//TScriptInterface<IControllableByTablet> TargettedCharacter;

	UPROPERTY(BlueprintReadOnly, Category = Tablet)
	class ATablet* OwningTablet;
};

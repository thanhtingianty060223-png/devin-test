// Copyright Void Interactive, 2017

#pragma once

#include "UObject/Interface.h"
#include "ControllableByTablet.generated.h"

UINTERFACE(BlueprintType, Blueprintable)
class READYORNOT_API UControllableByTablet : public UInterface
{
	GENERATED_BODY()
};

class READYORNOT_API IControllableByTablet
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|ControllableWithTablet")
	bool CanControlWithTablet(class APlayerCharacter* TabletOwner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|ControllableWithTablet")
	void AssumeTabletControl(class APlayerCharacter* TabletOwner);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|ControllableWithTablet")
	bool CanTabletViewMe(class APlayerCharacter* TabletOwner, class AReadyOrNotGameState* GameState);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|ControllableWithTablet")
	USceneComponent* GetTabletViewComponent();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|ControllableWithTablet")
		FName GetTabletViewSocket();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|ControllableWithTablet")
	ETeamType GetTabletViewTeamColor();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|ControllableWithTablet")
	void HideActorsForTabletView(class USceneCaptureComponent2D* Component);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|ControllableWithTablet")
	FText GetTabletNameText();
};

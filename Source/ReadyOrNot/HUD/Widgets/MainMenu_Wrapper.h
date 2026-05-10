// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MainMenu_Wrapper.generated.h"

UCLASS()
class READYORNOT_API UMainMenu_Wrapper : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	UTexture2D* RenderToMesh(UUserWidget* Widget, const FVector2D& DrawSize);

public:
	UFUNCTION(BlueprintCallable)
	void OpenModMenu();
	UFUNCTION(BlueprintCallable)
	void CloseModMenu();
};

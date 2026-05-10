// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "CommonButtonFMOD.generated.h"

UCLASS()
class READYORNOT_API UCommonButtonStyleFMOD : public UCommonButtonStyle
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Common UI FMOD")
	UFMODEvent* OnHoveredEvent;
	UPROPERTY(EditAnywhere, Category="Common UI FMOD")
	UFMODEvent* OnClickedEvent;
};

UCLASS()
class READYORNOT_API UCommonButtonFMOD : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Common UI FMOD")
	UFMODEvent* OnHoveredEvent;
	UPROPERTY(EditAnywhere, Category="Common UI FMOD")
	UFMODEvent* OnClickedEvent;

protected:
	virtual void NativeOnHovered() override;
	virtual void NativeOnClicked() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnCurrentTextStyleChanged() override;
};

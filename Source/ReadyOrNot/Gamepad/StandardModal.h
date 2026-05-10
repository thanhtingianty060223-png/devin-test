// Copyright Void Interactive, 2023

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StandardModal.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCancelClickedDelegate, UStandardModal*, CallingModal);

UCLASS()
class READYORNOT_API UStandardModal : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StandardModal", meta = (ExposeOnSpawn = "true"))
	FText TitleBar = FText::FromString("Title Bar Text");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StandardModal", meta = (ExposeOnSpawn = "true"))
	FText ModalDescription = FText::FromString("Modal description");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StandardModal", meta = (ExposeOnSpawn = "true"))
	bool ShowApplyButton;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StandardModal", meta = (ExposeOnSpawn = "true"))
	bool ShowCancelButton;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StandardModal", meta = (ExposeOnSpawn = "true"))
	bool ShowOkButton;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StandardModal", meta = (ExposeOnSpawn = "true"))
	FText ApplyButtonText = FText::FromString("Apply");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StandardModal", meta = (ExposeOnSpawn = "true"))
	FText CancelButtonText = FText::FromString("Cancel");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StandardModal", meta = (ExposeOnSpawn = "true"))
	FText OkButtonText = FText::FromString("OK");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StandardModal", meta = (ExposeOnSpawn = "true"))
	bool RequireScrollToBottom;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StandardModal", meta = (ExposeOnSpawn = "true"))
	FVector2D Size = FVector2D(300, 200);

	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = "StandardModal")
	FCancelClickedDelegate OnCancelClicked;
};
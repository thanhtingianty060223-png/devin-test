// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "PageWidget.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EInputExclusiveType : uint8
{
	AllInputs,
	MouseAndKeyboard,
	Gamepad,
	Touch
};

USTRUCT(BlueprintType)
struct FScreenFooterEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EInputExclusiveType OnlyDisplayWithInput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TEnumAsByte<ETextJustify::Type> Alignment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool IsButton;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool ActionIsVisualOnly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FText ButtonLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InputAction, meta = (RowType = CommonInputActionDataBase, TitleProperty = "RowName"))
		FDataTableRowHandle InputAction;

};

UCLASS()
class READYORNOT_API UPageWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()
};

UCLASS(Abstract, Blueprintable, ClassGroup = UI)
class READYORNOT_API UPageFooter : public UPageWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = true))
		TMap<FString, FScreenFooterEntry> FooterEntries;

	UFUNCTION(BlueprintCallable)
	void GetInputActionData(FDataTableRowHandle InputActionRow, FText& ActionName, FKey& ActionKey);
};
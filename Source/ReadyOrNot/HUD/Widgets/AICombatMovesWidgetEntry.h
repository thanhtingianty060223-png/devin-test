// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AICombatMovesWidgetEntry.generated.h"

UCLASS()
class READYORNOT_API UAICombatMovesWidgetEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Combat Moves Widget")
	class UTextBlock* CombatAction_TextBlock = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Combat Moves Widget")
	class UTextBlock* SuccessfulConsiderations_TextBlock = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Combat Moves Widget")
	class UTextBlock* FailedConsiderations_TextBlock = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Combat Moves Widget")
	class UTextBlock* RunTime_TextBlock = nullptr;
};

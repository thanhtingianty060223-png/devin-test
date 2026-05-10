// Copyright Void Interactive, 2021

#pragma once

#include "UObject/Interface.h"
#include "Reportable.generated.h"

UINTERFACE(BlueprintType, Blueprintable)
class READYORNOT_API UReportable : public UInterface
{
	GENERATED_BODY()
};

class READYORNOT_API IReportable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Interfaces|Reportable")
	void ReportToTOC(class AReadyOrNotCharacter* Reporter, bool bPlayAnimation = true);

	UFUNCTION(BlueprintNativeEvent, Category = "Interfaces|Reportable")
	bool CanReportNow();

	UFUNCTION(BlueprintNativeEvent, Category = "Interfaces|Reportable")
	FString GetSpeechTypeForReport();
};

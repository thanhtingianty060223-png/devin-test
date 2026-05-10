// Copyright Void Interactive, 2021

#pragma once

#include "GameFramework/SaveGame.h"
#include "LicenseSave.generated.h"

/**
 *	LicenseSave contains all license agreement acceptances (Alpha NDA, etc)
 *	@author	eezstreet
 */
UCLASS(BlueprintType)
class READYORNOT_API ULicenseSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = Alpha)
	bool bAcceptedAlphaNonDisclosureAgreement = false;

	UFUNCTION(BlueprintCallable, Category = Alpha)
		void AcceptAlphaNDA() { bAcceptedAlphaNonDisclosureAgreement = true; }
};
// Copyright Void Interactive, 2023

#pragma once

#include "UObject/Interface.h"
#include "Securable.generated.h"

UINTERFACE(MinimalAPI)
class USecurable : public UInterface
{
	GENERATED_BODY()
};

class READYORNOT_API ISecurable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Securable")
	void Secure(AReadyOrNotCharacter* Instigator);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Securable")
	FVector GetLocation() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Securable")
	bool IsSecured() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Securable")
	bool CanBeSecured() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interfaces|Securable")
	bool CanBeSecuredByTrailers() const;
};

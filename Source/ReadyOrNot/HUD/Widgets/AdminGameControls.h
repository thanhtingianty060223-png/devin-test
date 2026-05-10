// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AdminGameControls.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAdminGameControls : public UUserWidget
{
	GENERATED_BODY()

	UFUNCTION(BlueprintPure)
	bool IsAdmin();

	UFUNCTION(BlueprintCallable)
	void KickPlayer(APlayerState* KickingPlayerState, FText Reason);
	
	UFUNCTION(BlueprintCallable, Category = "Server Administration")
	void GetKickablePlayers(TArray<APlayerState*>& KickablePlayers);
	
};

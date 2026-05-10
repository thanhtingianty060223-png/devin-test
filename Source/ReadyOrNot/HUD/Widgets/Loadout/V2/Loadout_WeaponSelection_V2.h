// Copyright Void Interactive, 2023

#pragma once

#include "CommonActivatableWidget.h"
#include "Loadout_WeaponSelection_V2.generated.h"

UCLASS()
class READYORNOT_API ULoadout_WeaponSelection_V2 : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetShouldUpdateWorkbench(bool ShouldUpdate);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetShouldUpdateWorkbench();
	// UFUNCTION(BlueprintCallable)
	// void SetWorkbenchItemClass(TSubclassOf<ABaseItem> Item, FName Tag, FSavedLoadout ActiveLoadout);
	// UFUNCTION(BlueprintCallable)
	// void UpdateWorkbenchItemAttachments(FSavedLoadout ActiveLoadout);

protected:
	UPROPERTY(BlueprintReadWrite)
	int ActiveSwitcherIndex = 0;
	
	virtual void NativeOnActivated() override;

private:
	UPROPERTY()
	bool ShouldUpdateWorkbench = true;
};

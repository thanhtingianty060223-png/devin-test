// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CollectableWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UCollectableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	
	UFUNCTION(BlueprintCallable)
	void CloseCollectableWidget();
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetItem(class ACollectable* Collectable);
	
	UPROPERTY()
	class ACollectableViewController* ParentController;
};

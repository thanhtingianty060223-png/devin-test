// Void Interactive, 2020

#pragma once

#include "Blueprint/UserWidget.h"
#include "IconWidget.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetParentComponent(UInteractableComponent* NewParent) { ParentComponent = NewParent; }

	void SetIcon(const FSlateBrush& NewIconBrush);

	UFUNCTION(BlueprintPure, Category = "Icon Widget")
	FString GetAttachedObjectName() const;

	UFUNCTION(BlueprintPure, Category = "Icon Widget")
	UTexture2D* GetAttachedObjectIcon() const;
	
protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category = "Icon Widget|Required Widgets", meta = (BindWidget))
	class UImage* MainImage = nullptr;
	
	// Set from UInteractableComponent, used to access data from interactables
	UPROPERTY(BlueprintReadOnly, Category = "Icon Widget")
	class UInteractableComponent* ParentComponent;
};

// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Components/ContentWidget.h"
#include "AspectRatioConstraintBox.generated.h"

/**
 * Aspect ratio constraint box, fixes the aspect ratio of the content to the user's desired setting
 */
UCLASS()
class READYORNOT_API UAspectRatioConstraintBox : public UContentWidget
{
	GENERATED_BODY()

public:
	UAspectRatioConstraintBox();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Aspect Ratio")
	bool bUseFixedConstraint = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Aspect Ratio", meta=(EditCondition="bUseFixedConstraint"))
	float FixedAspectRatio = 16.0f / 9.0f;

#if WITH_EDITORONLY_DATA
	// Enable previewing the aspect ratio set in the "PreviewAspectRatio" variable
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Aspect Ratio")
	bool bEnablePreview = false;

	// The aspect ratio to preview in the designer when previewing is enabled
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Aspect Ratio", meta=(EditCondition="bEnablePreview"))
	float PreviewAspectRatio = 16.0f / 9.0f;
#endif

	UFUNCTION(BlueprintCallable, Category = "Aspect Ratio")
	void SetFixedAspectRatio(float NewAspectRatio);

	UFUNCTION(BlueprintCallable, Category = "Aspect Ratio")
	void EnableConstraint();

	UFUNCTION(BlueprintCallable, Category = "Aspect Ratio")
	void DisableConstraint();
	
protected:
	TSharedPtr<class SAspectRatioConstraintBox> ConstraintBox;
	
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void OnWidgetRebuilt() override;

	virtual UClass* GetSlotClass() const override;
	virtual void OnSlotAdded(UPanelSlot* InSlot) override;
	virtual void OnSlotRemoved(UPanelSlot* InSlot) override;
	
	UFUNCTION()
	void OnSettingsUpdated();

public:
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif
};

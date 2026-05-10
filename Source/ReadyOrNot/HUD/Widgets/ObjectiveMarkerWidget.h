// Void Interactive, 2020

#pragma once

#include "HUD/Widgets/BaseWidget.h"
#include "ObjectiveMarkerWidget.generated.h"

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class READYORNOT_API UObjectiveMarkerWidget : public UBaseWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Objective Marker")
	void OnMarkerVisibilityEnabled();
	
	UFUNCTION(BlueprintNativeEvent, Category = "Objective Marker")
	void OnMarkerVisibilityDisabled();
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker")
	void ShowIcon();
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker")
	void HideIcon();

	UFUNCTION(BlueprintCallable, Category = "Objective Marker")
	void ShowAll();
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker")
	void HideAll();

	UFUNCTION(BlueprintCallable, Category = "Objective Marker")
	void ShowMarkerText();
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker")
	void HideMarkerText();
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker")
	void SetMarkerNameText(FText NewMarkerNameText);
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker")
	void SetMarkerNameTextFontSize(int32 NewFontSize);
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker")
	void SetIconImage(const FSlateBrush& InBrush);
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker")
	void SetIconSize(FVector2D NewIconSize);
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker")
	void SetIconColorAndOpacity(const FLinearColor& InColor);
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker")
	void SetMarkerNameTextColorAndOpacity(const FLinearColor& InColor);
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker")
	void SetTargetLocation(FVector NewLocation);
	
	UFUNCTION(BlueprintCallable, Category = "Objective Marker")
	void SetDirectionAngle(float Angle);
	
	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker Widget|Data")
	class UObjectiveMarkerComponent* ParentComponent = nullptr;
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual void OnMarkerVisibilityEnabled_Implementation();
	virtual void OnMarkerVisibilityDisabled_Implementation();
	
	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker Widget|Required Widgets", meta = (BindWidget))
	class UCanvasPanel* RootCanvasPanel = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker Widget|Required Widgets", meta = (BindWidget))
	class USizeBox* Icon_SizeBox = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker Widget|Required Widgets", meta = (BindWidget))
	class UImage* Icon_Image = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker Widget|Required Widgets", meta = (BindWidget))
	class UImage* DirectionalArrow_Image = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker Widget|Required Widgets", meta = (BindWidget))
	class UTextBlock* DistanceInMeters_Text = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker Widget|Required Widgets", meta = (BindWidget))
	class UTextBlock* MarkerName_Text = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker Widget|Data")
	FVector Location = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker Widget|Data")
	float DistanceToLocalPlayer = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker Widget|Data")
	float DirectionAngle = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Objective Marker Widget|Data")
	uint8 bHideDistance : 1;
};

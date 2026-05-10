// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RadialSectorWidget.generated.h"

/**
 * An abstract base class for creating radial sectors of a radial menu.
 * You must derive from this class when instantiating/creating this widget.
 * @author Ali
 */
UCLASS(Abstract, BlueprintType)
class READYORNOT_API URadialSectorWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Sector Widget")
	bool InitializeSectorWidget(float Angle, float Percentage, float InSectorInnerRadius, float InSectorOuterRadius, class UMaterialInterface* InSectorMaterial, const FLinearColor& UnselectedColor, class UImage* InSectorImage = nullptr);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Radial Sector Widget")
	bool SetSectorColor(const FLinearColor& NewColor, class UImage* InSectorImage = nullptr);

	UPROPERTY(BlueprintReadOnly, Category = "Radial Sector Widget", meta = (BindWidget))
	class UPanelWidget* SectorImagePanel = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Radial Sector Widget", meta = (BindWidget))
	class UImage* SectorImage = nullptr;

protected:
	virtual bool InitializeSectorWidget_Implementation(float Angle, float Percentage, float InSectorInnerRadius, float InSectorOuterRadius, class UMaterialInterface* InSectorMaterial, const FLinearColor& UnselectedColor, class UImage* InSectorImage = nullptr);
	virtual bool SetSectorColor_Implementation(const FLinearColor& NewColor, class UImage* InSectorImage = nullptr);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Sector Widget | Spawn Properties", meta = (ExposeOnSpawn = true, ClampMin = 0.0f, ClampMax = 1.0f))
	float SectorInnerRadius = 0.49f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radial Sector Widget | Spawn Properties", meta = (ExposeOnSpawn = true, ClampMin = 0.0f, ClampMax = 1.0f))
	float SectorOuterRadius = 0.5f;
};

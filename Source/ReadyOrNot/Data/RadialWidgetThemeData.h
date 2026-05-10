// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RadialWidgetThemeData.generated.h"

/**
 * A data asset that is used in conjunction with URadialWidgetBase widgets. This overrides and applies visual settings to the widget that references this data asset.
 * @author Ali
 */
UCLASS()
class READYORNOT_API URadialWidgetThemeData : public UDataAsset
{
	GENERATED_BODY()

public:
	URadialWidgetThemeData();

	// An optional name. Can be used for UI purposes.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data")
    FName Name = NAME_None;

	// An optional description. Can be used for UI purposes.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data")
	FText Description = FText::FromString("No description");
	
	// The sector index of the radial wheel we should automatically select, when we first create this widget. -1 = select nothing
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data", meta = (ClampMin = -1, UIMin = -1))
	int32 StartingSectorIndex = -1;

	// The angle to start at when creating sectors on the radial wheel
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 360.0f, UIMax = 360.0f))
	float StartingSectorAngle = 0.0f;

	// The size of each image widget
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data", meta = (ClampMin = 0.0f, UIMin = 0.0f))
    float IconSize = 60.0f;

	// The offset from the circumference of the radial wheel
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data", meta = (ClampMin = 0.0f, UIMin = 0.0f))
    float IconPadding = 70.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 0.5f, UIMax = 0.5f))
	float SectorInnerRadius = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 0.5f, UIMax = 0.5f))
	float SectorOuterRadius = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data", meta = (ClampMin = 0.0f, UIMin = 0.0f))
	float GapSize = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 2048.0f, UIMax = 2048.0f))
	float WheelSize = 800.0f;

	// The distance from the center of the radial wheel widget to place the radial wheel cursor at
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 2048.0f, UIMax = 2048.0f))
	float WheelCursorDistanceFromCenterWheel = 160.0f;

	// Upon opening a radial menu, should we hide the radial wheel cursor widget?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data")
    uint8 bHideRadialWheelCursorOnMenuOpened : 1;

	// The color to use when a sector of the radial wheel is currently selected
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data")
	FLinearColor SelectedColor = FLinearColor(0.765625f, 0.0f, 0.0f, 0.7f);

	// The color to use when a sector of the radial wheel is not currently selected
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data")
	FLinearColor UnselectedColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.7f);

	// The color to use when a sector of the radial wheel cannot be selected
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data")
	FLinearColor UnselectableColor = FLinearColor(0.385417f, 0.0f, 0.0f, 0.611f);

	// The font to use to represent text on this radial menu
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data")
    UFont* Font = nullptr;
	
	// The sound to play when selecting a sector
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data")
    UFMODEvent* SelectionSound = nullptr;

	// The sound to play when opening a radial menu
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data")
    UFMODEvent* MenuOpenSound = nullptr;
	
	// The sound to play when closing a radial menu
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data")
    UFMODEvent* MenuCloseSound = nullptr;

	// The sound to play when closing a radial menu, if there are no active selections on a radial menu
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme Data")
    UFMODEvent* MenuCloseSound_NoSelection = nullptr;
};

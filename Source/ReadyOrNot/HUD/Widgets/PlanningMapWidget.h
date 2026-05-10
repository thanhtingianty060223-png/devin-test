// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlanningMapWidget.generated.h"

// different colors that we can use for free draw
UENUM(BlueprintType)
enum class EFreeDrawColor : uint8
{
	FDC_Black,	// actually white
	FDC_Red,
	FDC_Blue,
	FDC_Gold,
	FDC_Green,
	FDC_Purple,
	FDC_Orange,
	FDC_Cyan,
};

UENUM(BlueprintType)
enum class EPlanningPage : uint8
{
	PP_None,
	PP_Overview,
	PP_Spawn,
	PP_Deployables,
	PP_Tactics,
	PP_FreePlanning
};

UENUM(BlueprintType)
enum class ESituationPage : uint8
{
	SP_None,
	SP_Objectives,
	SP_Suspects,
	SP_Civilians,
	SP_Timeline
};


UENUM(BlueprintType)
enum class EPlanningStage : uint8
{
	PS_None,
	PS_Planning,
	PS_Situation,
	PS_Loadout
};

UENUM(BlueprintType)
enum class EPlanningMapStage : uint8
{
	PMS_Overview,
	PMS_Spawn,
	PMS_Deployables,
	PMS_PersonnelMain,
	PMS_PersonnelPoint,
	PMS_PersonnelMapZones,
	PMS_FreeDraw,
};

UENUM(BlueprintType)
enum class EPlanningMapTool : uint8
{
	PMT_Draw,
	PMT_Pan,
};

// a line that is drawn on the preplanning
USTRUCT(BlueprintType)
struct FMapData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = PointData)
	TArray<FVector2D> PointData;

	UPROPERTY(BlueprintReadOnly, Category = PointData)
	EFreeDrawColor ColorType;

	UPROPERTY(BlueprintReadOnly, Category = PointData)
	float Thickness;
};

USTRUCT(BlueprintType)
struct FFloorMapPointData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Point Data")
	TArray<FMapData> MapData;
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UPlanningMapWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Planning")
		bool bDrawable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Free Planning")
		TArray<FFloorMapPointData> DrawPointData;

	UFUNCTION(BlueprintCallable, Category = "Free Planning")
		void AddPointData(FVector2D PointData, int32 FloorNum, EFreeDrawColor Color, bool bNewPoint, float Thickness);

	UFUNCTION(BlueprintCallable, Category = "Free Planning")
		void ClearPointData(bool bClearAll, int32 FloorNum, bool bClearAllFloors);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
		UWorld* GetWorldContext() { return GetWorld(); }

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void OnPersonnelAdded(int32 PersonnelNum, int32 PersonnelZone);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void OnPersonnelRemoved(int32 PersonnelNum);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void OnForceMapRefresh();
};

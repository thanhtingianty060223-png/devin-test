// Copyright Void Interactive, 2017

#include "PlanningMapWidget.h"
#include "ReadyOrNot.h"

#define FloorMapData DrawPointData[FloorNum].MapData

void UPlanningMapWidget::AddPointData(FVector2D PointData, int32 FloorNum, EFreeDrawColor Color, bool bNewPoint, float Thickness)
{
	FMapData newData;
	newData.PointData.AddUnique(PointData);
	newData.ColorType = Color;
	newData.Thickness = Thickness;

	if (!DrawPointData.IsValidIndex(FloorNum) && DrawPointData.Num() == 0)
	{
		// Re-initialize map data if blank
		FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetLevelData();
		if (LevelData.MapFloors.Num() == 0)
		{
			return;
		}
		DrawPointData.AddDefaulted(LevelData.MapFloors.Num());
	}
	else if (!DrawPointData.IsValidIndex(FloorNum))
	{
		// Not a valid floor to be drawing on
		return;
	}

	if (FloorMapData.IsValidIndex(FloorMapData.Num() - 1) && !bNewPoint)
	{
		FloorMapData[FloorMapData.Num() - 1].PointData.Add(PointData);
	}
	else
	{
		FloorMapData.Add(newData);
	}

	AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(GetOwningPlayer());
	if (pc)
	{
		//pc->Server_AddDrawingPointData(PointData, FloorNum, Color, bNewPoint, Thickness);
	}
}

void UPlanningMapWidget::ClearPointData(bool bClearAll, int32 FloorNum, bool bClearAllFloors)
{
	if (bClearAllFloors)
	{
		DrawPointData.Empty();
	}
	else if (bClearAll)
	{
		if (DrawPointData.IsValidIndex(FloorNum))
		{
			FloorMapData.Empty();
		}
	}
	else if(DrawPointData.IsValidIndex(FloorNum))
	{
		if (FloorMapData.IsValidIndex(FloorMapData.Num() - 1))
		{
			FloorMapData.RemoveAt(FloorMapData.Num() - 1);
		}
	}
	AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(GetOwningPlayer());
	if (pc)
	{
		//pc->Server_ClearDrawingPointData(bClearAll, bClearAllFloors, FloorNum);
	}
}

#undef FloorMapData

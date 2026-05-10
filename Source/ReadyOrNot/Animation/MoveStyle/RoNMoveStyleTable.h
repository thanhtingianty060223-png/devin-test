#pragma once

#include "Engine/DataTable.h"
#include "RoNMovementShared.h"
#include "RoNMoveStyleTable.generated.h"

USTRUCT(BlueprintType)
struct FRoNMoveStyleTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, meta = (TitleProperty = "Name"), Category = "Movement Style Data")
	TArray<FRoNMovementStyle> MoveStyles;
};

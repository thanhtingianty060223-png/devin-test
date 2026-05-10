// Void Interactive, 2020

#pragma once

#include "PatrolPoint.generated.h"

USTRUCT()
struct FPatrolPoint
{
	GENERATED_BODY()

	FIntVector Location; // lot more stable than floating point when trying to test for equality
	TArray<FName> Tags;

	friend bool operator==(const FPatrolPoint& Lhs, const FPatrolPoint& Rhs)
	{
		return Lhs.Location == Rhs.Location;
	}
};

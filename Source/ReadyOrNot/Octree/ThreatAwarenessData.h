// Void Interactive, 2020

#pragma once

#include "Actors/ThreatAwarenessActor.h"

struct FThreatAwarenessData
{
	FThreatAwarenessData() {}
	FThreatAwarenessData(class AThreatAwarenessActor* InThreatAwarenessActor, const FVector& InThreatLocation, const EThreatLevel& InThreatLevel)
	{
		ThreatAwarenessActor = InThreatAwarenessActor;
		ThreatLocation = InThreatLocation;
		ThreatLevel = InThreatLevel;
	}

	class AThreatAwarenessActor* ThreatAwarenessActor = nullptr;
	FVector ThreatLocation = FVector::ZeroVector;
	EThreatLevel ThreatLevel = EThreatLevel::TL_Low;

	friend bool operator==(const FThreatAwarenessData& Lhs, const FThreatAwarenessData& Rhs)
	{
		return Lhs.ThreatAwarenessActor == Rhs.ThreatAwarenessActor;
	}
};

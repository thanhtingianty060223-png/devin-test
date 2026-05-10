// Copyright Void Interactive, 2022

#pragma once

#include "AIDataTypes.generated.h"

UENUM(BlueprintType)
enum class EAIAwarenessState : uint8
{
	Unalerted, // Idle, nothing aggressive or suspicous being sensed
	Suspicious, // when hearing an aggressive noise
	Alerted // When seen enemy
};

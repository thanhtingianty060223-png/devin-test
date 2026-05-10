// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"

template<typename T>
class TGenericCooldownList
{
public:
	TMap<T, float> Map;
	
	void Tick(float DeltaTime)
	{
		TArray<T> KeysToRemoveFromMap;
		for (auto& Element : Map)
		{
			Element.Value -= DeltaTime;
			
			if (Element.Value <= 0.0f)
				KeysToRemoveFromMap.AddUnique(Element.Key);
		}

		for (const T Controller : KeysToRemoveFromMap)
		{
			KeysToRemoveFromMap.Remove(Controller);
		}
	}
};

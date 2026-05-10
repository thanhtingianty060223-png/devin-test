// Copyright Void Interactive, 2021

#pragma once

#include "Containers/UnrealString.h"
#include "Delegates/DelegateCombinations.h"
#include "Delegates/Delegate.h"

class READYORNOT_API FReadyOrNotCoreDelegates
{
public:
	// Checks for steam emulators and other programs
	//DECLARE_MULTICAST_DELEGATE_OneParam(FPiracyCheckDelegate, bool /*bIsPirated*/);
	//static FPiracyCheckDelegate PiracyCheckUpdate;
	
	// Checks for steam emulators and other programs
	//DECLARE_MULTICAST_DELEGATE_TwoParams(FPiracyCheckDelegate_Detail, bool /*bIsPirated*/, const FString& /*ProgramDetected*/);
	//static FPiracyCheckDelegate_Detail PiracyCheckUpdate_Detail;
	
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnHUDCreated, class UHumanCharacterHUD_V2* /*HUD*/);
	static FOnHUDCreated OnHUDCreated;
	
private:
	FReadyOrNotCoreDelegates();
	~FReadyOrNotCoreDelegates();
};


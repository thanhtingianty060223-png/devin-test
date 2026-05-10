// Copyright Void Interactive, 2017

#include "SteamworksIntegration.h"
#include "AdvancedSteamFriendsLibrary.h"

bool USteamworksIntegration::IsSteamEnabled()
{
#if defined(WITH_STEAM)
	return true;
	
#else
	return false;
#endif
}

bool USteamworksIntegration::IsDLCInstalled(int32 AppID)
{
	if (GetDLCENum(AppID) == EGameVersionRestriction::GVR_NoRestriction)
		return true;

	bool bEnabledOnSteam = false;
#if defined(WITH_STEAM)
	if (SteamApps())
	{
		bEnabledOnSteam = SteamApps()->BIsDlcInstalled(AppId_t(AppID));
	}
#endif

	if (UReadyOrNotStatics::GetReadyOrNotGameInstance())
	{
		TMap<int32, bool> OwnedDLCMap = UReadyOrNotStatics::GetReadyOrNotGameInstance()->OwnedDLCMap;
		if (OwnedDLCMap.Find(AppID))
		{
			return OwnedDLCMap[AppID];
		}
	}	
	
	return bEnabledOnSteam;

}

bool USteamworksIntegration::IsDLCInstalledEnum(EGameVersionRestriction DLC)
{
	// all DLC enabled for editor
	if (DLC == EGameVersionRestriction::GVR_NoRestriction)
		return true;
	
	int32 DLCNumber = GetDLCNumber(DLC);
	bool bEnabledOnSteam = false;
#if defined(WITH_STEAM)
	if (SteamApps())
	{
		bEnabledOnSteam = SteamApps()->BIsDlcInstalled(AppId_t(DLCNumber));
	}
#endif


	if (UReadyOrNotStatics::GetReadyOrNotGameInstance())
	{
		TMap<int32, bool> OwnedDLCMap = UReadyOrNotStatics::GetReadyOrNotGameInstance()->OwnedDLCMap;
		if (OwnedDLCMap.Find(DLCNumber))
		{
			return OwnedDLCMap[DLCNumber];
		}
	}

	
	return bEnabledOnSteam;
}

int32 USteamworksIntegration::GetDLCNumber(EGameVersionRestriction InDLC)
{
	switch (InDLC)
	{
	case EGameVersionRestriction::GVR_Base:
		return STEAM_APPID_CORE_GAME;
	case EGameVersionRestriction::GVR_Supporter:
		return STEAM_DLC_SUPPORTER;
	case EGameVersionRestriction::GVR_PreorderBonus:
		return STEAM_DLC_PREORDER_BONUS;
	case EGameVersionRestriction::GVR_Demo:
		return STEAM_APPID_DEMO_GAME;
	default:
		ensureMsgf(false, TEXT("No DLC provided for Game Feature %i"), InDLC);
		return -1;
	}
}

EGameVersionRestriction USteamworksIntegration::GetDLCENum(int32 InDLC)
{
	if (InDLC == STEAM_DLC_SUPPORTER)
	{
		return EGameVersionRestriction::GVR_Supporter;
	}
	if (InDLC == STEAM_DLC_PREORDER_BONUS)
	{
		return EGameVersionRestriction::GVR_PreorderBonus;
	}
	return EGameVersionRestriction::GVR_Base;
}

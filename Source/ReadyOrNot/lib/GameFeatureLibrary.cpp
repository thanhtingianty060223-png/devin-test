// Copyright Void Interactive, 2023


#include "GameFeatureLibrary.h"

#if !(UE_BUILD_SHIPPING)
static TAutoConsoleVariable<int32> CVarIsDemoMode(
	TEXT("IsDemoMode"),
	0,
	TEXT("Enables / Disables Demo mode.\n")
	TEXT("  0: off (fastest)\n")
	TEXT("  1: Demo Mode\n"));

static TAutoConsoleVariable<int32> CVarSupporterInEditor(
	TEXT("EnableSupporterInEditor"),
	1,
	TEXT("Enable supporter DLC in editor"));
#endif

TMap<EGameVersionRestriction, bool> UGameFeatureLibrary::Cache;
TMap<EGameFeature, EGameVersionRestriction> UGameFeatureLibrary::FeatureRestrictions;

UGameFeatureLibrary::UGameFeatureLibrary()
{
	FeatureRestrictions.Add(EGameFeature::GF_None, EGameVersionRestriction::GVR_NoRestriction);
	FeatureRestrictions.Add(EGameFeature::GF_Practice, EGameVersionRestriction::GVR_NoRestriction);
	FeatureRestrictions.Add(EGameFeature::GF_Training, EGameVersionRestriction::GVR_NoRestriction);
	FeatureRestrictions.Add(EGameFeature::GF_Commander, EGameVersionRestriction::GVR_Base);
	FeatureRestrictions.Add(EGameFeature::GF_Mulitplayer, EGameVersionRestriction::GVR_Base);
	FeatureRestrictions.Add(EGameFeature::GF_ModSupport, EGameVersionRestriction::GVR_Base);

}

bool UGameFeatureLibrary::IsFeatureEnabled(EGameFeature InFeature)
{
	if(InFeature == EGameFeature::GF_None)
		return true;

	ensure(FeatureRestrictions.Contains(InFeature));

	if(!FeatureRestrictions.Contains(InFeature))
	{
		const FString ResourceString = StaticEnum<EGameFeature>()->GetValueAsString(InFeature);
		UE_LOG(LogReadyOrNot, Fatal, TEXT("No Feature Resctriction defined for Feature %s"), *ResourceString);
		return false;
	}

	return IsGameVersionEnabled(FeatureRestrictions[InFeature]);
}

bool UGameFeatureLibrary::IsGameVersionEnabled(EGameVersionRestriction InFeature)
{
	// These don't require cache
	if(InFeature == EGameVersionRestriction::GVR_NoRestriction)
		return true;
	if(InFeature == EGameVersionRestriction::GVR_Base)
		return IsFullGame();
	if(InFeature == EGameVersionRestriction::GVR_Demo)
		return IsGameDemo();

#if !(UE_BUILD_SHIPPING)
	if (InFeature == EGameVersionRestriction::GVR_Supporter &&
		CVarSupporterInEditor.GetValueOnAnyThread() != 0)
		return true;
#endif
	
	// Speed up querying from Steam
	if(Cache.Contains(InFeature))
		return Cache[InFeature];

#if defined(WITH_STEAM)
	const bool bResult = UGameFeatureLibrary::IsGameVersionEnabled(InFeature);
	Cache.Add(InFeature, bResult);
	return bResult;
#endif
	// TODO EOS
	ensure(false);
	return false;
}

bool UGameFeatureLibrary::IsFullGame()
{
#if defined(RON_DEMO)
	return false;
#endif
#if !(UE_BUILD_SHIPPING)
	return CVarIsDemoMode.GetValueOnAnyThread() == 0;
#endif
	return true;
}

bool UGameFeatureLibrary::IsGameDemo()
{
	// TODO Check command line args for testing...
	return !IsFullGame();
}

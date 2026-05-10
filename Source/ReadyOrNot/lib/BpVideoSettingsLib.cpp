// Copyright Void Interactive, 2024

#include "BpVideoSettingsLib.h"

#include "ReadyOrNot.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformMisc.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#if defined(NVIDIA_REFLEX_ENABLED) && defined(RONENGINE)
#include "ReflexBlueprint.h"
#endif
#endif
// ##UE5UPGRADE## Re add this after DLSS upgraded       
//#include "DLSSLibrary.h"
//#include "StreamlineLibraryDLSSG.h"
#include "GenericPlatform/GenericPlatformMisc.h"

UGameUserSettings* UBpVideoSettingsLib::GetGameUserSettings()
{
	if (GEngine != nullptr)
	{
		return GEngine->GameUserSettings;
	}
	return nullptr;
}

bool UBpVideoSettingsLib::GetSupportedScreenResolutions(TArray<FString>& Resolutions)
{
	FScreenResolutionArray ResolutionsArray;

	if (RHIGetAvailableResolutions(ResolutionsArray, true))
	{
		for (const FScreenResolutionRHI& Resolution : ResolutionsArray)
		{
			// Don't add any stupidly low resolutions that we can't support
			if (Resolution.Height > 720 && Resolution.Width > 1280)
			{
				FString StrW = FString::FromInt(Resolution.Width);
				FString StrH = FString::FromInt(Resolution.Height);
				Resolutions.AddUnique(StrW + "x" + StrH);
				// add eg 1920x1080 format
			}
		}
		return true;
	}
	return false; // failed to receive screen resolutions
}

FString UBpVideoSettingsLib::GetCurrentScreenResolution()
{
	UGameUserSettings* Settings = GetGameUserSettings();
	if (!Settings)
		return "";

	return FString::FromInt(Settings->GetScreenResolution().X) + "x" + FString::FromInt(Settings->GetScreenResolution().Y);
}

EWindowMode::Type UBpVideoSettingsLib::GetCurrentScreenMode()
{
	UGameUserSettings* Settings = GetGameUserSettings();
	if (!Settings)
		return EWindowMode::WindowedFullscreen;

	return Settings->GetLastConfirmedFullscreenMode();
}

bool UBpVideoSettingsLib::SetScreenResolution(const int32 Width, const int32 Height, EWindowMode::Type NewWindowMode)
{
	UGameUserSettings* Settings = GetGameUserSettings();
	if (!Settings)
		return false;

	Settings->SetScreenResolution(FIntPoint(Width, Height));
	Settings->SetFullscreenMode(NewWindowMode);
	Settings->ConfirmVideoMode();
	SaveVideoModeAndQuality();
	return true;


}

bool UBpVideoSettingsLib::ChangeScreenResolution(const int32 Width, const int32 Height, EWindowMode::Type NewWindowMode)
{
	UGameUserSettings* Settings = GetGameUserSettings();
	if (!Settings)
		return false;

	EWindowMode::Type WindowMode = NewWindowMode;
	Settings->RequestResolutionChange(Width, Height, WindowMode, false);
	Settings->SetFullscreenMode(NewWindowMode);
	Settings->ConfirmVideoMode();
	return true;
}

bool UBpVideoSettingsLib::GetInterfaceAspectRatio(float& OutAspectRatio)
{
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (Settings)
	{
		OutAspectRatio = Settings->InterfaceAspectRatio;
		return true;
	}
	return false;
}

bool UBpVideoSettingsLib::SetInterfaceAspectRatio(float InAspectRatio)
{
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (Settings)
	{
		Settings->InterfaceAspectRatio = InAspectRatio;
		Settings->SaveSettings();
		return true;
	}
	return false;
}

bool UBpVideoSettingsLib::GetVideoQualitySettings(int32& AntiAliasing, int32& Effects, int32& PostProcess, float& ResolutionScaling, int32& Shadow, int32& Texture, int32& ViewDistance)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	// Load ron settings

	// AntiAliasing = Settings->ScalabilityQuality.AntiAliasingQuality;
	// Effects = Settings->ScalabilityQuality.EffectsQuality;
	// PostProcess = Settings->ScalabilityQuality.PostProcessQuality;
	// 
	// Shadow = Settings->ScalabilityQuality.ShadowQuality;
	// Texture = Settings->ScalabilityQuality.TextureQuality;
	// ViewDistance = Settings->ScalabilityQuality.ViewDistanceQuality;
	// just store RON variables cause we fake some of these settings (ie shadows and VFX due to crashes and issues)
	ResolutionScaling = Settings->ScalabilityQuality.ResolutionQuality / 100.0f;
	AntiAliasing = Settings->AntiAliasingQuality;
	Effects = Settings->EffectsQuality;
	PostProcess = Settings->PostProcessQuality;
	Shadow = Settings->ShadowQuality;
	Texture = Settings->TextureQuality;
	ViewDistance = Settings->ViewDistanceQuality;
	

	return true;
}

bool UBpVideoSettingsLib::GetVideoQualitySettingsAsString(FText& OverallSetting, FText& AntiAliasing, FText& Effects, FText& PostProcess, FText& Shadow, FText& Texture, FText& ViewDistance)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	OverallSetting = GenerateQualityString(Settings->GetOverallScalabilityLevel());
	AntiAliasing = GenerateQualityString(Settings->AntiAliasingQuality);
	Effects = GenerateQualityString(Settings->EffectsQuality);
	PostProcess = GenerateQualityString(Settings->PostProcessQuality);
	Shadow = GenerateQualityString(Settings->ShadowQuality);
	Texture = GenerateQualityString(Settings->TextureQuality);
	ViewDistance = GenerateQualityString(Settings->ViewDistanceQuality);
	return true;
}

bool UBpVideoSettingsLib::GetAntiAliasingQuality(int32& AntiAliasing)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	AntiAliasing = Settings->AntiAliasingQuality;
	return true;
}

bool UBpVideoSettingsLib::GetEffectsQuality(int32& Effects)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	Effects = Settings->EffectsQuality;
	return true;
}

bool UBpVideoSettingsLib::GetPostProcessQuality(int32& PostProcess)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	PostProcess = Settings->PostProcessQuality;
	return true;
}

bool UBpVideoSettingsLib::GetResolutionScaling(float& ResolutionScaling)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	ResolutionScaling = Settings->ScalabilityQuality.ResolutionQuality / 100.0f;
	return true;
}

bool UBpVideoSettingsLib::GetShadowQuality(int32& Shadow)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	Shadow = Settings->ShadowQuality;
	return true;
}

bool UBpVideoSettingsLib::GetTextureQuality(int32& Texture)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	Texture = Settings->TextureQuality;
	return true;
}

bool UBpVideoSettingsLib::GetViewDistanceQuality(int32& ViewDistance)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	ViewDistance = Settings->ViewDistanceQuality;
	return true;
}

bool UBpVideoSettingsLib::GetPerObjectShadowsEnabled(bool& bPerObjectShadowsEnabled)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	bPerObjectShadowsEnabled = Settings->bEnablePerObjectShadows;
	return true;
}

bool UBpVideoSettingsLib::SetPerObjectShadowsEnabled(bool bPerObjectShadowsEnabled)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	Settings->bEnablePerObjectShadows = bPerObjectShadowsEnabled;
	return true;
}

bool UBpVideoSettingsLib::GetDepthOfFieldSetting(bool& bDoFEnabled)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	bDoFEnabled = Settings->bDepthOfField;
	return true;
}

bool UBpVideoSettingsLib::SetDepthofFieldSetting(bool bDoFEnabled)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	Settings->bDepthOfField = bDoFEnabled;
	return true;
}

FText UBpVideoSettingsLib::GenerateQualityString(int32 Quality)
{
	switch (Quality)
	{
	case 0:
		return NSLOCTEXT("CodeDefinedLiterals", "VideoOptions_Low", "Low");
	case 1:
		return NSLOCTEXT("CodeDefinedLiterals", "VideoOptions_Medium", "Medium");
	case 2:
		return NSLOCTEXT("CodeDefinedLiterals", "VideoOptions_High", "High");
	case 3:
		return NSLOCTEXT("CodeDefinedLiterals", "VideoOptions_Epic", "Epic");
	case 4:
		return NSLOCTEXT("CodeDefinedLiterals", "VideoOptions_Cinematic", "Cinematic");
	}
	return FText();
}

FText UBpVideoSettingsLib::GenerateDlssQualityString(int32 Quality)
{
	switch (Quality)
	{
	case 0:
		return NSLOCTEXT("CodeDefinedLiterals", "DLSSOptions_Off", "Off");
	case 1:
		return NSLOCTEXT("CodeDefinedLiterals", "DLSSOptions_Auto", "Auto");
	case 2:
		return NSLOCTEXT("CodeDefinedLiterals", "DLSSOptions_UltraQuality", "Ultra Quality");
	case 3:
		return NSLOCTEXT("CodeDefinedLiterals", "DLSSOptions_Quality", "Quality");
	case 4:
		return NSLOCTEXT("CodeDefinedLiterals", "DLSSOptions_Balanced", "Balanced");
	case 5:
		return NSLOCTEXT("CodeDefinedLiterals", "DLSSOptions_Performance", "Performance");
	case 6:
		return NSLOCTEXT("CodeDefinedLiterals", "DLSSOptions_UltraPerformance", "Ultra Performance");
	}
	return FText();
}

FText UBpVideoSettingsLib::GenerateFSRQualityString(int32 Quality)
{
	switch (Quality)
	{
	case 0:
		return NSLOCTEXT("CodeDefinedLiterals", "FSROptions_Off", "Off");
	case 1:
		return NSLOCTEXT("CodeDefinedLiterals", "FSROptions_Quality", "Quality");
	case 2:
		return NSLOCTEXT("CodeDefinedLiterals", "FSROptions_Balanced", "Balanced");
	case 3:
		return NSLOCTEXT("CodeDefinedLiterals", "FSROptions_Performance", "Performance");
	case 4:
		return NSLOCTEXT("CodeDefinedLiterals", "FSROptions_UltraPerformance", "Ultra Performance");
	}
	return FText();
}

bool UBpVideoSettingsLib::SetDlssQuality(int32 Quality)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;
	// ##UE5UPGRADE## Re add this after DLSS upgraded       
	//if (!UDLSSLibrary::IsDLSSSupported())
	//	return false;
	
	Settings->DlssQualitySetting = Quality;
	SaveVideoModeAndQuality();
	return true;
}

bool UBpVideoSettingsLib::GetDlssQuality(int32& Quality)
{
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	// ##UE5UPGRADE## Re add this after DLSS upgraded       
	//Quality = UDLSSLibrary::IsDLSSSupported() ? Settings->DlssQualitySetting : 0;
	return true;
}

bool UBpVideoSettingsLib::SetDlssFrameGenerationSetting(int32 Setting)
{
	return false; // NOTE(killo): Disabled for Hotfix 3
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	// ##UE5UPGRADE## Re add this after DLSS upgraded       
	//if (!UStreamlineLibraryDLSSG::IsDLSSGSupported())
	//	return false;
	
	Settings->ExperimentalFrameGenerationSetting = Setting;
	SaveVideoModeAndQuality();
	return true;
}

bool UBpVideoSettingsLib::GetDlssFrameGenerationSetting(int32& Setting)
{
	return false; // NOTE(killo): Disabled for Hotfix 3
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	// ##UE5UPGRADE## Re add this after DLSS upgraded       
	//Setting = UStreamlineLibraryDLSSG::IsDLSSGSupported() ? Settings->ExperimentalFrameGenerationSetting : 0;
	return true;
}

bool UBpVideoSettingsLib::SetFSRQuality(int32 Quality)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (!Settings)
		return false;

	Settings->FSRQualitySetting = Quality;
	SaveVideoModeAndQuality();
	return true;
}

bool UBpVideoSettingsLib::GetFSRQuality(int32& Quality)
{
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (!Settings)
		return false;

	Quality = Settings->FSRQualitySetting;
	return true;
}

bool UBpVideoSettingsLib::SetOverallVideoQuality(int32 Quality)
{
	UGameUserSettings* Settings = GetGameUserSettings();
	if (!Settings)
		return false;

	Quality = FMath::Clamp(Quality, 0, 3);

	int32 AntiAliasing, Effects, PostProcess, Shadow, Texture, ViewDistance;
	float ResolutionScaling;
	GetVideoQualitySettings(AntiAliasing, Effects, PostProcess, ResolutionScaling, Shadow, Texture, ViewDistance);
	SetVideoQualitySettings(Quality, Quality, Quality, ResolutionScaling, Quality, Quality, Quality);
	return true;
}

bool UBpVideoSettingsLib::GetOverallVideoQuality(int32& Quality)
{
	UGameUserSettings* Settings = GetGameUserSettings();
	if (!Settings)
		return false;


	Quality = Settings->GetOverallScalabilityLevel();
	Quality = FMath::Clamp(Quality, 0, 3);
	return true;
}

bool UBpVideoSettingsLib::SetVideoQualitySettings(const int32 AntiAliasing, const int32 Effects, const int32 PostProcess, const float ResolutionScaling, const int32 Shadow, const int32 Texture, const int32 ViewDistance)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	// store 'ron' variables just to fake the UI
	Settings->ScalabilityQuality.AntiAliasingQuality = AntiAliasing;
	Settings->AntiAliasingQuality = AntiAliasing;
	Settings->ScalabilityQuality.EffectsQuality = Effects;
	Settings->EffectsQuality = Effects;
	Settings->ScalabilityQuality.PostProcessQuality = PostProcess;
	Settings->PostProcessQuality =  PostProcess;
	Settings->ScalabilityQuality.ResolutionQuality = ResolutionScaling * 100.0f;
	Settings->ScalabilityQuality.ShadowQuality = Shadow;
	Settings->ShadowQuality = Shadow;
	Settings->ScalabilityQuality.TextureQuality = Texture;
	Settings->TextureQuality = Texture;
	Settings->ScalabilityQuality.ViewDistanceQuality = ViewDistance;
	Settings->ViewDistanceQuality = ViewDistance;
	Settings->SaveConfig();
	return true;
}


int32 UBpVideoSettingsLib::GetGraphicsPresetIndex()
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return 0;

	return Settings->GraphicsPresetIndex;
}



bool UBpVideoSettingsLib::SetGraphicsPresetIndex(const int32 GraphicsPresetIndex)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GetGameUserSettings());
	if (!Settings)
		return false;

	Settings->GraphicsPresetIndex = GraphicsPresetIndex;
	return true;
}


bool UBpVideoSettingsLib::SaveVideoModeAndQuality()
{
	UGameUserSettings* Settings = GetGameUserSettings();
	if (!Settings)
	{
		return false;
	}

	Settings->ApplySettings(true);
	return true;
}

void UBpVideoSettingsLib::ForceReloadSettings()
{
	UGameUserSettings* Settings = GetGameUserSettings();
	if (!Settings)
		return;

	Settings->LoadSettings(false);
}

bool UBpVideoSettingsLib::SetVSyncEnabled(bool bEnabled)
{
	UGameUserSettings* Settings = GetGameUserSettings();
	if (!Settings)
		return false;

	Settings->SetVSyncEnabled(bEnabled);
	SaveVideoModeAndQuality();
	return true;
}

bool UBpVideoSettingsLib::GetVSyncEnabled(bool& bEnabled)
{
	UGameUserSettings* Settings = GetGameUserSettings();
	if (!Settings)
		return false;
	
	bEnabled = Settings->IsVSyncEnabled();
	return true;
}

bool UBpVideoSettingsLib::SetWorldDecalEnabled(bool bEnabled, float FadeDistance, float Density)
{
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (Settings)
	{
		Settings->bWorldDecalsEnabled = bEnabled;
		Settings->WorldDecalScreenFadeSize = FadeDistance;
		Settings->WorldDecalDensity = Density;
		return true;
	}
	return false;
}

bool UBpVideoSettingsLib::GetWorldDecalsEnabled(bool& bEnabled, float& FadeDistance, float& Density)
{
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (Settings)
	{
		bEnabled = Settings->bWorldDecalsEnabled;
		FadeDistance = Settings->WorldDecalScreenFadeSize;
		Density = Settings->WorldDecalDensity;
		return true;
	}
	return false;
}

bool UBpVideoSettingsLib::SetMotionBlurEnabled(bool bEnabled)
{
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (Settings)
	{
		Settings->bMotionBlur = bEnabled;
		return true;
	}
	return false;
}

bool UBpVideoSettingsLib::GetMotionBlurEnabled(bool& bEnabled)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (Settings)
	{
		bEnabled = Settings->bMotionBlur;
		return true;
	}
	return false;
}

bool UBpVideoSettingsLib::SetMotionBlurStrength(float Strength)
{
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (Settings)
	{
		Settings->MotionBlurStrength = Strength;
		return true;
	}
	return false;
}

bool UBpVideoSettingsLib::GetMotionBlurStrength(float& Strength)
{
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (Settings)
	{
		Strength = Settings->MotionBlurStrength;
		return true;
	}
	return false;
}

bool UBpVideoSettingsLib::SetFrameRateLimit(int32 FrameRateLimit, bool bEnabled)
{
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (!Settings)
		return false;

	Settings->SetFrameRateLimit(bEnabled ? FrameRateLimit : 0.0f);
	Settings->bFrameLimitEnabled = bEnabled;
	SaveVideoModeAndQuality();
	return true;
}

bool UBpVideoSettingsLib::GetFrameRateLimit(int32& FrameRateLimit, bool& bEnabled)
{
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (!Settings)
		return false;

	FrameRateLimit = Settings->GetFrameRateLimit();
	bEnabled = Settings->bFrameLimitEnabled;
	return true;
}

bool UBpVideoSettingsLib::SetRaytracingSettings(bool bRTXEnabled, bool bRTXReflectionsEnabled, bool bRTXShadowsEnabled, bool bRTXAmbientOcclusionEnabled, bool bRTXGlobalIllumination, bool bRTXTranslucency)
{
	return false;
	UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (!Settings)
		return false;

	Settings->bRayTracingEnabled = bRTXEnabled;
	Settings->bRayTracingReflectionsEnabled = bRTXReflectionsEnabled;
	Settings->bRayTracingShadowsEnabled = bRTXShadowsEnabled;
	Settings->bRayTracingAmbientOcclusionEnabled = bRTXAmbientOcclusionEnabled;
	Settings->bRTXGlobalIllumination = bRTXGlobalIllumination;
	Settings->bRTXTranslucency = bRTXTranslucency;
	return true;
}

bool UBpVideoSettingsLib::GetRaytracingSettings(bool& bRTXEnabled, bool& bRTXReflectionsEnabled, bool& bRTXShadowsEnabled, bool& bRTXAmbientOcclusionEnabled, bool& bRTXGlobalIllumination, bool& bRTXTranslucency)
{
	// TODO: temp
	bRTXEnabled = false;
	bRTXReflectionsEnabled = false;
	bRTXTranslucency = false;
	bRTXAmbientOcclusionEnabled = false;
	bRTXGlobalIllumination = false;
	bRTXShadowsEnabled = false;
	return false;
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (!Settings)
		return false;

	bRTXEnabled = Settings->bRayTracingEnabled;
	bRTXReflectionsEnabled = Settings->bRayTracingReflectionsEnabled;
	bRTXShadowsEnabled = Settings->bRayTracingShadowsEnabled;
	bRTXAmbientOcclusionEnabled = Settings->bRayTracingAmbientOcclusionEnabled;
	bRTXGlobalIllumination = Settings->bRTXGlobalIllumination;
	bRTXTranslucency = Settings->bRTXTranslucency;
	return false;
}

bool UBpVideoSettingsLib::SupportsRayTracing()
{
	return IsRayTracingEnabled();
}


bool UBpVideoSettingsLib::HasRTXCard()
{
	return GetGPUList().Contains("RTX");
}

bool UBpVideoSettingsLib::IsNvidiaReflexEnabled()
{
#if defined(WITH_REFLEX) && defined(NVIDIA_REFLEX_ENABLED) && defined(RONENGINE)
	return UReflexBlueprintLibrary::GetReflexAvailable();
#endif
	return false;
}

bool UBpVideoSettingsLib::SetReflexEnabled(uint8 ReflexMode, bool bFlashIndicatorEnabled)
{
#if defined(WITH_REFLEX) && defined(NVIDIA_REFLEX_ENABLED) && defined(RONENGINE)
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (!Settings)
		return false;

	Settings->ReflexMode = ReflexMode;
	Settings->bReflexFlashIndicatorEnabled = bFlashIndicatorEnabled;
	SaveVideoModeAndQuality();
	return true;
#endif
	return false;
}

bool UBpVideoSettingsLib::GetReflexEnabled(uint8& ReflexMode, bool& bFlashIndicatorEnabled)
{
#if defined(WITH_REFLEX) && defined(NVIDIA_REFLEX_ENABLED) && defined(RONENGINE)
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (!Settings)
		return false;

	ReflexMode = Settings->ReflexMode;
	bFlashIndicatorEnabled = Settings->bReflexFlashIndicatorEnabled;
	return true;
#endif

	return false;
}

bool UBpVideoSettingsLib::SetReflexLatencyOptions(bool bGameToRenderLatencyEnabled, bool bGameLatencyEnabled,
	bool bRenderLatencyEnabled)
{
#if defined(WITH_REFLEX) && defined(NVIDIA_REFLEX_ENABLED) && defined(RONENGINE)
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (!Settings)
		return false;

	Settings->bReflexGameToRenderLatencyInMSEnabled = bGameToRenderLatencyEnabled;
	Settings->bReflexGameLatencyInMSEnabled = bGameLatencyEnabled;
	Settings->bReflexRenderLatencyInMSEnabled = bRenderLatencyEnabled;
	SaveVideoModeAndQuality();
	return true;
#endif

	return false;
}

bool UBpVideoSettingsLib::GetReflexLatencyOptions(bool& bGameToRenderLatencyEnabled, bool& bGameLatencyEnabled,
	bool& bRenderLatencyEnabled)
{
#if defined(WITH_REFLEX) && defined(NVIDIA_REFLEX_ENABLED) && defined(RONENGINE)
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (!Settings)
		return false;

	bGameToRenderLatencyEnabled = Settings->bReflexGameToRenderLatencyInMSEnabled;
	bGameLatencyEnabled = Settings->bReflexGameLatencyInMSEnabled;
	bRenderLatencyEnabled = Settings->bReflexRenderLatencyInMSEnabled;
	return true;
#endif
	return false;
}

void UBpVideoSettingsLib::GetReflexLatency(bool& bGameToRenderLatencyEnabled, float& GametoRenderLatency,
	bool& bGameLatencyEnabled, float& GameLatencyInMS, bool& bRenderLatencyEnabled, float& RenderLatencyInMS)
{
#if defined(WITH_REFLEX) && defined(NVIDIA_REFLEX_ENABLED) && defined(RONENGINE)
	UReadyOrNotGameUserSettings* Settings  = Cast<UReadyOrNotGameUserSettings>( GetGameUserSettings());
	if (!Settings)
		return;
	
	bGameToRenderLatencyEnabled = UReflexBlueprintLibrary::GetReflexAvailable() && Settings->bReflexGameToRenderLatencyInMSEnabled;
	GametoRenderLatency = UReflexBlueprintLibrary::GetGameToRenderLatencyInMs();
	bGameLatencyEnabled = UReflexBlueprintLibrary::GetReflexAvailable() && Settings->bReflexGameLatencyInMSEnabled;
	GameLatencyInMS = UReflexBlueprintLibrary::GetGameLatencyInMs();
	bRenderLatencyEnabled = UReflexBlueprintLibrary::GetReflexAvailable() && Settings->bReflexRenderLatencyInMSEnabled;
	RenderLatencyInMS = UReflexBlueprintLibrary::GetRenderLatencyInMs();
#endif
}
FString UBpVideoSettingsLib::GetGPUList()
{
#if PLATFORM_WINDOWS
	static FString PrimaryGPUBrand;
	if (PrimaryGPUBrand.IsEmpty())
	{
		// Find primary display adapter and get the device name.
		PrimaryGPUBrand = FGenericPlatformMisc::GetPrimaryGPUBrand();

		DISPLAY_DEVICE DisplayDevice;
		DisplayDevice.cb = sizeof(DisplayDevice);
		DWORD DeviceIndex = 0;

		while (EnumDisplayDevices(0, DeviceIndex, &DisplayDevice, 0))
		{

			PrimaryGPUBrand += DisplayDevice.DeviceString;
			PrimaryGPUBrand += "\r\n";

			FMemory::Memzero(DisplayDevice);
			DisplayDevice.cb = sizeof(DisplayDevice);
			DeviceIndex++;
		}
	}
	return PrimaryGPUBrand;
#endif
	return "";
}

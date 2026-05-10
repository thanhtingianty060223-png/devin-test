// Copyright Void Interactive, 2024

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "BpVideoSettingsLib.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UBpVideoSettingsLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static UGameUserSettings* GetGameUserSettings();

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetSupportedScreenResolutions(TArray<FString>& Resolutions);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static FString GetCurrentScreenResolution();

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static EWindowMode::Type GetCurrentScreenMode();
	
	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SetScreenResolution(const int32 Width, const int32 Height, EWindowMode::Type NewWindowMode);

	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool ChangeScreenResolution(const int32 Width, const int32 Height, EWindowMode::Type NewWindowMode);

	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool GetInterfaceAspectRatio(float& OutAspectRatio);

	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SetInterfaceAspectRatio(float InAspectRatio);
	
	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetVideoQualitySettings(int32& AntiAliasing, int32& Effects, int32& PostProcess, float& ResolutionScaling, int32& Shadow, int32& Texture, int32& ViewDistance);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetVideoQualitySettingsAsString(FText& OverallSetting, FText& AntiAliasing, FText& Effects, FText& PostProcess, FText& Shadow, FText& Texture, FText& ViewDistance);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetAntiAliasingQuality(int32& AntiAliasing);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetEffectsQuality(int32& Effects);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetPostProcessQuality(int32& PostProcess);
	
	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetResolutionScaling(float& ResolutionScaling);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetShadowQuality(int32& Shadow);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetTextureQuality(int32& Texture);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetViewDistanceQuality(int32& ViewDistance);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetPerObjectShadowsEnabled(bool& bPerObjectShadowsEnabled);

	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SetPerObjectShadowsEnabled(bool bPerObjectShadowsEnabled);
	
	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetDepthOfFieldSetting(bool& bDoFEnabled);

	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SetDepthofFieldSetting(bool bDoFEnabled);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static FText GenerateQualityString(int32 Quality);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static FText GenerateDlssQualityString(int32 Quality);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static FText GenerateFSRQualityString(int32 Quality);

	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SetDlssQuality(int32 Quality);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetDlssQuality(int32& Quality);

	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SetDlssFrameGenerationSetting(int32 Setting);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetDlssFrameGenerationSetting(int32& Setting);
	
	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SetFSRQuality(int32 Quality);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetFSRQuality(int32& Quality);
	
	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SetOverallVideoQuality(int32 Quality);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetOverallVideoQuality(int32& Quality);

	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SetVideoQualitySettings(const int32 AntiAliasing, const int32 Effects, const int32 PostProcess, const float ResolutionScaling, const int32 Shadow, const int32 Texture, const int32 ViewDistance);

	UFUNCTION(BlueprintCallable, Category= "Video Settings")
	static int32 GetGraphicsPresetIndex();
	
	UFUNCTION(BlueprintCallable, Category="Video Settings")
	static bool SetGraphicsPresetIndex(const int32 GraphicsPresetIndex);

	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SaveVideoModeAndQuality();

	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static void ForceReloadSettings();

	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SetVSyncEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetVSyncEnabled(bool& bEnabled);
	
	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SetWorldDecalEnabled(bool bEnabled, float FadeDistance, float Density);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetWorldDecalsEnabled(bool& bEnabled, float& FadeDistance, float& Density);

	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SetMotionBlurEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetMotionBlurEnabled(bool& bEnabled);
	
	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SetMotionBlurStrength(float Strength);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetMotionBlurStrength(float& Strength);

	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SetFrameRateLimit(int32 FrameRateLimit, bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Video Settings")
	static bool GetFrameRateLimit(int32& FrameRateLimit, bool& bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Video Settings | Raytracing")
	static bool SetRaytracingSettings(bool bRTXEnabled, bool bRTXReflectionsEnabled, bool bRTXShadowsEnabled, bool bRTXAmbientOcclusionEnabled, bool bRTXGlobalIllumination, bool bRTXTranslucency);

	UFUNCTION(BlueprintPure, Category = "Video Settings | Raytracing")
	static bool GetRaytracingSettings(bool& bRTXEnabled, bool& bRTXReflectionsEnabled, bool& bRTXShadowsEnabled, bool& bRTXAmbientOcclusionEnabled, bool& bRTXGlobalIllumination, bool& bRTXTranslucency);

	UFUNCTION(BlueprintCallable, Category = "Video Settings")
	static bool SupportsRayTracing();

	static bool HasRTXCard();

	UFUNCTION(BlueprintCallable, Category = "Nvidia Reflex")
	static bool IsNvidiaReflexEnabled();

	UFUNCTION(BlueprintCallable, Category = "Nvidia Reflex")
	static bool SetReflexEnabled(uint8 ReflexMode, bool bFlashIndicatorEnabled);

	UFUNCTION(BlueprintPure, Category = "Nvidia Reflex")
	static bool GetReflexEnabled(uint8& ReflexMode, bool& bFlashIndicatorEnabled);

	UFUNCTION(BlueprintCallable)
	static bool SetReflexLatencyOptions(bool bGameToRenderLatencyEnabled, bool bGameLatencyEnabled, bool bRenderLatencyEnabled);
	
	UFUNCTION(BlueprintPure)
	static bool GetReflexLatencyOptions(bool& bGameToRenderLatencyEnabled, bool& bGameLatencyEnabled, bool& bRenderLatencyEnabled);

	UFUNCTION(BlueprintPure, Category = "Nvidia Reflex")
	static void GetReflexLatency(bool& bGameToRenderLatencyEnabled, float& GametoRenderLatency, bool& bGameLatencyEnabled, float& GameLatencyInMS, bool& bRenderLatencyEnabled, float& RenderLatencyInMS);

	static FString GetGPUList();
};

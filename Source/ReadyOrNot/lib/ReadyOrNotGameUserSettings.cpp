// Copyright Void Interactive, 2024

#include "ReadyOrNotGameUserSettings.h"
#include "ReadyOrNot.h"
#include "Subsystems/SubtitlesSubsystem.h"
#if defined(WITH_REFLEX) && defined(RONENGINE)
#include "ReflexBlueprint.h"
#endif
// #include "StreamlineLibraryDLSSG.h"

void UReadyOrNotGameUserSettings::SaveSettings()
{
	Super::SaveSettings();
	OnSettingsSaved.Broadcast();
}

void UReadyOrNotGameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
 	Super::ApplySettings(bCheckForCommandLineOverrides);
	
	ApplyCommandLineOverrides();
	USubtitlesStatics::SetLocale(UBpGameplayHelperLib::GetWorldStatic(), SubtitlesLocale);
}

void UReadyOrNotGameUserSettings::ApplyCommandLineOverrides()
{
	if (GEngine->GameViewport)
	{
		UWorld* WorldContext = GEngine->GameViewport->GetWorld();
		if (WorldContext)
		{
			FString ExecRenderTargetPool = "r.RenderTargetPoolMin 2000";
			GEngine->Exec(WorldContext, *ExecRenderTargetPool);
			FString ViewDistanceQualityStr = "sg.ViewDistanceQuality " + FString::FromInt(FMath::Clamp(ScalabilityQuality.ViewDistanceQuality, 2, 4));
			GEngine->Exec(WorldContext, *ViewDistanceQualityStr);
			// oddly enough shadow quality 0 seems to TANK performance hard
			FString ShadowQualityStr = "sg.ShadowQuality " + FString::FromInt(FMath::Clamp(ScalabilityQuality.ShadowQuality, 1, 4));
			GEngine->Exec(WorldContext, *ShadowQualityStr);
			// VFX at 0 causes crash
			FString EffectsQualityStr = "sg.EffectsQuality " + FString::FromInt(FMath::Clamp(ScalabilityQuality.EffectsQuality, 1, 4));
			GEngine->Exec(WorldContext, *EffectsQualityStr);
			FString DepthOfFieldConsoleCommand = "r.DepthOfFieldQuality 0";
			GEngine->Exec(WorldContext, *DepthOfFieldConsoleCommand);
			FString RTXReflectionsConsoleCommand = "r.RayTracing.Reflections " + FString::FromInt(bRayTracingEnabled && bRayTracingReflectionsEnabled ? 1 : 0);
			GEngine->Exec(WorldContext, *RTXReflectionsConsoleCommand);
			FString RTXShadowsConsoleCommand = "r.RayTracing.Shadows " + FString::FromInt(bRayTracingEnabled && bRayTracingShadowsEnabled ? 1 : 0);
			GEngine->Exec(WorldContext, *RTXShadowsConsoleCommand);
			FString RTXAmbientOcclusionCommand = "r.RayTracing.AmbientOcclusion  " + FString::FromInt(bRayTracingEnabled && bRayTracingAmbientOcclusionEnabled ? 1 : 0);
			GEngine->Exec(WorldContext, *RTXAmbientOcclusionCommand);
			FString RTXGlobalIllumination = "r.RayTracing.GlobalIllumination  " + FString::FromInt(bRayTracingEnabled && bRTXGlobalIllumination ? 1 : 0);
			GEngine->Exec(WorldContext, *RTXGlobalIllumination);
			FString RTXTranslucency = "r.RayTracing.Translucency 0";
			GEngine->Exec(WorldContext, *RTXTranslucency);

			FString PerObjectShadows = "r.Shadow.PerObject " + FString::FromInt(bEnablePerObjectShadows);
			GEngine->Exec(WorldContext, *PerObjectShadows);
			
			FString DlssEnabled = "r.NGX.DLSS.Enable 0";
			if (UReadyOrNotFunctionLibrary::IsDLSSEnabled())
			{
				if (DlssQualitySetting != 0)
				{
					DlssEnabled =  "r.NGX.DLSS.Enable 1";
				}

				bool bUseAuto = false;
				
				// In-Game Quality (0 = Off, 1 = Auto, 2 = UltraQuality, 3 = Quality, 4 = Balanced, 5 = Performance, 6 = UltraPerformance)
				int32 MappedQualityValue = 0;
				switch (DlssQualitySetting)
				{
				case 1:
					bUseAuto = true;
					break;
				case 2:
					MappedQualityValue = 2;
					break;
				case 3:
					MappedQualityValue = 1;
					break;
				case 4:
					MappedQualityValue = 0;
					break;
				case 5:
					MappedQualityValue = -1;
					break;
				case 6:
					MappedQualityValue = -2;
					break;
				}
				
				FString DlssQuality = "r.NGX.DLSS.Quality " +  FString::FromInt(MappedQualityValue);
				GEngine->Exec(WorldContext, *DlssQuality);

				FString DlssAuto = "r.NGX.DLSS.Quality.Auto " + FString::FromInt(bUseAuto);
				GEngine->Exec(WorldContext, *DlssAuto);
			}
			GEngine->Exec(WorldContext, *DlssEnabled);

			// DLSS Frame Generation
			{
				// In-Game Setting (0 = Off, 1 = On, 2 = Auto)
				// int32 DlssgEnable = UStreamlineLibraryDLSSG::IsDLSSGSupported() ? ExperimentalFrameGenerationSetting : 0;
				int32 DlssgEnable = 0;
				
				FString FrameGenerationMode = "r.Streamline.DLSSG.Enable " + FString::FromInt(DlssgEnable);
				GEngine->Exec(WorldContext, *FrameGenerationMode);

				if (DlssgEnable > 0)
				{
					// Nvidia recommends vsync is disabled while frame generation is enabled
					GEngine->Exec(WorldContext, TEXT("r.vsync 0"));
				}
			}

#ifdef AMD_FSR_ENABLED
			if (DlssQualitySetting == 0 && FSRQualitySetting > 0)
			{
				FString EnableFSR = "r.FidelityFX.FSR2.Enabled 1";
				GEngine->Exec(WorldContext, *EnableFSR);
				
				FString FSRQuality = "r.FidelityFX.FSR2.QualityMode " + FString::FromInt(FSRQualitySetting);
				GEngine->Exec(WorldContext, *FSRQuality);

				// FSR2 requires temporal upsampling enabled
				FString TemporalUpsampling = "r.TemporalAA.Upsampling 1";
				GEngine->Exec(WorldContext, *TemporalUpsampling);
			}
			else
			{
				FString DisableFSR = "r.FidelityFX.FSR2.Enabled 0";
				GEngine->Exec(WorldContext, *DisableFSR);

				// Temporal upsampling is normally disabled, so keep it off if FSR2 is disabled
				FString TemporalUpsampling = "r.TemporalAA.Upsampling 0";
				GEngine->Exec(WorldContext, *TemporalUpsampling);
			}
#endif
			
#if defined(WITH_REFLEX) && defined(NVIDIA_REFLEX_ENABLED) && defined(RONENGINE)
			switch(ReflexMode)
			{
				case 0:
				    UReflexBlueprintLibrary::SetReflexMode(EReflexMode::Disabled);
				break;
				case 1:
					UReflexBlueprintLibrary::SetReflexMode(EReflexMode::Enabled);
				break;
				case 3:
					UReflexBlueprintLibrary::SetReflexMode(EReflexMode::EnabledPlusBoost);
				break;
			}
#endif

			if (DlssQualitySetting == 0 && FSRQualitySetting == 0)
			{
				FString ScreenRes = "r.ScreenPercentage " + FString::SanitizeFloat(ScalabilityQuality.ResolutionQuality);
				GEngine->Exec(WorldContext, *ScreenRes);
			}

			FString EyeAdaptionCommand = "r.EyeAdaptationQuality 4";
			GEngine->Exec(WorldContext, *EyeAdaptionCommand);
			
			FString TemporalFrameWeight = "r.TemporalAACurrentFrameWeight 0.2";
			GEngine->Exec(WorldContext, *TemporalFrameWeight);
			FString TemporalAASamples = "r.TemporalAASamples 4";
			GEngine->Exec(WorldContext, *TemporalAASamples);
			FString ToneMapperSharpen = "r.Tonemapper.Sharpen 0.8";
			GEngine->Exec(WorldContext, *ToneMapperSharpen);
#if !WITH_EDITOR
			RequestResolutionChange(ResolutionSizeX, ResolutionSizeY, GetFullscreenMode(), false);
			SetFullscreenMode(GetFullscreenMode());
			ConfirmVideoMode();
#endif

			UFMODBlueprintStatics::SetGlobalParameterByName("Music_Verb_Volume", MusicSoundVolume);
		}
	}
}

void UReadyOrNotGameUserSettings::ResetKeybinds()
{
	if (UInputSettings* InputSettings = UInputSettings::GetInputSettings())
	{
		// Clear out all axis and action bindings
		TArray<FInputAxisKeyMapping> AxisMappings = InputSettings->GetAxisMappings();
		for (FInputAxisKeyMapping& mapping : AxisMappings)
		{
			InputSettings->RemoveAxisMapping(mapping);
		}
		
		TArray<FInputActionKeyMapping> ActionMappings = InputSettings->GetActionMappings();
		for (FInputActionKeyMapping& mapping : ActionMappings)
		{
			InputSettings->RemoveActionMapping(mapping);
		}

		// Save to config and rebuild the keymaps for all UPlayerInput objects
		// Note: this may be redundant, will need to verify before deleting these two lines
		InputSettings->SaveKeyMappings();
		InputSettings->ForceRebuildKeymaps();

		// Reload Input.ini
		FConfigCacheIni::LoadGlobalIniFile(GInputIni, TEXT("Input"), nullptr, true, false);
		// ##UE5UPGRADE##
		InputSettings->ReloadConfig(InputSettings->GetClass(), *GInputIni, UE::LCPF_PropagateToInstances);

		// Resave to config and rebuild keymaps again
		InputSettings->SaveKeyMappings();
		InputSettings->ForceRebuildKeymaps();
	}
}

void UReadyOrNotGameUserSettings::ResetGamepadControlsSettings()
{
	if (UInputSettings* InputSettings = UInputSettings::GetInputSettings())
	{
		GamepadLookSensitivity = 0.25f;
		GamepadAimSensitivity = 0.25f;
		bInvertGamepadVertical = false;
		bInvertGamepadHorizontal = false;
		bToggleADS = false;
		bHoldCrouch = false;
		bTogglePS5Gyro = false;
		bUsingAlternateControls = false;

		SaveSettings();
	}
}

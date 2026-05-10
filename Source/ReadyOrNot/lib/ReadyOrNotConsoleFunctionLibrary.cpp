// Copyright Void Interactive, 2022
#include "lib/ReadyOrNotConsoleFunctionLibrary.h"

#if defined(TARGET_PS4)
#include <system_service.h>
#include <kernel.h>
#include <video_out.h>

FString const GetRuntimeDeviceProfileName()
{
	static FString ProfileName;
	
	if (ProfileName.IsEmpty())
	{
		ProfileName = FPlatformProperties::PlatformName();

		if( sceKernelIsNeoMode() )
		{
			ProfileName = "PS4_Neo";

			SceVideoOutResolutionStatus ResolutionStatus;
			int Handle = sceVideoOutOpen( SCE_USER_SERVICE_USER_ID_SYSTEM, SCE_VIDEO_OUT_BUS_TYPE_MAIN, 0, NULL );
			if( Handle > 0 )
			{
				if( SCE_OK == sceVideoOutGetResolutionStatus( Handle, &ResolutionStatus ) && ResolutionStatus.fullHeight > 1080 )
				{
					// 4k Mode
					ProfileName = "PS4_Neo_4k";
				}
				sceVideoOutClose( Handle );
			}
		}

		UE_LOG( LogSony, Log, TEXT( "Selected Device Profile: [%s]" ), *ProfileName );
	}

	return ProfileName;
}
#endif

// Utility functions for console 

ERuntimeDevice UReadyOrNotConsoleFunctionLibrary::GetRuntimeDeviceProfile()
{
	FString ProfileName;
#if defined(TARGET_PS4)
	ProfileName = GetRuntimeDeviceProfileName();
	if (ProfileName == "PS4")
	{
		return ERuntimeDevice::PS4;
	}
	if (ProfileName == "PS4_Neo")
	{
		return ERuntimeDevice::PS4_Pro;
	}
	if (ProfileName == "PS4_Neo_4k")
	{
		return ERuntimeDevice::PS4_Pro_4K;
	}
	// fallback
	return ERuntimeDevice::PS4;
#elif defined(TARGET_PS5)
	return ERuntimeDevice::PS5;
#elif defined (TARGET_XB1)
	EXboxOneConsoleType ConsoleType = FPlatformMisc::GetConsoleType();
	if (ConsoleType == EXboxOneConsoleType::XboxOne)
	{
		return ERuntimeDevice::XBoxOne;
	}
	if (ConsoleType == EXboxOneConsoleType::XboxOneS)
	{
		return ERuntimeDevice::XBoxOneS;
	}
	if (ConsoleType == EXboxOneConsoleType::Scorpio)
	{
		return ERuntimeDevice::XBoxOneX;
	}
	// fallback
	return ERuntimeDevice::XBoxOne;
#elif defined(TARGET_XSX)
	EXSXConsoleType ConsoleType = FPlatformMisc::GetConsoleType();
	if (ConsoleType == EXSXConsoleType::Anaconda)
	{
		return ERuntimeDevice::XBoxSeriesX;
	}
	if (ConsoleType == EXSXConsoleType::Lockhart)
	{
		return ERuntimeDevice::XBoxSeriesS;
	}
	// fallback
	return ERuntimeDevice::XBoxSeriesS;
#else
	return ERuntimeDevice::PC;
#endif
}


void UReadyOrNotConsoleFunctionLibrary::ConsoleApplyLevelSpecificSettings(FString MapName, bool QualityOverFrameRate)
{
	const ERuntimeDevice RuntimeDevice = GetRuntimeDeviceProfile();
	
	UE_LOG(LogReadyOrNot, Warning, TEXT("Applying settings to map:%s specific settings"), *MapName);
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());

	AReadyOrNotPlayerController* Player = UReadyOrNotStatics::GetReadyOrNotPlayerController();
	Player->ConsoleCommand("r.DynamicRes.OperationMode 0");
	Player->ConsoleCommand("r.FidelityFX.FSR.Enabled 1");
	Player->ConsoleCommand("r.FidelityFX.FSR.UseFP16 1");
	Player->ConsoleCommand("r.ScreenPercentage 50");
	Player->ConsoleCommand("r.SSR.Quality 1");

	switch (RuntimeDevice)
	{
	case ERuntimeDevice::PS4:
	case ERuntimeDevice::PS4_Pro:
	case ERuntimeDevice::PS4_Pro_4K:
		{
			if (us)
			{
				us->SetScreenResolution(FIntPoint(1280, 720));
				us->ApplyResolutionSettings(false);
				Player->ConsoleCommand("r.Streaming.PoolSize 500");
				Player->ConsoleCommand("sg.ShadowQuality 2");
				Player->ConsoleCommand("sg.AntialiasingQuality 2");
			}
			break;
		}
	case ERuntimeDevice::XBoxOne:
		{
			Player->ConsoleCommand("r.Streaming.PoolSize 1000");
			Player->ConsoleCommand("sg.ShadowQuality 2");
			Player->ConsoleCommand("sg.AntialiasingQuality 2");
			break;
		}

	case ERuntimeDevice::XBoxOneS:
		{			
			Player->ConsoleCommand("r.Streaming.PoolSize 1000");
			Player->ConsoleCommand("sg.ShadowQuality 2");
			Player->ConsoleCommand("sg.AntialiasingQuality 2");
			break;
		}
	case ERuntimeDevice::XBoxOneX:
		{
			Player->ConsoleCommand("r.Streaming.PoolSize 2000");
			Player->ConsoleCommand("sg.ShadowQuality 2");
			Player->ConsoleCommand("sg.AntialiasingQuality 2");
			break;
		}
	case ERuntimeDevice::XBoxSeriesS:
		{
			Player->ConsoleCommand("r.Streaming.PoolSize 1200");
			Player->ConsoleCommand("sg.ShadowQuality 2");
			Player->ConsoleCommand("sg.AntialiasingQuality 2");
			Player->ConsoleCommand("r.SubsurfaceScattering 0");
			break;
		}
	case ERuntimeDevice::XBoxSeriesX:
		{
			Player->ConsoleCommand("r.Streaming.PoolSize 2000");
			break;
		}
	case ERuntimeDevice::PS5:
		{
			Player->ConsoleCommand("r.Streaming.PoolSize 2000");
			break;
		}
	default:
		break;
	}
}

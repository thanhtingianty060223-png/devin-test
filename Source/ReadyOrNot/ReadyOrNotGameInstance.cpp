// Copyright Void Interactive, 2024

#pragma once

#include "ReadyOrNotGameInstance.h"
#include "ReadyOrNotGameMode.h"

#include "NetworkReplayStreaming/NullNetworkReplayStreaming/Public/NullNetworkReplayStreaming.h"
#include "JsonUtilities/Public/JsonObjectConverter.h"

#include "lib/BpGameplayHelperLib.h"
#include "lib/ReadyOrNotFunctionLibrary.h"

#include "Engine/AssetManager.h"

#if !UE_BUILD_SHIPPING
#include "GameplayDebugger.h"
#endif

#include "HttpModule.h"

#if defined(WITH_STEAM)
#include "OnlineSubsystemSteam.h"
#endif

#include "Debug/GameplayDebuggerCategory_ReadyOrNot.h"
#include "Engine/DemoNetDriver.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Info/ReadyOrNotBackend.h"
#include "IPlatformFilePak.h"

//#if defined(WITH_VICODYNAMICS)
//#include "VICODynamicsPlugin/Public/VDMeshClothComponent.h"
//#endif

#include "Actors/Door.h"
#include "Components/InteractableComponent.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Info/HostMigrationManager.h"
#include "Info/ModioManager.h"
#include "Info/ScoringManager.h"
#include "Characters/ReplayCameraPawn.h"
// ##UE5UPGRADE## UrlRequest only used for inet_addr?
// #include "ThirdParty/UrlRequest.hpp"

//#if PLATFORM_WINDOWS
//#include <winsock2.h>
//#endif

#include "NavigationSystem.h"
#include "ReadyOrNotAIConfig.h"
#include "ReadyOrNotNavigationSystem.h"
#include "Actors/DestructibleVehicle.h"
#include "Actors/ThreatAwarenessActor.h"
#include "Actors/WorldDataGenerator.h"
#include "Actors/Environment/MissionPortal.h"
#include "Animation/UMGSequencePlayer.h"
#include "Commander/CampaignData.h"
#include "Commander/CommanderProfile.h"
#include "Commander/MetaGameProfile.h"
#include "GameModes/LobbyGM.h"
#include "Info/LoadoutManager.h"
#include "Internationalization/StringTableRegistry.h"
#include "Localization/ReadyOrNotStringTables.h"

#if defined(TARGET_PS5)
#include "Subsystems/PS5ActivitiesSubsystem.h"
#endif	

#if defined(TARGET_CONSOLE) || defined(WITH_EOS)
const FName EOSName(TEXT("EOS"));
#endif

#define VIVOX_VOICE_SERVER TEXT("https://mt2p.www.vivox.com/api2")
#define VIVOX_VOICE_DOMAIN TEXT("mt2p.vivox.com")
#define VIVOX_VOICE_ISSUER TEXT("voidin2477-re20")

bool UReadyOrNotGameInstance::bIsBuildPirated;
bool UReadyOrNotGameInstance::bIsModded;
bool UReadyOrNotGameInstance::bNoScoring;

bool AreVivoxVoiceChatValuesSet()
{
	FString CheckString("https://GETFROMPORTAL.www.vivox.com/api2");
	if (!CheckString.Compare(VIVOX_VOICE_SERVER)) return false;

	CheckString = FString("GET VALUE FROM VIVOX DEVELOPER PORTAL");
	if (!CheckString.Compare(VIVOX_VOICE_DOMAIN)) return false;
	if (!CheckString.Compare(VIVOX_VOICE_ISSUER)) return false;

	// NB: VIVOX_VOICE_KEY matters too, but this should be
	// sufficient to base creating the warning dialog on.

	return true;
}

#if ENABLE_ANTI_PIRACY_CHECKS
void UReadyOrNotGameInstance::CheckForSteamEmus()
{
	// we don't need to worry about any of this on linux
#if PLATFORM_WINDOWS
	/*if (IsInGameThread())
	{
		return;
	}
	
	// DLL hashes
	TArray<TArray<uint8>> Ddh;
	Ddh.Add({51, 48, 52, 97, 50, 95, 100, 49, 46, 51, 100, 50, 50, 49, 54, 99, 99, 49, 95, 99, 52, 100, 100, 54, 55, 50, 98, 53, 52, 51, 55, 53}); // steam_api.dll [ALI213 emu v6.6.99.59] (526c4af305f4438ee3ae6ff894d76597)
	Ddh.Add({98, 98, 50, 99, 46, 48, 50, 96, 100, 48, 46, 95, 95, 99, 51, 53, 95, 55, 50, 52, 99, 53, 100, 53, 46, 49, 51, 99, 96, 52, 100, 53}); // steam_api64.dll [ALI213 emu v6.6.99.59] (dd4e024bf20aae57a946e7f7035eb6f7)

	Ddh.Add({97, 98, 48, 95, 55, 54, 52, 52, 51, 52, 54, 100, 48, 100, 48, 99, 95, 97, 96, 46, 98, 55, 48, 55, 46, 48, 100, 95, 52, 96, 96, 47}); // steam_api.dll [Goldberg Emulator v0.2.5] (cd2a9866568f2f2eacb0d92902fa6bb1)
	Ddh.Add({99, 48, 55, 47, 49, 49, 95, 55, 49, 100, 98, 99, 95, 52, 98, 49, 55, 51, 95, 95, 47, 49, 47, 99, 97, 100, 50, 96, 100, 53, 96, 95}); // steam_api64.dll [Goldberg Emulator v0.2.5] (e29133a93fdea6d395aa131ecf4bf7ba)
	Ddh.Add({49, 47, 53, 55, 49, 99, 54, 95, 49, 54, 95, 49, 54, 46, 48, 96, 50, 95, 50, 96, 50, 48, 52, 95, 51, 51, 48, 53, 95, 49, 53, 55}); // steam_api.dll [Goldberg Emulator v0.2.4] (31793e8a38a3802b4a4b426a5527a379)
	Ddh.Add({53, 55, 50, 98, 100, 98, 51, 98, 98, 54, 48, 49, 52, 48, 97, 51, 52, 54, 54, 54, 53, 55, 48, 55, 53, 49, 98, 51, 96, 49, 50, 49}); // steam_api64.dll [Goldberg Emulator v0.2.4] (794dfd5dd82362c56888792973d5b343)
	Ddh.Add({51, 95, 96, 55, 47, 100, 48, 49, 49, 95, 55, 48, 53, 51, 52, 50, 47, 52, 99, 53, 49, 54, 50, 47, 99, 55, 98, 47, 54, 50, 51, 54}); // steam_api.dll [Goldberg Emulator v0.2.3] (5ab91f233a92756416e73841e9d18458)
	Ddh.Add({100, 95, 95, 98, 98, 54, 96, 97, 99, 50, 99, 100, 52, 50, 48, 49, 47, 97, 54, 100, 99, 96, 100, 48, 100, 54, 98, 96, 54, 49, 100, 55}); // steam_api64.dll [Goldberg Emulator v0.2.3] (faadd8bce4ef64231c8febf2f8db83f9)
	Ddh.Add({55, 49, 52, 46, 98, 51, 55, 50, 48, 53, 55, 47, 48, 97, 99, 53, 52, 95, 97, 99, 54, 53, 52, 53, 100, 97, 50, 47, 55, 52, 95, 48}); // steam_api.dll [Goldberg Emulator v0.2.2] (9360d59427912ce76ace8767fc4196a2)
	Ddh.Add({98, 48, 55, 54, 52, 54, 51, 95, 47, 98, 98, 46, 52, 98, 54, 100, 51, 96, 99, 99, 48, 51, 48, 46, 98, 51, 99, 48, 98, 51, 96, 95}); // steam_api64.dll [Goldberg Emulator v0.2.2] (d298685a1dd06d8f5bee2520d5e2d5ba)
	Ddh.Add({97, 97, 48, 55, 51, 96, 100, 47, 53, 48, 48, 50, 54, 51, 46, 98, 95, 50, 98, 53, 98, 49, 48, 54, 49, 50, 97, 46, 53, 99, 55, 96}); // steam_api.dll [Goldberg Emulator] (cc295bf17224850da4d7d32834c07e9b)
	Ddh.Add({48, 46, 97, 52, 55, 100, 100, 55, 54, 97, 51, 47, 49, 98, 52, 96, 46, 49, 53, 52, 99, 95, 51, 97, 48, 46, 55, 54, 52, 53, 50, 97}); // steam_api64.dll [Goldberg Emulator] (20c69ff98c513d6b0376ea5c2098674c)

	Ddh.Add({47, 54, 95, 50, 99, 47, 96, 96, 47, 98, 53, 98, 50, 49, 53, 49, 49, 49, 48, 55, 99, 98, 100, 50, 48, 97, 54, 95, 99, 55, 52, 48}); // steam_api64.dll [Goldberg Emulator] (18a4e1bb1d7d43733329edf42c8ae962)

	Ddh.Add({48, 100, 50, 99, 47, 54, 99, 100, 48, 99, 50, 95, 50, 47, 55, 95, 48, 98, 99, 96, 98, 96, 55, 100, 98, 100, 49, 52, 48, 96, 52, 49}); // steam_api.dll [Goldberg Emulator] (2f4e18ef2e4a419a2debdb9fdf362b63)

	Ddh.Add({50, 50, 96, 53, 53, 98, 95, 52, 54, 96, 96, 48, 96, 47, 53, 53, 51, 97, 47, 47, 97, 49, 55, 46, 52, 55, 53, 47, 53, 55, 51, 96}); // OnlineFix64.dll (44b77da68bb2b1775c11c3906971795b)
	Ddh.Add({95, 98, 55, 52, 54, 51, 98, 49, 50, 97, 53, 96, 55, 50, 55, 52, 52, 100, 46, 55, 99, 97, 48, 47, 51, 97, 98, 54, 52, 54, 49, 97}); // StubDRM64.dll (ad9685d34c7b94966f09ec215cd8683c)

	Ddh.Add({99, 97, 51, 95, 53, 96, 49, 99, 51, 50, 48, 46, 95, 55, 98, 55, 46, 97, 96, 99, 100, 49, 95, 98, 96, 48, 46, 49, 55, 47, 99, 47}); // ReadyOrNot[unknowncheats.me].dll (ec5a7b3e5420a9d90cbef3adb20391e1)

	#if !UE_BUILD_SHIPPING
	V_LOGM(LogReadyOrNot, "Checking Blacklist DLL Hashes");
	#endif

	// Find the steam_api(64).dll and test against the blacklisted hashes
	IFileManager& FileManager = IFileManager::Get();
	//TArray<uint8> Bb = {64, 103, 108, 95, 112, 103, 99, 113, 45}; // Binaries/
	//FString Bs;
	//ByteArrayToString_Offset(Bb.GetData(), Bb.Num(), -2, Bs);
	//const FString Root = "D:/Steam/steamapps/common/Ready Or Not/Engine/Binaries/";
	const FString Root = FPaths::ConvertRelativePathToFull(FPaths::EngineDir()) /*+ Bs#1#;
	FileManager.IterateDirectoryRecursively(*Root, [&](const TCHAR* FilenameOrDirectory, bool bIsDirectory)
	{
		TArray<uint8> Dlbs = {44, 98, 106, 106}; // .dll
		FString Dls;
		ByteArrayToString_Offset(Dlbs.GetData(), Dlbs.Num(), -2, Dls);
		
		if (!bIsDirectory && FString(FilenameOrDirectory).Contains(Dls))
		{
			const FMD5Hash H = FMD5Hash::HashFile(FilenameOrDirectory);
			const FString Dhs = LexToString(H);
			
			#if WITH_EDITOR
			//ULog::Info(FilenameOrDirectory);
			//ULog::Info(Dhs);
			#endif

			for (TArray<uint8>& D : Ddh)
			{
				FString Ds;
				ByteArrayToString_Offset(D.GetData(), D.Num(), -2, Ds);
				
				if (!bWantsRunPThread)
					return true;
				
				if (Dhs == Ds)
				{
					bIsBuildPirated = true;
					
					#if !WITH_EDITOR
					AActor* MyCoolAsNullReferencePtr = nullptr;
					MyCoolAsNullReferencePtr->SetActorLocation(FVector(0));
					#endif
				}
			}
			
			#if !WITH_EDITOR
			#if !UE_BUILD_SHIPPING
			V_LOGM(LogReadyOrNot, "Checking Blacklist DLL Hashes %d", Hashes.BLDLLHZ.Num());
			#endif

			if (!bWantsRunPThread)
				return true;

			for (FString D : Hashes.BLDLLHZ)
			{
				FString Psd; 
				FBase64::Decode(D, Psd);
				if (!bWantsRunPThread)
					return true;
				
				if (Dhs == Psd)
				{
					bIsBuildPirated = true;
					
					#if !WITH_EDITOR
					AActor* MyCoolAsNullReferencePtr = nullptr;
					MyCoolAsNullReferencePtr->SetActorLocation(FVector(0));
					#endif
				}
			}
			#endif
		}
		
		return true;
	});
	
	// Program exes
	{
		TArray<TArray<uint8>> Pp;
		Pp.Add({81, 107, 95, 112, 114, 81, 114, 99, 95, 107, 74, 109, 95, 98, 99, 112, 44, 99, 118, 99}); // SmartSteamLoader.exe
		Pp.Add({81, 107, 95, 112, 114, 81, 114, 99, 95, 107, 74, 109, 95, 98, 99, 112, 93, 118, 52, 50, 44, 99, 118, 99}); // SmartSteamLoader_x64.exe
		Pp.Add({81, 81, 67, 74, 95, 115, 108, 97, 102, 99, 112, 44, 99, 118, 99}); // SSELauncher.exe
		Pp.Add({65, 102, 99, 95, 114, 30, 67, 108, 101, 103, 108, 99, 44, 99, 118, 99}); // Cheat Engine.exe
		Pp.Add({97, 102, 99, 95, 114, 99, 108, 101, 103, 108, 99, 43, 103, 49, 54, 52, 44, 99, 118, 99}); // cheatengine-i386.exe
		Pp.Add({97, 102, 99, 95, 114, 99, 108, 101, 103, 108, 99, 43, 118, 54, 52, 93, 52, 50, 44, 99, 118, 99}); // cheatengine-x86_64.exe
		Pp.Add({97, 102, 99, 95, 114, 99, 108, 101, 103, 108, 99, 43, 118, 54, 52, 93, 52, 50, 43, 81, 81, 67, 50, 43, 63, 84, 86, 48, 44, 99, 118, 99}); // cheatengine-x86_64-SSE4-AVX2.exe
		Pp.Add({74, 115, 107, 95, 69, 95, 107, 99, 74, 95, 115, 108, 97, 102, 99, 112, 93, 118, 52, 50, 44, 99, 118, 99}); // LumaGameLauncher_x64.exe v1.9.7
		Pp.Add({74, 115, 107, 95, 69, 95, 107, 99, 74, 95, 115, 108, 97, 102, 99, 112, 93, 118, 54, 52, 44, 99, 118, 99}); // LumaGameLauncher_x86.exe v1.9.7
		Pp.Add({81, 114, 99, 95, 107, 106, 99, 113, 113, 44, 99, 118, 99}); // Steamless.exe
		Pp.Add({86, 99, 108, 109, 113, 44, 99, 118, 99}); // Xenos.exe
		Pp.Add({86, 99, 108, 109, 113, 52, 50, 44, 99, 118, 99}); // Xenos64.exe
		Pp.Add({78, 112, 109, 97, 99, 113, 113, 70, 95, 97, 105, 99, 112, 44, 99, 118, 99}); // ProcessHacker.exe

		for (TArray<uint8>& P : Pp)
		{
			FString Ps;
			ByteArrayToString_Offset(P.GetData(), P.Num(), -2, Ps);

			#if WITH_EDITOR
			//const bool bProgramRunning = UReadyOrNotFunctionLibrary::IsProcessRunning_Windows(*ProgramString);
			//ULog::Info("Is " + ProgramString + " running? " + (bProgramRunning ? "True" : "False"));
			#endif

			if (!bWantsRunPThread)
				return;
			
			if (UReadyOrNotFunctionLibrary::IsProcessRunning_Windows(*Ps))
			{
				bIsBuildPirated = true;
				break;
			}
		}

		#if !WITH_EDITOR
		#if !UE_BUILD_SHIPPING
		V_LOGM(LogReadyOrNot, "Checking Blacklist Program Names %d", Hashes.BLPN.Num());
		#endif
		
		if (!bWantsRunPThread)
			return;
		
		for (FString Ps : Hashes.BLPN)
		{
			FString Psd;
			FBase64::Decode(Ps, Psd);
			if (!bWantsRunPThread)
				return;
			
			if (UReadyOrNotFunctionLibrary::IsProcessRunning_Windows(*Psd))
			{
				bIsBuildPirated = true;
				break;
			}
		}
		#endif
	}

	// Program exe hashes
	{
		TArray<TArray<uint8>> Pph;
		Pph.Add({99, 49, 96, 95, 47, 96, 97, 50, 100, 49, 50, 54, 99, 96, 54, 100, 98, 95, 49, 54, 49, 97, 48, 54, 96, 55, 51, 98, 47, 99, 53, 100}); // SmartSteamLoader_x64.exe [MD5 Hash] (e3ba1bc4f348eb8fda383c28b95d1e7f)
		Pph.Add({46, 99, 53, 100, 98, 99, 46, 55, 54, 98, 52, 50, 95, 55, 49, 99, 52, 46, 47, 55, 47, 98, 48, 51, 99, 46, 52, 96, 100, 52, 50, 48}); // SmartSteamLoader.exe [MD5 Hash] (0e7fde098d64a93e60191d25e06bf642)
		Pph.Add({49, 47, 95, 100, 96, 95, 97, 46, 55, 100, 53, 52, 47, 98, 49, 48, 46, 96, 52, 48, 49, 46, 51, 97, 100, 99, 51, 96, 97, 96, 98, 47}); // SSELauncher.exe [MD5 Hash] (31afbac09f761d320b62305cfe5bcbd1)
		Pph.Add({54, 50, 47, 48, 52, 48, 95, 48, 98, 54, 54, 50, 52, 52, 46, 96, 99, 53, 53, 48, 95, 46, 100, 47, 52, 47, 54, 54, 54, 100, 49, 99}); // LumaGameLauncher_x64.exe v1.9.7 [MD5 Hash] (841262a2d884660be772a0f161888f3e)
		Pph.Add({51, 95, 49, 53, 96, 97, 55, 53, 99, 48, 100, 54, 46, 52, 100, 52, 55, 53, 100, 52, 49, 100, 53, 52, 50, 55, 50, 96, 47, 100, 96, 100}); // LumaGameLauncher_x86.exe v1.9.7[MD5 Hash] (5a37bc97e2f806f697f63f76494b1fbf)
		Pph.Add({54, 53, 53, 46, 55, 51, 47, 52, 51, 96, 54, 50, 52, 53, 53, 48, 96, 55, 100, 51, 47, 99, 49, 51, 55, 95, 99, 52, 96, 99, 52, 52}); // Steamless.exe [MD5 Hash] (877095165b846772b9f51e359ae6be66)

		for (TArray<uint8>& P : Pph)
		{
			FString Ps;
			ByteArrayToString_Offset(P.GetData(), P.Num(), -2, Ps);

			#if WITH_EDITOR
			//const bool bProgramRunning = UReadyOrNotFunctionLibrary::IsProcessRunning_Windows(*ProgramString);
			//ULog::Info("Is " + ProgramString + " running? " + (bProgramRunning ? "True" : "False"));
			#endif
			if (!bWantsRunPThread)
				return;
			
			if (UReadyOrNotFunctionLibrary::IsProcessRunning_Windows(Ps))
			{
				bIsBuildPirated = true;
				break;
			}
		}
		
		#if !WITH_EDITOR
		#if !UE_BUILD_SHIPPING
		V_LOGM(LogReadyOrNot, "Checking Blacklist Program Hashes %d", Hashes.BLPHZ.Num());
		#endif

		if (!bWantsRunPThread)
			return;

		for (FString P: Hashes.BLPHZ)
		{
			FString Psd;
			FBase64::Decode(P, Psd);
			if (!bWantsRunPThread)
				return;
			
			if (UReadyOrNotFunctionLibrary::IsProcessRunning_Windows(Psd))
			{
				bIsBuildPirated = true;
				break;
			}
		}
		#endif
	}

	// Programs window title names
	{
		TArray<TArray<uint8>> Ppwtn;
		Ppwtn.Add({81, 107, 95, 112, 114, 81, 114, 99, 95, 107, 67, 107, 115, 30, 74, 95, 115, 108, 97, 102, 99, 112}); // SmartSteamEmu Launcher
		Ppwtn.Add({81, 107, 95, 112, 114, 81, 114, 99, 95, 107, 67, 107, 115}); // SmartSteamEmu
		Ppwtn.Add({65, 102, 99, 95, 114, 30, 67, 108, 101, 103, 108, 99}); // Cheat Engine
		Ppwtn.Add({81, 114, 99, 95, 107, 106, 99, 113, 113}); // Steamless
		Ppwtn.Add({78, 112, 109, 97, 99, 113, 113, 30, 70, 95, 97, 105, 99, 112}); // Process Hacker
		Ppwtn.Add({86, 99, 108, 109, 113}); // Xenos
		Ppwtn.Add({86, 99, 108, 109, 113, 52, 50}); // Xenos64

		for (TArray<uint8>& P : Ppwtn)
		{
			FString Ps;
			ByteArrayToString_Offset(P.GetData(), P.Num(), -2, Ps);

			#if WITH_EDITOR
			//const bool bProgramRunning = UReadyOrNotFunctionLibrary::IsProcessRunning_Windows(*ProgramString);
			//ULog::Info("Is " + ProgramString + " running? " + (bProgramRunning ? "True" : "False"));
			#endif
			if (!bWantsRunPThread)
				return;
			
			if (UReadyOrNotFunctionLibrary::DoesProcessWindowTitleExist_Windows(Ps) || UReadyOrNotFunctionLibrary::DoesProcessWindowTitleContain_Windows(Ps))
			{
				bIsBuildPirated = true;
				break;
			}
		}
		
		#if !WITH_EDITOR
		#if !UE_BUILD_SHIPPING
		V_LOGM(LogReadyOrNot, "Checking Blacklist Window Titles %d", Hashes.BLWT.Num());
		#endif

		if (!bWantsRunPThread)
			return;

		for (FString P : Hashes.BLWT)
		{
			FString Psd;
			FBase64::Decode(P, Psd);
			if (UReadyOrNotFunctionLibrary::DoesProcessWindowTitleExist_Windows(Psd) || UReadyOrNotFunctionLibrary::DoesProcessWindowTitleContain_Windows(Psd))
			{
				bIsBuildPirated = true;
				break;
			}
		}
		#endif
	}

	// Dll filenames
	{
		TArray<TArray<uint8>> Dd;
		Dd.Add({81, 107, 95, 112, 114, 81, 114, 99, 95, 107, 67, 107, 115, 44, 98, 106, 106}); // SmartSteamEmu.dll [SSE Launcher]
		Dd.Add({81, 107, 95, 112, 114, 81, 114, 99, 95, 107, 67, 107, 115, 52, 50, 44, 98, 106, 106}); // SmartSteamEmu64.dll [SSE Launcher]
		Dd.Add({81, 81, 67, 77, 116, 99, 112, 106, 95, 119, 44, 98, 106, 106}); // SSEOverlay.dll [SSE Launcher]
		
		Dd.Add({74, 115, 107, 95, 69, 95, 107, 99, 74, 95, 115, 108, 97, 102, 99, 112, 93, 118, 54, 52, 44, 98, 106, 106}); // LumaGameLauncher_x86.dll [LumaGameLauncher v1.9.7]
		Dd.Add({74, 115, 107, 95, 69, 95, 107, 99, 74, 95, 115, 108, 97, 102, 99, 112, 93, 118, 52, 50, 44, 98, 106, 106}); // LumaGameLauncher_x64.dll [LumaGameLauncher v1.9.7]

		Dd.Add({77, 108, 106, 103, 108, 99, 68, 103, 118, 52, 50, 44, 98, 106, 106}); // OnlineFix64.dll
		Dd.Add({81, 114, 115, 96, 66, 80, 75, 52, 50, 44, 98, 106, 106}); // StubDRM64.dll

		Dd.Add({80, 99, 95, 98, 119, 77, 112, 76, 109, 114, 89, 115, 108, 105, 108, 109, 117, 108, 97, 102, 99, 95, 114, 113, 44, 107, 99, 91, 44, 98, 106, 106}); // ReadyOrNot[unknowncheats.me].dll

		for (TArray<uint8>& D : Dd)
		{
			FString Ds;
			ByteArrayToString_Offset(D.GetData(), D.Num(), -2, Ds);

			if (!bWantsRunPThread)
				return;
			
			if (UReadyOrNotFunctionLibrary::IsDLLLoaded_Windows(*Ds))
			{
				bIsBuildPirated = true;
					
				#if !WITH_EDITOR
				AActor* MyCoolAsNullReferencePtr = nullptr;
				MyCoolAsNullReferencePtr->SetActorLocation(FVector(0));
				#endif
			}
		}
		
		#if !WITH_EDITOR
		#if !UE_BUILD_SHIPPING
		V_LOGM(LogReadyOrNot, "Checking Blacklist DLL Names %d", Hashes.BLDLLN.Num());
		#endif

		if (!bWantsRunPThread)
			return;

		for (FString d : Hashes.BLDLLN)
		{
			FString Psd;
			FBase64::Decode(d, Psd);
			if (!bWantsRunPThread)
				return;
			
			if (UReadyOrNotFunctionLibrary::IsDLLLoaded_Windows(*Psd))
			{
				bIsBuildPirated = true;
					
				#if !WITH_EDITOR
				AActor* MyCoolAsNullReferencePtr = nullptr;
				MyCoolAsNullReferencePtr->SetActorLocation(FVector(0));
				#endif
			}
		}
		#endif
	}

	// Dll hashes (for this process)
	{
		for (TArray<uint8>& D : Ddh)
		{
			FString Ds;
			ByteArrayToString_Offset(D.GetData(), D.Num(), -2, Ds);

			if (!bWantsRunPThread)
				return;
			
			if (UReadyOrNotFunctionLibrary::IsDLLLoadedThisProcess_Windows(Ds))
			{
				bIsBuildPirated = true;
				
				#if !WITH_EDITOR
				AActor* MyCoolAsNullReferencePtr = nullptr;
				MyCoolAsNullReferencePtr->SetActorLocation(FVector(0));
				#endif
			}
		}
		
		#if !WITH_EDITOR
		#if !UE_BUILD_SHIPPING
		V_LOGM(LogReadyOrNot, "Checking Blacklist DLL Hashes %d (Our Process)", Hashes.BLDLLHZ.Num());
		#endif

		if (!bWantsRunPThread)
			return;

		for (FString Ds : Hashes.BLDLLHZ)
		{
			FString Psd;
			FBase64::Decode(Ds, Psd);
			if (!bWantsRunPThread)
				return;
			
			if (UReadyOrNotFunctionLibrary::IsDLLLoadedThisProcess_Windows(Psd))
			{
				bIsBuildPirated = true;
				
				#if !WITH_EDITOR
				AActor* MyCoolAsNullReferencePtr = nullptr;
				MyCoolAsNullReferencePtr->SetActorLocation(FVector(0));
				#endif
			}
		}
		#endif
	}

	// Dll hashes (for other processes)
	{
		Ddh.Empty(4);
		Ddh.Add({97, 99, 55, 97, 95, 98, 49, 54, 48, 51, 49, 96, 48, 55, 55, 47, 48, 54, 47, 98, 49, 97, 49, 54, 99, 51, 95, 46, 53, 53, 49, 51}); // SmartSteamEmu.dll [SSE Launcher] (ce9cad38253b2991281d3c38e5a07735)
		Ddh.Add({96, 46, 100, 55, 49, 49, 99, 47, 95, 46, 49, 49, 50, 52, 98, 54, 49, 55, 97, 98, 50, 97, 49, 95, 51, 47, 97, 52, 50, 50, 48, 47}); // SmartSteamEmu64.dll [SSE Launcher] (b0f933e1a03346d839cd4c3a51c64421)

		Ddh.Add({54, 49, 100, 51, 99, 50, 100, 54, 95, 49, 48, 99, 46, 99, 54, 49, 55, 96, 54, 50, 54, 49, 49, 48, 50, 99, 52, 51, 51, 99, 96, 51}); // LumaGameLauncher_x86.dll [LumaGameLauncher] (83f5e4f8a32e0e839b8483324e655eb5)
		Ddh.Add({51, 50, 53, 49, 51, 55, 55, 95, 54, 53, 95, 48, 48, 98, 100, 53, 97, 47, 97, 97, 46, 55, 51, 53, 46, 97, 49, 46, 97, 46, 99, 98}); // LumaGameLauncher_x64.dll [LumaGameLauncher] (5473599a87a22df7c1cc09570c30c0ed)

		for (TArray<uint8>& D : Ddh)
		{
			FString Ds;
			ByteArrayToString_Offset(D.GetData(), D.Num(), -2, Ds);

			if (!bWantsRunPThread)
				return;
			
			if (UReadyOrNotFunctionLibrary::IsDLLLoaded_Windows(Ds))
			{
				bIsBuildPirated = true;
				
				#if !WITH_EDITOR
				AActor* MyCoolAsNullReferencePtr = nullptr;
				MyCoolAsNullReferencePtr->SetActorLocation(FVector(0));
				#endif
			}
		}
				
		#if !WITH_EDITOR
		#if !UE_BUILD_SHIPPING
		V_LOGM(LogReadyOrNot, "Checking Blacklist DLL Hashes %d (Other Processes)", Hashes.BLDLLHZ.Num());
		#endif
		
		if (!bWantsRunPThread)
			return;
		
		for (FString Ds : Hashes.BLDLLHZ)
		{
			FString Psd;
			FBase64::Decode(Ds, Psd);
			if (!bWantsRunPThread)
				return;
			
			if (UReadyOrNotFunctionLibrary::IsDLLLoaded_Windows(Psd))
			{
				AActor* MyCoolAsNullReferencePtr = nullptr;
				MyCoolAsNullReferencePtr->SetActorLocation(FVector(0));
			}
		}
		#endif
	}*/
	#endif
}
#endif

bool UReadyOrNotGameInstance::IsLoggedIntoBackend() const
{
	return ReadyOrNotBackend && ReadyOrNotBackend->IsLoggedIn();
}

uint8 UReadyOrNotGameInstance::GetBackendState() const
{
	if (!ReadyOrNotBackend)
		return 0;
	return (uint8)ReadyOrNotBackend->GetLoginState();
}

void UReadyOrNotGameInstance::RetryLogin()
{
	if (ReadyOrNotBackend)
	{
		ReadyOrNotBackend->StartLogin();
	}
}

/*
void UReadyOrNotGameInstance::LoadHashMap()
{
	UReadyOrNotSaveGame* lg = UBpGameplayHelperLib::GetLoadGameInstance();
	if (lg)
	{
		Hashes = lg->HashMap;
	}
}

void UReadyOrNotGameInstance::SaveHashMap()
{
	UReadyOrNotSaveGame* lg = UBpGameplayHelperLib::GetLoadGameInstance();
	if (lg)
	{
		lg->HashMap = Hashes;
		UGameplayStatics::SaveGameToSlot(lg, lg->SaveSlotName, lg->UserIndex);	
	}
}
*/

#if defined(WITH_VIVOX)
void UReadyOrNotGameInstance::BindLoginSessionHandlers(bool DoBind, ILoginSession& LoginSession)
{
	if (DoBind)
	{
		LoginSession.EventStateChanged.AddUObject(this, &UReadyOrNotGameInstance::OnLoginSessionStateChanged);
	}
	else
	{
		LoginSession.EventStateChanged.RemoveAll(this);
	}
}

void UReadyOrNotGameInstance::BindChannelSessionHandlers(bool DoBind, IChannelSession& ChannelSession)
{
	if (DoBind)
	{
		ChannelSession.EventAfterParticipantAdded.AddUObject(this, &UReadyOrNotGameInstance::OnChannelParticipantAdded);
		ChannelSession.EventBeforeParticipantRemoved.AddUObject(this, &UReadyOrNotGameInstance::OnChannelParticipantRemoved);
		ChannelSession.EventAfterParticipantUpdated.AddUObject(this, &UReadyOrNotGameInstance::OnChannelParticipantUpdated);
		ChannelSession.EventAudioStateChanged.AddUObject(this, &UReadyOrNotGameInstance::OnChannelAudioStateChanged);
		ChannelSession.EventTextStateChanged.AddUObject(this, &UReadyOrNotGameInstance::OnChannelTextStateChanged);
		ChannelSession.EventChannelStateChanged.AddUObject(this, &UReadyOrNotGameInstance::OnChannelStateChanged);
		ChannelSession.EventTextMessageReceived.AddUObject(this, &UReadyOrNotGameInstance::OnChannelTextMessageReceived);
	}
	else
	{
		ChannelSession.EventAfterParticipantAdded.RemoveAll(this);
		ChannelSession.EventBeforeParticipantRemoved.RemoveAll(this);
		ChannelSession.EventAfterParticipantUpdated.RemoveAll(this);
		ChannelSession.EventAudioStateChanged.RemoveAll(this);
		ChannelSession.EventTextStateChanged.RemoveAll(this);
		ChannelSession.EventChannelStateChanged.RemoveAll(this);
		ChannelSession.EventTextMessageReceived.RemoveAll(this);
	}
}

VivoxCoreError UReadyOrNotGameInstance::Initialize(int logLevel)
{
	if (GIsEditor)
	{
		UE_LOG(LogReadyOrNot, Warning, TEXT("This sample does not support Play In-Editor: the Vivox Plugin does, but hosting or joining a match in the underlying ShooterGame sample (base app, not our sample integration) doesn't function properly in the editor, so the plugin cannot adequately be showcased."));
		return VxErrorInvalidOperation; // This is not necessary for your game and is just a choice for our sample.
	}

	if (bInitialized)
	{
		return VxErrorSuccess;
	}

	VivoxConfig Config;
	Config.SetLogLevel((vx_log_level)logLevel);
	VivoxCoreError Status = VivoxVoiceClient->Initialize(Config);
	if (Status != VxErrorSuccess)
	{
		UE_LOG(LogReadyOrNot, Error, TEXT("Initialize failed: %s (%d)"), ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)), Status);
	}
	else
	{
		bInitialized = true;
	}

	return Status;
}

void UReadyOrNotGameInstance::Uninitialize()
{
#if !UE_EDITOR
	if (!bInitialized)
	{
		return;
	}

	VivoxVoiceClient->Uninitialize();
#endif
}

void UReadyOrNotGameInstance::StartVivoxLogin()
{
	const FString PlayerName = GetVivoxSafePlayerName(GetFirstGamePlayer()->GetPreferredUniqueNetId()->ToString());
	if (!ReadyOrNotBackend)
	{
		return;
	}
	
	if (!ReadyOrNotBackend->IsLoggedIn())
	{
		ReadyOrNotBackend->StartLogin();
		return;
	}
	if (bVivoxLoggedIn)
    {
        UE_LOG(LogReadyOrNot, Verbose, TEXT("Already logged in"));
        return;
    }

    if (bVivoxLoggingIn)
    {
        UE_LOG(LogReadyOrNot, Verbose, TEXT("Already logging in"));
        return;
    }

    if (!bInitialized)
    {
        UE_LOG(LogReadyOrNot, Verbose, TEXT("Not initialized"));
        return;
    }
	bVivoxLoggingIn = true;
	ReadyOrNotBackend->RetrieveVivoxLoginToken(PlayerName);
}

void UReadyOrNotGameInstance::DoVivoxLogin(FString Token)
{
	bVivoxLoggingIn = false;
	if (Token.IsEmpty())
	{
		return;
	}
	
	V_LOGM(LogReadyOrNot, "Retrieved login token: %s", *Token);
	LoggedInPlayerName = GetVivoxSafePlayerName(GetFirstGamePlayer()->GetPreferredUniqueNetId()->ToString());
	LoggedInAccountID = AccountId(VIVOX_VOICE_ISSUER, LoggedInPlayerName, VIVOX_VOICE_DOMAIN);
	ILoginSession &LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInAccountID);

	UE_LOG(LogReadyOrNot, Verbose, TEXT("Logging in %s with token %s"), *LoggedInPlayerName, *Token);

	ILoginSession::FOnBeginLoginCompletedDelegate OnBeginLoginCompleteCallback;
	OnBeginLoginCompleteCallback.BindLambda([this, &LoginSession](VivoxCoreError Status)
	{
		bVivoxLoggingIn = false;
		if (VxErrorSuccess != Status)
		{
#if UE_BUILD_DEVELOPMENT
			UE_LOG(LogReadyOrNot, Error, TEXT("Login failure for %s: %s (%d)"), *LoggedInPlayerName, ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)), Status);
#endif
			BindLoginSessionHandlers(false, LoginSession); // Unbind handlers if we fail to log in
			LoggedInAccountID = AccountId();
			LoggedInPlayerName = FString();
			bVivoxLoggedIn = false; // should already be false, but we'll just make sure
		}
		else
		{
#if UE_BUILD_DEVELOPMENT
			UE_LOG(LogReadyOrNot, Log, TEXT("Login success for %s"), *LoggedInPlayerName);
#endif
			bVivoxLoggedIn = true;
		}
	});
	BindLoginSessionHandlers(true, LoginSession);
	bVivoxLoggingIn = true;	

	LoginSession.BeginLogin(VIVOX_VOICE_SERVER, Token, OnBeginLoginCompleteCallback);
}
 
void UReadyOrNotGameInstance::Logout()
{
	if (!bVivoxLoggedIn && !bVivoxLoggingIn)
	{
#if UE_BUILD_DEVELOPMENT
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Not logged in, skipping logout"));
#endif
		return;
	}

	ILoginSession &LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInAccountID);

	LoginSession.Logout();

	LoggedInAccountID = AccountId();
	LoggedInPlayerName = FString();
	bVivoxLoggingIn = false;
	bVivoxLoggedIn = false;
}
#endif

bool UReadyOrNotGameInstance::JoinVoiceChannels(FString OnlineSessionId, int32 TeamNum)
{
#if defined(WITH_VIVOX)
	if (OnlineSessionId.IsEmpty() || OnlineSessionId.Len() < 5)
	{
		return false;
	}

	FString channelName = *OnlineSessionId;
// UE5UPGRADE : Console	
// #if PLATFORM_XBOXONE || PLATFORM_XSX
// 	OnlineSessionId.Split(TEXT("/"), NULL, &channelName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
// 	UE_LOG(LogReadyOrNot, Log, TEXT("Split OnlineSessionId: %s"), *channelName);
// 	channelName = channelName.ToUpper();
// #endif
	
	Join(ChannelType::Positional, false, FString::Printf(TEXT("TP%s"), *channelName), PTTKey::PTTAreaChannel);
	Join(ChannelType::NonPositional, false, FString::Printf(TEXT("TN%d_%s"), TeamNum, *channelName), PTTKey::PTTTeamChannel);

	return true;
#else
	return false;
#endif
}

#if defined(WITH_VIVOX)
VivoxCoreError UReadyOrNotGameInstance::Join(ChannelType ChannelType, bool ShouldTransmitOnJoin,
	const FString& ChannelName, PTTKey AssignChanneltoPTTKey, FString Token)
{
	if (!bVivoxLoggedIn)
    {
        UE_LOG(LogReadyOrNot, Warning, TEXT("Not logged in; cannot join a channel"));
        return VxErrorNotLoggedIn;
    }
    ensure(!LoggedInPlayerName.IsEmpty());
    ensure(!ChannelName.IsEmpty());
	
    ILoginSession &LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInAccountID);
	Channel3DProperties ChannelProperties(8100, 270, 1.0, EAudioFadeModel::InverseByDistance);

	// Load the ini voip settings if available
	const UReadyOrNotVoipSettings* VoipSettings = GetDefault<UReadyOrNotVoipSettings>();
	if (VoipSettings)
	{
		ChannelProperties = Channel3DProperties(VoipSettings->AudibleDistance, VoipSettings->ConversationalDistance,
			VoipSettings->FadeIntensityByDistance, VoipSettings->FadeModel);
	}
	
    // It's perfectly safe to add 3D properties to any channel type (they don't have any effect if the channel type is not Positional)
    ChannelId Channel = ChannelId(VIVOX_VOICE_ISSUER, ChannelName, VIVOX_VOICE_DOMAIN, ChannelType, ChannelProperties);
    IChannelSession &ChannelSession = LoginSession.GetChannelSession(Channel);

    // IMPORTANT: in production, developers should NOT use the insecure client-side token generation methods.
    // To generate secure access tokens, call GenerateClientJoinToken or a custom implementation from your game server.
    // This is important not only to secure access to Chat features, but tokens issued by the client could
    // appear expired if the client's clock is set incorrectly, resulting in rejection.
	if (Token.IsEmpty())
	{
		FChannelInfo ChannelInfo;
		ChannelInfo.ChannelType = ChannelType;
		ChannelInfo.bShouldTransmitOnJoin = ShouldTransmitOnJoin;
		ChannelInfo.AssignChannelToPTTKey = AssignChanneltoPTTKey;
		ChannelInfo.ChannelName = ChannelName;
		PendingChannelJoin.Add(FGenericPlatformHttp::UrlEncode(Channel.ToString()), ChannelInfo);
		// grab the token then call this again
		if (ReadyOrNotBackend)
		{
			ReadyOrNotBackend->RetrieveJoinToken(LoggedInPlayerName, Channel.ToString());
			return VxErrorSuccess;
		} else
		{
			return VxErrorInternalError;
		}

	}

	V_LOGM(LogReadyOrNot, "Received Token %s", *Token);

    IChannelSession::FOnBeginConnectCompletedDelegate OnBeginConnectCompleteCallback;
    OnBeginConnectCompleteCallback.BindLambda([this, ShouldTransmitOnJoin, AssignChanneltoPTTKey, &LoginSession, &ChannelSession](VivoxCoreError Status)
    {
        if (VxErrorSuccess != Status)
        {
            UE_LOG(LogReadyOrNot, Error, TEXT("Join failure for %s: %s (%d)"), *ChannelSession.Channel().Name(), ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)), Status);
            BindChannelSessionHandlers(false, ChannelSession); // Unbind handlers if we fail to join.
            LoginSession.DeleteChannelSession(ChannelSession.Channel()); // Disassociate this ChannelSession from the LoginSession.
        }
        else
        {
            UE_LOG(LogReadyOrNot, Log, TEXT("Join success for %s"), *ChannelSession.Channel().Name());
            if (ChannelType::Positional == ChannelSession.Channel().Type())
            {
                ConnectedPositionalChannel = ChannelSession.Channel();
            }

            if (PTTKey::PTTAreaChannel == AssignChanneltoPTTKey)
            {
                PTTAreaChannel = TPairInitializer<ChannelId, bool>(ChannelSession.Channel(), false);
            }
            else if (PTTKey::PTTTeamChannel == AssignChanneltoPTTKey)
            {
                PTTTeamChannel = TPairInitializer<ChannelId, bool>(ChannelSession.Channel(), false);
            }

            // NB: It is usually not necessary to adjust transmission when joining channels.
            // The conditional below controls desired behavior specific to this application.
            if (ShouldTransmitOnJoin)
            {
                if (AssignChanneltoPTTKey != PTTKey::PTTNoChannel)
                    MultiChanToggleChat(AssignChanneltoPTTKey);
                else
                    LoginSession.SetTransmissionMode(TransmissionMode::All);
            }
            else if (LoginSession.ChannelSessions().Num() == 1)
            {
                LoginSession.SetTransmissionMode(TransmissionMode::None);
            }
        }
    });
    BindChannelSessionHandlers(true, ChannelSession);

    return ChannelSession.BeginConnect(true, false, ShouldTransmitOnJoin, Token, OnBeginConnectCompleteCallback);
}
#endif

FString UReadyOrNotGameInstance::GetDiscordOneTimeUseCode()
{
	if (ReadyOrNotBackend && !ReadyOrNotBackend->GetCachedOneTimeDiscordCode().IsEmpty())
	{
		FString Code = ReadyOrNotBackend->GetCachedOneTimeDiscordCode();
		Code.ReplaceInline(TEXT("\""), TEXT(""));
		Code.ReplaceInline(TEXT("\n"), TEXT(""));
		Code.ReplaceInline(TEXT("\r"), TEXT(""));
		return Code;
	}
	return NSLOCTEXT("Code Not Available", "Code Not Available", "Code Not Available").ToString();
}

#if defined(WITH_VIVOX)
void UReadyOrNotGameInstance::OnJoinTokenReceived(FString ChannelName, FString Token)
{
	if (PendingChannelJoin.Find(ChannelName))
	{
		
		FChannelInfo ChannelInfo = PendingChannelJoin[ChannelName];
		Join(ChannelInfo.ChannelType, ChannelInfo.bShouldTransmitOnJoin, ChannelInfo.ChannelName, ChannelInfo.AssignChannelToPTTKey, Token);
		PendingChannelJoin.Remove(ChannelName);
	}
}
#endif

void UReadyOrNotGameInstance::LeaveVoiceChannels()
{
#if defined(WITH_VIVOX)
	if (!bVivoxLoggedIn)
	{
		UE_LOG(LogReadyOrNot, Verbose, TEXT("Not logged in; cannot leave channel(s)"));
		return;
	}

	TArray<ChannelId> ChannelSessionKeys;
	VivoxVoiceClient->GetLoginSession(LoggedInAccountID).ChannelSessions().GenerateKeyArray(ChannelSessionKeys);
	for (ChannelId SessionKey : ChannelSessionKeys)
	{
		UE_LOG(LogReadyOrNot, Log, TEXT("Disconnecting from channel %s"), *SessionKey.Name());
		BindChannelSessionHandlers(false, VivoxVoiceClient->GetLoginSession(LoggedInAccountID).GetChannelSession(SessionKey));
		VivoxVoiceClient->GetLoginSession(LoggedInAccountID).DeleteChannelSession(SessionKey);
	}

	// Always clear stashed Positional channel and PTT channels
	ConnectedPositionalChannel = ChannelId();
	PTTAreaChannel.Key = ChannelId();
	PTTTeamChannel.Key = ChannelId();
#endif
}

void UReadyOrNotGameInstance::Update3DPosition(APawn* Pawn)
{
#if defined(WITH_VIVOX)
	/// Return if argument is invalid.
	if (Pawn == nullptr)
		return;

	/// Return if we're not in a positional channel.
	if (ConnectedPositionalChannel.IsEmpty())
		return;

	/// Update cached 3D position and orientation.
	CachedPosition.SetValue(Pawn->GetActorLocation());
	CachedForwardVector.SetValue(Pawn->GetActorForwardVector());
	CachedUpVector.SetValue(Pawn->GetActorUpVector());
	
	/// Return If there's no change from cached values.
	if (!Get3DValuesAreDirty())
		return;
	
	ILoginSession &LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInAccountID);
	LoginSession.GetChannelSession(ConnectedPositionalChannel).Set3DPosition(CachedPosition.GetValue(), CachedPosition.GetValue(), CachedForwardVector.GetValue(), CachedUpVector.GetValue());

	Clear3DValuesAreDirty();
#endif
}

#if defined(WITH_VIVOX)
void UReadyOrNotGameInstance::OnLoginSessionStateChanged(LoginState State)
{
}

void UReadyOrNotGameInstance::OnChannelParticipantAdded(const IParticipant& Participant)
{
	ChannelId Channel = Participant.ParentChannelSession().Channel();
	UE_LOG(LogReadyOrNot, Log, TEXT("User %s has joined channel %s (self = %s)"), *Participant.Account().Name(), *Channel.Name(), Participant.IsSelf() ? TEXT("true") : TEXT("false"));
}

void UReadyOrNotGameInstance::OnChannelParticipantRemoved(const IParticipant& Participant)
{
	ChannelId Channel = Participant.ParentChannelSession().Channel();
	UE_LOG(LogReadyOrNot, Log, TEXT("User %s has left channel %s (self = %s)"), *Participant.Account().Name(), *Channel.Name(), Participant.IsSelf() ? TEXT("true") : TEXT("false"));
}

void UReadyOrNotGameInstance::OnChannelParticipantUpdated(const IParticipant& Participant)
{
	if (Participant.IsSelf())
	{
		UE_LOG(LogReadyOrNot, Log, TEXT("Self Participant Updated (audio=%d, text=%d, speaking=%d)"), Participant.InAudio(), Participant.InText(), Participant.SpeechDetected());
	}
}

void UReadyOrNotGameInstance::OnChannelAudioStateChanged(const IChannelConnectionState& State)
{
	UE_LOG(LogReadyOrNot, Log, TEXT("ChannelSession Audio State Change in %s: %s"), *State.ChannelSession().Channel().Name(), *RON_ENUM_TO_STRING(ConnectionState, State.State()));
}

void UReadyOrNotGameInstance::OnChannelTextStateChanged(const IChannelConnectionState& State)
{
	UE_LOG(LogReadyOrNot, Log, TEXT("ChannelSession Text State Change in %s: %s"), *State.ChannelSession().Channel().Name(), *RON_ENUM_TO_STRING(ConnectionState, State.State()));
}

void UReadyOrNotGameInstance::OnChannelStateChanged(const IChannelConnectionState& State)
{
	UE_LOG(LogReadyOrNot, Log, TEXT("ChannelSession Connection State Change in %s: %s"), *State.ChannelSession().Channel().Name(), *RON_ENUM_TO_STRING(ConnectionState, State.State()));
}

void UReadyOrNotGameInstance::OnChannelTextMessageReceived(const IChannelTextMessage& Message)
{
	UE_LOG(LogReadyOrNot, Log, TEXT("Message Received from %s: %s"), *Message.Sender().Name(), *Message.Message());
}
#endif

bool UReadyOrNotGameInstance::GetMutedState(FString UniqueNetId)
{
#if defined(WITH_VIVOX)
	if (!ConnectedPositionalChannel.IsEmpty()) 
	{
		ILoginSession& LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInAccountID);
		auto Participants = LoginSession.GetChannelSession(ConnectedPositionalChannel).Participants();
		if (Participants.Contains(UniqueNetId))
		{
			return Participants[UniqueNetId]->LocalMute();
		} 
		return false;
	}
	return false;

#else
	return false;
#endif
}

void UReadyOrNotGameInstance::SetMutedState(FString UniqueNetId, bool value, FDelegateSetLocalMutedStateCompleted Delegate)
{
#if defined(WITH_VIVOX)
	if (!ConnectedPositionalChannel.IsEmpty())
	{
		ILoginSession& LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInAccountID);
		auto Participants = LoginSession.GetChannelSession(ConnectedPositionalChannel).Participants();
		if (Participants.Contains(UniqueNetId))
		{
			IParticipant* Participant = Participants[UniqueNetId];
			IParticipant::FOnBeginSetLocalMuteCompletedDelegate LocalMuteDelegate;
			LocalMuteDelegate.BindLambda([Delegate, Participant](int32 error)
			{
				Delegate.Broadcast();
			});
			Participant->BeginSetLocalMute(value, LocalMuteDelegate);
		}
	}
#endif
}


bool UReadyOrNotGameInstance::IsInitialized()
{
#if defined(WITH_VIVOX)
	return bInitialized;
#else
	return false;
#endif
}

bool UReadyOrNotGameInstance::IsLoggedIn()
{
#if defined(WITH_VIVOX)
	return bVivoxLoggedIn;
#else
	return false;
#endif
}

bool UReadyOrNotGameInstance::MultiChanPushToTalk(PTTKey Key, bool PTTKeyPressed)
{
#if defined(WITH_VIVOX)

	FString Channel;
	if (PTTKey::PTTAreaChannel == Key && !PTTAreaChannel.Key.IsEmpty())
	{
		UE_LOG(LogReadyOrNot, Log, TEXT("MultiChanPushToTalk: %s talking in %s"), PTTKeyPressed ? TEXT("Started") : TEXT("Stopped"), *PTTAreaChannel.Key.Name());
		PTTAreaChannel.Value = PTTKeyPressed;
	}
	else if (PTTKey::PTTTeamChannel == Key && !PTTTeamChannel.Key.IsEmpty())
	{
		UE_LOG(LogReadyOrNot, Log, TEXT("MultiChanPushToTalk: %s talking in %s"), PTTKeyPressed ? TEXT("Started") : TEXT("Stopped"), *PTTTeamChannel.Key.Name());
		PTTTeamChannel.Value = PTTKeyPressed;
	}
	else
	{
		UE_LOG(LogReadyOrNot, Warning, TEXT("MultiChanPushToTalk: No ChannelId assigned to %s"), *RON_ENUM_TO_STRING(PTTKey, Key));
		return false;
	}

	if (PTTAreaChannel.Value && PTTTeamChannel.Value) // Both
	{
		LastKnownTransmittingChannel = LastKnownTransmittingChannel == PTTAreaChannel.Key ? PTTTeamChannel.Key : PTTAreaChannel.Key; // flip
		return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::All) == VX_E_SUCCESS;
	}
	else if (PTTAreaChannel.Value) // Area Only
	{
		LastKnownTransmittingChannel = PTTAreaChannel.Key;
		return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::Single, PTTAreaChannel.Key) == VX_E_SUCCESS;
	}
	else if (PTTTeamChannel.Value) // Team Only
	{
		LastKnownTransmittingChannel = PTTTeamChannel.Key;
		return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::Single, PTTTeamChannel.Key) == VX_E_SUCCESS;
	}
	else // None
	{
		return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::None) == VX_E_SUCCESS;
	}
#else
	return false;
#endif
}

bool UReadyOrNotGameInstance::MultiChanToggleChat(PTTKey Key)
{
#if defined(WITH_VIVOX)
	if (PTTKey::PTTAreaChannel == Key && !PTTAreaChannel.Key.IsEmpty())
    {
        PTTAreaChannel.Value = !PTTAreaChannel.Value;
        UE_LOG(LogReadyOrNot, Log, TEXT("MultiChanToggleChat: Toggling transmission %s for %s"), PTTAreaChannel.Value ? TEXT("on") : TEXT("off"), *PTTAreaChannel.Key.Name());
        if (PTTAreaChannel.Value && PTTTeamChannel.Value)
        {
            UE_LOG(LogReadyOrNot, Log, TEXT("MultiChanToggleChat: Toggling transmission off for %s"), *PTTTeamChannel.Key.Name());
            PTTTeamChannel.Value = false;
        }
    }
    else if (PTTKey::PTTTeamChannel == Key && !PTTTeamChannel.Key.IsEmpty())
    {
        PTTTeamChannel.Value = !PTTTeamChannel.Value;
        UE_LOG(LogReadyOrNot, Log, TEXT("MultiChanToggleChat: Toggling transmission %s for %s"), PTTTeamChannel.Value ? TEXT("on") : TEXT("off"), *PTTTeamChannel.Key.Name());
        if (PTTTeamChannel.Value && PTTAreaChannel.Value)
        {
            UE_LOG(LogReadyOrNot, Log, TEXT("MultiChanToggleChat: Toggling transmission off for %s"), *PTTAreaChannel.Key.Name());
            PTTAreaChannel.Value = false;
        }
    }
    else
    {
        UE_LOG(LogReadyOrNot, Warning, TEXT("MultiChanToggleChat: No ChannelId assigned to %s"), *RON_ENUM_TO_STRING(PTTKey, Key));
        return false;
    }

    if (PTTAreaChannel.Value && PTTTeamChannel.Value) // Both
    {
        ensureMsgf(false, TEXT("MultiChanToggleChat: Transmitting to all channels on console. This is safe, but was designed not to happen in this sample."));
        LastKnownTransmittingChannel = LastKnownTransmittingChannel == PTTAreaChannel.Key ? PTTTeamChannel.Key : PTTAreaChannel.Key; // flip
        return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::All) == VX_E_SUCCESS;
    }
    else if (PTTAreaChannel.Value) // Area Only
    {
        LastKnownTransmittingChannel = PTTAreaChannel.Key;
        return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::Single, PTTAreaChannel.Key) == VX_E_SUCCESS;
    }
    else if (PTTTeamChannel.Value) // Team Only
    {
        LastKnownTransmittingChannel = PTTTeamChannel.Key;
        return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::Single, PTTTeamChannel.Key) == VX_E_SUCCESS;
    }
    else // None
    {
        return VivoxVoiceClient->GetLoginSession(LoggedInAccountID).SetTransmissionMode(TransmissionMode::None) == VX_E_SUCCESS;
    }
#else
	return false;
#endif
}

#if defined(WITH_VIVOX)
ILoginSession* UReadyOrNotGameInstance::GetLoginSessionForRoster()
{
	if (!LoggedInAccountID.IsEmpty())
	{
		return &VivoxVoiceClient->GetLoginSession(LoggedInAccountID);
	}
	return NULL;
}

TSharedPtr<IChannelSession> UReadyOrNotGameInstance::GetChannelSessionForRoster()
{
	for (auto& Session : VivoxVoiceClient->GetLoginSession(LoggedInAccountID).ChannelSessions())
	{
		if (Session.Value->Channel().Name().StartsWith("TN", ESearchCase::CaseSensitive))
		{
			return Session.Value;
		}
	}
	return NULL;
}

FString UReadyOrNotGameInstance::GetVivoxSafePlayerName(FString BaseName)
{
	bool bDoHash = false;

	// check length is <= 60 minus length of VivoxIssuer; default assumes max issuer length
	int32 VivoxSafePlayerLength = 35;
	FString VivoxIssuer = FString(VIVOX_VOICE_ISSUER);
	if (!VivoxIssuer.IsEmpty())
		VivoxSafePlayerLength = 60 - VivoxIssuer.Len();

	// a known issue limits this by one further character so this is >= instead of > for now.
	if (BaseName.Len() >= VivoxSafePlayerLength)
	{
		bDoHash = true;
	}
	else // also check character restrictions
		{
		FString ValidCharacters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890=+-_.!~()";
		int32 Loc;
		auto ConstItr = BaseName.CreateConstIterator();
		while (BaseName.IsValidIndex(ConstItr.GetIndex()))
		{
			if (!ValidCharacters.FindChar(BaseName[ConstItr++.GetIndex()], Loc))
			{
				bDoHash = true;
				break;
			}
		}
		}

	if (bDoHash)
		return FMD5::HashAnsiString(*BaseName);
	else
		return BaseName;
}
#endif

bool UReadyOrNotGameInstance::Get3DValuesAreDirty() const
{
	return (CachedPosition.IsDirty() ||
		CachedForwardVector.IsDirty() ||
		CachedUpVector.IsDirty());
}

void UReadyOrNotGameInstance::Clear3DValuesAreDirty()
{
	CachedPosition.SetDirty(false);
	CachedForwardVector.SetDirty(false);
	CachedUpVector.SetDirty(false);
}

bool UReadyOrNotGameInstance::GetAvailableAudioDevices(TArray<FString>& OutAudioDevices)
{
#if defined(WITH_VIVOX) && !WITH_EDITOR
	for (auto k : VivoxVoiceClient->AudioInputDevices().AvailableDevices())
	{
		OutAudioDevices.Add(k.Value->Name());
	}
	return OutAudioDevices.Num() > 0;
#else
	return false;
#endif
}

bool UReadyOrNotGameInstance::SetInputAudioDevice(FString DeviceName, bool bShouldSave)
{
#if defined(WITH_VIVOX) && !WITH_EDITOR
	for (auto k : VivoxVoiceClient->AudioInputDevices().AvailableDevices())
	{
		if (k.Value->Name() == DeviceName)
		{
			VivoxVoiceClient->AudioInputDevices().SetActiveDevice(*k.Value);
			if (bShouldSave)
			{
				UBpGameplayHelperLib::SaveSelectedAudioDevice(DeviceName);
			}
			return true;
		}
	}
#endif
	return false;
}

void UReadyOrNotGameInstance::SetInputVolume(float InputVolume)
{
#if defined(WITH_VIVOX)
	if (VivoxVoiceClient)
	{
		V_LOGM(LogReadyOrNot, "Setting Input Volume to %f", (InputVolume - 1.0f) * 25.0f);
		VivoxVoiceClient->AudioInputDevices().SetVolumeAdjustment((InputVolume - 1.0f) * 25.0f);
	}
#endif
}

void UReadyOrNotGameInstance::SetOutputVolume(float OutputVolume)
{
#if defined(WITH_VIVOX)
	if (VivoxVoiceClient)
	{
		VivoxVoiceClient->AudioOutputDevices().SetVolumeAdjustment((OutputVolume - 1.0f) * 25.0);
	}
#endif
}

#if ENABLE_ANTI_PIRACY_CHECKS
FCheckForSteamEmuThreadWorker::FCheckForSteamEmuThreadWorker(class UReadyOrNotGameInstance* GameInstance)
{
	GameInstanceReference = GameInstance;
	Thread = FRunnableThread::Create(this, TEXT("TesCF"), 0, TPri_Lowest); // TesCF = FCheckForSteamEmuThread backwards
}

FCheckForSteamEmuThreadWorker::~FCheckForSteamEmuThreadWorker()
{
	delete Thread;
	Thread = nullptr;
}

bool FCheckForSteamEmuThreadWorker::Init()
{
	return true;
}

uint32 FCheckForSteamEmuThreadWorker::Run()
{
	// Run every 5 seconds
	while (true)
	{
		if (StopTaskCounter.GetValue() == 0)
		{
			if (GameInstanceReference)
			{
				GameInstanceReference->CheckForSteamEmus();
			}
			if (StopTaskCounter.GetValue() != 0)
				return 0;
				
			FPlatformProcess::Sleep(5.0f);
		}
		else
		{
			return 0;
		}
	}
}

void FCheckForSteamEmuThreadWorker::Stop()
{
	StopTaskCounter.Increment();
}

void FCheckForSteamEmuThreadWorker::StopInstantly()
{
	Stop();
	Thread->Kill(false);
}

void FCheckForSteamEmuThreadWorker::EnsureCompletion()
{
	Stop();
	Thread->WaitForCompletion();
}
#endif

UReadyOrNotGameInstance::UReadyOrNotGameInstance(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	LoadoutManagerClass = ULoadoutManager::StaticClass();
	
	OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &UReadyOrNotGameInstance::OnFindSessionsComplete);
	OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &UReadyOrNotGameInstance::OnJoinSessionComplete);
#if WITH_EDITOR
	//##UE5.3UPGRADE##
	//FEditorDelegates::PreSaveWorldWithContext.AddUObject(this, &UReadyOrNotGameInstance::OnWorldPresave);
	//##UE5.3UPGRADE##
#endif
	
#if WITH_GAMEPLAY_DEBUGGER
	IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
	GameplayDebuggerModule.RegisterCategory("ReadyOrNot", IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_ReadyOrNot::MakeInstance), EGameplayDebuggerCategoryState::EnabledInGame, 5);
	V_LOGM(LogReadyOrNot, "Registered Ready Or Not Debug Category");
#endif

#if !UE_EDITOR && defined(WITH_VIVOX)
	VivoxVoiceClient = nullptr;
	bInitialized = false;
	bVivoxLoggedIn = false;
	bVivoxLoggingIn = false;
	VivoxVoiceClient = &static_cast<FVivoxCoreModule *>(&FModuleManager::Get().LoadModuleChecked(TEXT("VivoxCore")))->VoiceClient();
#endif
	// From merge
	// Main
	LoadReadyOrNotStringTables();
	// 1.1
	//if (!FStringTableRegistry::Get().FindStringTable("ActionPromptTable"))
	//{
	//	CreateActionPromptTable();
	//}
	//if (!FStringTableRegistry::Get().FindStringTable("TooltipTable"))
	//{
	//	CreateTooltipStringTable();
	//}
}

UReadyOrNotGameInstance::~UReadyOrNotGameInstance()
{
#if ENABLE_ANTI_PIRACY_CHECKS
	if (SteamEmuThreadWorker)
	{
		bWantsRunPThread = false;
		SteamEmuThreadWorker->StopInstantly();
		SteamEmuThreadWorker = nullptr;
	}
#endif
#if !UE_EDITOR && defined(WITH_VIVOX)
	FModuleManager::Get().UnloadModule(TEXT("VivoxCore"));
	Logout();
#endif
}

bool UReadyOrNotGameInstance::OnWindowCloseRequested()
{
#if UE_BUILD_DEVELOPMENT
	V_LOGM(LogReadyOrNot, "Processed Window Close Request");
#endif
	FPlatformMisc::RequestExit(true);
	return false;
}

#if defined(WITH_MODIO)
void UReadyOrNotGameInstance::EnableModManager()
{
	if (ModioManager)
		ModioManager->Initialize(this);
}

void UReadyOrNotGameInstance::DisableModManager()
{
	if (ModioManager)
		ModioManager->Shutdown();
}

void UReadyOrNotGameInstance::MountInstalledMods()
{
	if (ModioManager)
		ModioManager->MountInstalledMods();
}
#endif

void UReadyOrNotGameInstance::Init()
{
	Super::Init();

#if defined(WITH_MODIO)	
	if(UModioManager::IsPackagedBuild())
	{
		FString PlatformFileName = FPlatformFileManager::Get().GetPlatformFile().GetName();
		if (PlatformFileName.Equals(FString(TEXT("PakFile"))))
		{
			PakPlatform = static_cast<FPakPlatformFile*>(&FPlatformFileManager::Get().GetPlatformFile());
			PakPlatform->InitializeNewAsyncIO();
		}
		else
		{
			PakPlatform = new FPakPlatformFile();
			PakPlatform->InitializeNewAsyncIO();
			if (!PakPlatform->Initialize(&FPlatformFileManager::Get().GetPlatformFile(), TEXT("")))
			{
				UE_LOG(LogTemp, Error, TEXT("FPakPlatformFile failed to initialize"));
				return;
			}
			FPlatformFileManager::Get().SetPlatformFile(*PakPlatform);
		}
	}
#endif	

	ReadyOrNotBackend = NewObject<UReadyOrNotBackend>(this);

#if WITH_EDITOR
	// Editor only version, packaged game creates the LoadoutManager in ::StartGameInstance()
	LoadoutManager = NewObject<ULoadoutManager>(this, LoadoutManagerClass);
	LoadoutManager->Initialize();
#endif

#if defined(WITH_MODIO)
	if (!GIsAutomationTesting)
	{
		if (!ModioManager)
		{
			ModioManager = NewObject<UModioManager>(this);
			ModioManager->Initialize(this);
		}
	}
#endif

#if !UE_EDITOR && defined(WITH_VIVOX)
	if (!AreVivoxVoiceChatValuesSet())
	{
		FText TitleText = FText::FromString(FString("Warning: Vivox Portal Credentials Misconfigured!"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString("Voice Chat will not work properly until your Developer Portal credential values are defined correctly at the top of 'Vivox/VivoxGameInstance.cpp' AND 'Vivox/VivoxToken.cpp'. See the online documentation 'Vivox Unreal: Developer First Steps' for more info.")), &TitleText);
	}

	VivoxCoreError Status = Initialize(1); // Logs Core Errors and Warnings only
#endif

#if ENABLE_ANTI_PIRACY_CHECKS && !UE_SERVER
	SteamEmuThreadWorker = new FCheckForSteamEmuThreadWorker(this);
	bWantsRunPThread = true;
#endif

#if UE_BUILD_SHIPPING && ENABLE_ANTI_PIRACY_CHECKS && PLATFORM_WINDOWS
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		if (FCString::Atoi(*OnlineSub->GetAppId()) == 480 || FCString::Atoi(*OnlineSub->GetAppId()) == 0)
		{
			AActor* MyNullPtr = nullptr;
			MyNullPtr->SetActorLocation(FVector::ZeroVector);
		}
	}
#endif
	
	GenerateURLMap();

	FNetworkVersion::GetLocalNetworkVersionOverride.BindUObject(this, &UReadyOrNotGameInstance::GetLocalNetworkVersion);
	FNetworkVersion::IsNetworkCompatibleOverride.BindUObject(this, &UReadyOrNotGameInstance::IsNetworkCompatible);
	FWorldDelegates::OnPostWorldCleanup.AddUObject(this, &UReadyOrNotGameInstance::OnPostWorldCleanup);
	FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &UReadyOrNotGameInstance::OnWorldInitalized);
	FWorldDelegates::OnPreWorldInitialization.AddUObject(this, &UReadyOrNotGameInstance::OnPreWorldInitialization);
	FWorldDelegates::LevelAddedToWorld.AddUObject(this, &UReadyOrNotGameInstance::OnLevelChanged);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UReadyOrNotGameInstance::PostLoadMap);
	FCoreDelegates::OnPreExit.AddUObject(this, &UReadyOrNotGameInstance::OnPreExit);
	FCoreDelegates::OnUnmountPak.BindUObject(this, &UReadyOrNotGameInstance::OnUnmountPak);

	
	OnConnectSteamServerByIP.AddDynamic(this, &UReadyOrNotGameInstance::OnConnectSteamServer);


	FPlatformMisc::SetEnvironmentVar(TEXT("SteamAppId"), ToCStr(FString::FormatAsNumber(STEAM_APP_ID)));
	FPlatformMisc::SetEnvironmentVar(TEXT("SteamGameId"), ToCStr(FString::FormatAsNumber(STEAM_APP_ID)));

	GEngine->OnNetworkFailure().RemoveAll(GEngine);
	GEngine->OnNetworkFailure().AddUObject(this, &UReadyOrNotGameInstance::HandleNetworkFailure);
	GEngine->OnTravelFailure().AddUObject(this, &UReadyOrNotGameInstance::HandleTravelFailure);

	AIConfig = NewObject<UReadyOrNotAIConfig>(this, UReadyOrNotAIConfig::StaticClass());
	AIConfig->ReloadConfig();

	MetaGameProfile = UMetaGameProfile::LoadProfile();
}

void UReadyOrNotGameInstance::EOSLoginComplete(int32 LocalUserNum, bool WasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
#if defined(TARGET_CONSOLE) || defined(USE_EOS)
	UE_LOG(LogTemp, Warning, TEXT("EOSLoginComplete"));

#if defined(USE_EOS)
	if (WasSuccessful) {
		APlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
		if (pc && pc->PlayerState) {
			pc->PlayerState->SetUniqueId(FUniqueNetIdRepl(UserId));
		}
	}
#endif
#endif
}

void UReadyOrNotGameInstance::NativeLoginComplete(int32 LocalUserNum, bool WasSuccessful, const FUniqueNetId& UserId, const FString& Error) 
{
    UE_LOG(LogTemp, Warning, TEXT("NativeLoginComplete"));

#if defined(TARGET_CONSOLE)
	// Need to login to EOS to use Relay for Console
	IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld(), EOSName);
#else
	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
#endif
	if (OSS) {
	    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();

		if (Identity.IsValid()) {
		  FOnlineAccountCredentials AccountCredentials;
		  AccountCredentials.Id = TEXT("");
		  AccountCredentials.Token = TEXT("");
		  AccountCredentials.Type = TEXT("accountportal");

		  OnEOSLoginCompleteDelegate = FOnLoginCompleteDelegate::CreateUObject(this, &UReadyOrNotGameInstance::EOSLoginComplete);
		  OnEOSLoginCompleteDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(0, OnEOSLoginCompleteDelegate);

		  Identity->Login(0, AccountCredentials);
		}
    } else {
        UE_LOG(LogTemp, Warning, TEXT("Could not get Online SubSystem"));
    }
}

void UReadyOrNotGameInstance::StartGameInstance()
{
	Super::StartGameInstance();

#if defined(TARGET_CONSOLE) || defined(USE_EOS)
#if PLATFORM_XSX || PLATFORM_XBOXONEGDK
    IOnlineSubsystem* NativeOSS = IOnlineSubsystem::Get();
    if (NativeOSS) 
	{
		IOnlineIdentityPtr Identity = NativeOSS->GetIdentityInterface();
		if (Identity.IsValid()) {
			FOnlineAccountCredentials AccountCredentials;
			AccountCredentials.Id = TEXT("");
			AccountCredentials.Token = TEXT("");
			AccountCredentials.Type = TEXT("accountportal");

			OnNativeLoginCompleteDelegate = FOnLoginCompleteDelegate::CreateUObject(this, &UReadyOrNotGameInstance::NativeLoginComplete);
            OnNativeLoginCompleteDelegateHandle = Identity->AddOnLoginCompleteDelegate_Handle(0, OnNativeLoginCompleteDelegate);

			Identity->Login(0, AccountCredentials);
		}
    }
#else
	APlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
    NativeLoginComplete(0, true, *pc->PlayerState->GetUniqueId().GetUniqueNetId(), TEXT(""));
#endif
#endif

	ReadyOrNotBackend->StartLogin();

	HostMigrationManager = NewObject<UHostMigrationManager>(this);
	HostMigrationManager->Init();

	UGameViewportClient* ViewportClient = GetGameViewportClient();
	if (ViewportClient)
	{
		ViewportClient->OnWindowCloseRequested().BindUObject(this, &UReadyOrNotGameInstance::OnWindowCloseRequested);
#if UE_BUILD_DEVELOPMENT
		V_LOGM(LogReadyOrNot, "Bound Window Close Request");
#endif
	}

#if !defined(TARGET_CONSOLE)
	BuildChecksum();

	// Build checksum currently loads mod paks, LoadModdedLevelData needs the paks loaded
	LoadModdedLevelData();
#endif

#if !WITH_EDITOR
	// Do this after checksum is built so mod files are discovered. StartGameInstance is only called in packaged!
	LoadoutManager = NewObject<ULoadoutManager>(this, LoadoutManagerClass);
	LoadoutManager->Initialize();
#endif

#if !WITH_EDITOR
	ReadyOrNotLogOutput = new FReadyOrNotBackendOutputDevice();
	FOutputDeviceRedirector::Get()->AddOutputDevice(ReadyOrNotLogOutput);
#endif
}

ULocalPlayer* UReadyOrNotGameInstance::CreateInitialPlayer(FString& OutError)
{
	ULocalPlayer* LocalPlayer = Super::CreateInitialPlayer(OutError);
	return LocalPlayer;
}

void UReadyOrNotGameInstance::Shutdown()
{
	Super::Shutdown();
	
#if defined(WITH_VIVOX)
	Logout();
#endif

	if (ReadyOrNotBackend)
	{
		ReadyOrNotBackend->Logout();
	}

#if defined(WITH_MODIO)
	if (ModioManager)
	{
		ModioManager->Shutdown();
	}
#endif

	bIsBuildPirated = false;

	#if ENABLE_ANTI_PIRACY_CHECKS
	if (SteamEmuThreadWorker)
	{
		bWantsRunPThread = false;
		SteamEmuThreadWorker->StopInstantly();
		SteamEmuThreadWorker = nullptr;
	}
	#endif
}

void UReadyOrNotGameInstance::OnPreExit()
{
	// Nothing to do here now
	// Maybe remove safe mode flag
}

bool UReadyOrNotGameInstance::OnUnmountPak(const FString& PakFile)
{
	UE_LOG(LogReadyOrNot, Warning, TEXT("Unmount Pak File %s"), *PakFile);
	return true;
}

bool UReadyOrNotGameInstance::IsPublicMissionInProgress()
{
	return false;
	// AReadyOrNotGameMode* gm = Cast<AReadyOrNotGameMode>(GetWorld()->GetAuthGameMode());
	// return gm && gm->GetMatchState() == EMatchState::MS_Playing && !Cast<ALobbyGM>(gm) && !UReadyOrNotFunctionLibrary::IsSinglePlayer(GetWorld()) && SessionType == ESessionType::ST_Public;
}

void UReadyOrNotGameInstance::OnPostWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	LazyLoadedClasses.Empty();
	LazyLoadedObjects.Empty();
}

void UReadyOrNotGameInstance::PostLoadMap(UWorld* World)
{
}

void UReadyOrNotGameInstance::OnWorldInitalized(const UWorld::FActorsInitializedParams& Params)
{
	Params.World->OnWorldBeginPlay.RemoveAll(this);
	Params.World->OnWorldBeginPlay.AddUObject(this, &UReadyOrNotGameInstance::OnWorldBeginPlay);
	
	//ApplyWorldSettings();
}

void UReadyOrNotGameInstance::OnPreWorldInitialization(UWorld* World, const UWorld::InitializationValues IVS)
{
	// use the ready or not nav sys
	{
		ULevel* LevelObject = World->GetCurrentLevel();
		if (LevelObject && LevelObject->GetWorld() && LevelObject->GetWorldSettings(false))
		{
			LevelObject->GetWorldSettings(false)->GetNavigationSystemConfig()->NavigationSystemClass = UReadyOrNotNavigationSystem::StaticClass();
		}
	}
	
	// try destroy duplicate world settings
	{
		TArray<AActor*> Settings;
		UGameplayStatics::GetAllActorsOfClass(World, AWorldSettings::StaticClass(), Settings);

		for (int32 i = 1; i < Settings.Num(); i++)
		{
			if (AWorldSettings* s = Cast<AWorldSettings>(Settings[i]))
			{
				s->Destroy();
			}
		}
	}
}

void UReadyOrNotGameInstance::OnLevelChanged(ULevel* Level, UWorld* World)
{
	//ApplyWorldSettings();
}

void UReadyOrNotGameInstance::ApplyWorldSettings()
{
	if (!GetWorld() || (GetWorld() && GetWorld()->bIsTearingDown))
		return;

	for (TActorIterator<APostProcessVolume>It(GetWorld()); It; ++It)
    {
    	It->Settings.bOverride_BloomMethod = false;
    	It->Settings.BloomMethod = BM_SOG;
    }

#if !defined(WITH_EDITOR)
	for (TActorIterator<ACineCameraActor>It(GetWorld()); It; ++It)
	{
		if(It->GetCineCameraComponent())
		{
			It->GetCineCameraComponent()->PostProcessSettings.bOverride_BloomMethod = false;
			It->GetCineCameraComponent()->PostProcessSettings.BloomMethod = BM_SOG;
		}
	}
#endif


//#if defined(WITH_VICODYNAMICS)
//	for (TObjectIterator<UVDMeshClothComponent>It; It; ++It)
//	{
//		if (It->GetOwner() && It->GetWorld() == GetWorld())
//		{
//			It->GetOwner()->Destroy();
//		}
//	}
//#endif


    if (GetWorld()->IsGameWorld())
    {
    	/*GetWorld()->PersistentLevel->PrecomputedVisibilityHandler.Invalidate(GetWorld()->Scene);
#if !WITH_EDITOR
    	// Copy precomputed visibility from the _Core level if it exists
    	TArray<ULevelStreaming*> StreamedLevels = GetWorld()->GetStreamingLevels();
    	for (ULevelStreaming* Level : StreamedLevels)
    	{
    		UWorld* SubWorld = Level->GetWorldAsset().LoadSynchronous();
    		if (SubWorld && SubWorld->GetMapName().EndsWith("_Core"))
    		{
    			if (SubWorld->PersistentLevel->PrecomputedVisibilityHandler.GetId() != GetWorld()->PersistentLevel->PrecomputedVisibilityHandler.GetId())
    			{
    				GetWorld()->PersistentLevel->PrecomputedVisibilityHandler = SubWorld->PersistentLevel->PrecomputedVisibilityHandler;
    				GetWorld()->PersistentLevel->PrecomputedVisibilityHandler.UpdateVisibilityStats(true);
    				GetWorld()->PersistentLevel->PrecomputedVisibilityHandler.UpdateScene(GetWorld()->Scene);
    				GetWorld()->Modify(true);
    			}
               
    		}
    	}
#endif#1#*/
    	AReadyOrNotLevelScript* LevelScript = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor());
    	if (LevelScript)
    	{
    		if (UReadyOrNotFunctionLibrary::IsDLSSEnabled())
    		{
    			UReadyOrNotGameUserSettings* Settings = UReadyOrNotFunctionLibrary::GetReadyOrNotGameUserSettings();
    			if (Settings && Settings->DlssQualitySetting != 0)
    			{
    				for (TActorIterator<APostProcessVolume>It(GetWorld()); It; ++It)
    				{
    					It->Settings.bOverride_BloomMethod = false;
    					for (int32 i = 0; i < It->Settings.WeightedBlendables.Array.Num(); i++)
    					{
    						if (It->Settings.WeightedBlendables.Array[i].Object == LevelScript->TAASharpenFilter)
    						{
    							It->Settings.WeightedBlendables.Array.RemoveAt(i);
    							i--;
    						}
    					}
    				}
    			}
    		}
    	}
    }

	int32 ReplicatingPhysicsActors = 0;
	for (TObjectIterator<UStaticMeshComponent> It; It; ++It)
	{
		UStaticMeshComponent* Component = *It;
		if (Component->IsTemplate())
			continue;
		
		if (Component->GetWorld() != GetWorld())
			continue;

		if (!Component->GetOwner())
			continue;
		
		if (Cast<ADestructibleVehicle>(Component->GetOwner()))
			continue;
			
		if (Component->IsSimulatingPhysics())
		{
			if (Component->GetOwner()->HasAuthority())
			{
				Component->bNavigationRelevant = false;
				Component->SetCanEverAffectNavigation(false);
				Component->GetOwner()->NetUpdateFrequency = 30.0f;
				Component->GetOwner()->MinNetUpdateFrequency = 1.0f;
				Component->GetOwner()->NetCullDistanceSquared = 5250000;
				Component->GetOwner()->SetReplicates(true);
				Component->GetOwner()->SetReplicateMovement(true);
				ReplicatingPhysicsActors++;
			}
		   
			Component->CanCharacterStepUpOn = ECB_Yes;
			Component->PutRigidBodyToSleep();
		}
		
		if (!Component->GetOwner<ADoor>() && Component->Mobility == EComponentMobility::Movable)
		{
			//V_LOGM(LogReadyOrNot, "Marking %s (Owner: %s) not navigationally relevant", *It->GetName(), *It->GetOwner()->GetName());
			Component->bNavigationRelevant = false;
			Component->SetCanEverAffectNavigation(false);
		}
	}

	V_LOGM(LogReadyOrNot, "Total Replicating Physics Actors: %d", ReplicatingPhysicsActors);
	
	for (TObjectIterator<UStaticMeshComponent>It; It; ++It)
	{
		if (It->GetWorld() == GetWorld())
		{
			ADoor* Door = Cast<ADoor>(It->GetOwner());
			if (Cast<ADestructibleVehicle>(It->GetOwner()))
				continue;
			if (!Door && It->Mobility == EComponentMobility::Movable)
			{
				//V_LOGM(LogReadyOrNot, "Marking %s (Owner: %s) not navigationally relevant", *It->GetName(), *It->GetOwner()->GetName());
				It->bNavigationRelevant = false;
				It->SetCanEverAffectNavigation(false);
			}
		}
	}

	ApplyDecalSettings();
}

void UReadyOrNotGameInstance::OnWorldBeginPlay()
{
	ApplyWorldSettings();
}

void UReadyOrNotGameInstance::ApplyDecalSettings()
{
	bool bEnabled;
	float DecalFadeScreenSize, DecalDensity;
	UBpVideoSettingsLib::GetWorldDecalsEnabled(bEnabled, DecalFadeScreenSize, DecalDensity);
	DecalFadeScreenSize = 1.0f - DecalFadeScreenSize;
	for (TActorIterator<ADecalActor>It(GetWorld()); It; ++It)
	{
		ADecalActor* Decal = *It;
		
		Decal->SetActorHiddenInGame(false);
		if (!bEnabled)
		{
			Decal->SetActorHiddenInGame(true);
		} else
		{
			Decal->GetDecal()->SetFadeScreenSize(DecalFadeScreenSize);
		}
	}
}

bool UReadyOrNotGameInstance::IsSafeMode() const
{
#if defined(ENABLE_SAFE_PAK_MODE)
	if(PakPlatform && PakPlatform->IsRunningInSafeMode())
		return true;
#endif
	return false;
}

bool UReadyOrNotGameInstance::SupportsDisablingMods() const
{
#if defined(ENABLE_SAFE_PAK_MODE)
	return true;
#endif
	return false;
}

void UReadyOrNotGameInstance::BuildChecksum()
{
	Checksum = 0;
	// IPlatformFile& InnerPlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	// FPakPlatformFile* PakPlatformFile = new FPakPlatformFile();
	// PakPlatformFile->Initialize(&InnerPlatformFile, TEXT(""));
		
	TArray<FString> MountedPaks;
	if(!PakPlatform)
		return;
	PakPlatform->GetMountedPakFilenames(MountedPaks);

#if UE_BUILD_DEVELOPMENT
	V_LOGM(LogReadyOrNot, "Found %d Pak Files", MountedPaks.Num());
#endif
	
	//ignore first pak, because it is default game pak and we don't need iterate over it
	for (int32 i = 0; i < MountedPaks.Num(); i++)
	{

		Checksum += PakPlatform->FileSize(*MountedPaks[i]);
		FString FolderPath, FileName;
		MountedPaks[i].Split(TEXT("/"), &FolderPath, &FileName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

		if (!VALID_GAME_PAK_FILES.Contains(FileName))
		{
#if UE_BUILD_DEVELOPMENT
			V_LOGM(LogReadyOrNot, "PakFile %s does not match, setting modded to true", *FileName);
#endif
			bIsModded = true;
			LoadModDataFromPak(MountedPaks[i]);
		}
	}
	// add a random buffer for the amount of files
	Checksum += MountedPaks.Num() * 76849;

#if UE_BUILD_DEVELOPMENT
	V_LOGM(LogReadyOrNot, "Generated %d As Checksum IsModded? %d", Checksum, bIsModded);
#endif


}

void UReadyOrNotGameInstance::LoadModDataFromPak(const FString& InPakFile)
{
	// find all modded maps
	TArray<FString> Files;
	PakPlatform->GetPrunedFilenamesInPakFile(InPakFile, Files);
	
	for (FString File : Files)
	{
		if (File.Contains(".umap"))
		{
			FString MapPath = File;
			MapPath = MapPath.Replace(TEXT(".umap"), TEXT(""));
			FString OutL, MapName;
			MapPath.Split("/", &OutL, &MapName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			V_LOGM(LogReadyOrNot, "Found Map %s in Pak File %s", *MapName, *InPakFile);
			BuiltModdedMapList.AddUnique(MapName);
		}

		if (File.Contains("AILevelData.ini"))
		{
			bNoScoring = true;
		}
	}

	// Attempt to tell the asset registry about files in the pak, for faster loading
	if (FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
	{
		IAssetRegistry& AssetRegistry = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		AssetRegistry.ScanFilesSynchronous(Files);
	}
}

void UReadyOrNotGameInstance::LoadModdedLevelData()
{
	UAssetManager& AssetManager = UAssetManager::Get();
	
	TArray<FAssetData> DataAssets;
	//AssetManager.GetPrimaryAssetDataList("ModLevelData", DataAssets);
	AssetManager.GetAssetRegistry().GetAssetsByClass(UModLevelData::StaticClass()->GetFName(), DataAssets, true);
	
	ModdedLevelData.Empty();
	ModdedLevelDataAssets.Empty();
	for (const FAssetData& AssetData : DataAssets)
	{
		UModLevelData* LevelData = Cast<UModLevelData>(AssetData.GetAsset());
		if (!ensure(LevelData))
			continue;
		
		// Ignore empty maps
		if (LevelData->LevelName.IsNone())
			continue;
		
		ModdedLevelData.Add(LevelData->LevelName, LevelData->Data);
		ModdedLevelDataAssets.Add(LevelData);
	}

	// Create data for levels that don't include level data assets
	for (const FString& Level : BuiltModdedMapList)
	{
		FName LevelName = FName(Level);
		if (ModdedLevelData.Contains(LevelName))
			continue;

		FLevelDataLookupTable LevelData = ModdedMapLookUpData;
		LevelData.LevelNickname = FText::FromString(Level);
		LevelData.FriendlyLevelName = FText::FromString(Level);

		UModLevelData* LevelDataObject = NewObject<UModLevelData>();
		LevelDataObject->LevelName = LevelName;
		LevelDataObject->Data = LevelData;
		
		ModdedLevelData.Add(LevelName, LevelData);
		ModdedLevelDataAssets.Add(LevelDataObject);
	}
}

uint32 UReadyOrNotGameInstance::GetLocalNetworkVersion()
{
	FString ProjectVersionStr = UBpGameplayHelperLib::GetProjectVersion();
	ProjectVersionStr = ProjectVersionStr.Replace(TEXT("."), TEXT(""));
	int32 nvn = FCString::Atoi(*ProjectVersionStr);
	V_LOGM(LogReadyOrNot, "Using NetworkVersionOveride: NETWORK_VERSION_NUMBER: %d", nvn);
	return nvn;
}

bool UReadyOrNotGameInstance::IsNetworkCompatible(const uint32 LocalNetworkVersion, const uint32 RemoteNetworkVersion)
{
#if !UE_BUILD_SHIPPING
	return true;
#endif
	return LocalNetworkVersion == RemoteNetworkVersion;
}

FString UReadyOrNotGameInstance::GetSessionTicket()
{
	IOnlineSubsystem* OnlineInterface = IOnlineSubsystem::Get();
	if (OnlineInterface)
	{
		return OnlineInterface->GetIdentityInterface()->GetAuthToken(0);
	}
	return "";
}

FStreamableManager& UReadyOrNotGameInstance::GetStreamableManager()
{
	return UAssetManager::GetStreamableManager();
}

void UReadyOrNotGameInstance::Gratr()
{
	UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_Gratr, this, &UReadyOrNotGameInstance::Gratr_Everything, 0.15f, true, true);
}

void UReadyOrNotGameInstance::Gratr_Everything()
{
	for (TObjectIterator<UTextBlock> It; It; ++It)
	{
		UTextBlock* TextBlock = *It;
		if (IsValid(TextBlock))
		{
			TextBlock->SetText(FText::FromString("gratr"));
		}
	}

	for (TObjectIterator<UInteractableComponent> It; It; ++It)
	{
		UInteractableComponent* InteractableComponent = *It;
		if (IsValid(InteractableComponent) && !InteractableComponent->IsTemplate())
		{
			InteractableComponent->ActionSlot1.ActionText = FText::FromString("gratr");
			InteractableComponent->ActionSlot2.ActionText = FText::FromString("gratr");
			InteractableComponent->ActionSlot3.ActionText = FText::FromString("gratr");
			InteractableComponent->ActionSlot4.ActionText = FText::FromString("gratr");
		}
	}

	/*for (TObjectIterator<URichTextBlock> It; It; ++It)
	{
		URichTextBlock* RichTextBlock = *It;
		if (IsValid(RichTextBlock))
		{
			RichTextBlock->SetText(FText::FromString("gratr"));
		}
	}*/
}

TArray<AReadyOrNotGameMode*> UReadyOrNotGameInstance::GetAllGameModes()
{
	TArray<AReadyOrNotGameMode*> OutGameModes;
	auto ObjectLibrary = UObjectLibrary::CreateLibrary(UBlueprint::StaticClass(), true, true);
	ObjectLibrary->LoadBlueprintsFromPath("/Game/Blueprints/Games");
	for (TObjectIterator<UClass>It; It; ++It)
	{
		AReadyOrNotGameMode* gm = Cast<AReadyOrNotGameMode>(It->GetDefaultObject());
		if (gm)
		{
			OutGameModes.Add(gm);
		}
	}
	return OutGameModes;
}

TArray<AReadyOrNotGameState*> UReadyOrNotGameInstance::GetAllGameStates()
{
	TArray<AReadyOrNotGameState*> OutGameStates;
	auto ObjectLibrary = UObjectLibrary::CreateLibrary(UBlueprint::StaticClass(), true, true);
	ObjectLibrary->LoadBlueprintsFromPath("/Game/Blueprints/Games");
	for (TObjectIterator<UClass>It; It; ++It)
	{
		AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(It->GetDefaultObject());
		if (gs)
		{
			OutGameStates.Add(gs);
		}
	}
	return OutGameStates;
}

void UReadyOrNotGameInstance::GenerateURLMap()
{
	UReadyOrNotGameInstance* gi = Cast<UReadyOrNotGameInstance>(GetWorld()->GetGameInstance());
	TArray<AReadyOrNotGameMode*> Modes = gi->GetAllGameModes();
	for (AReadyOrNotGameMode* m : Modes)
	{
		if (m->GameStateClass && m->GameStateClass != AReadyOrNotGameState::StaticClass() && m->GameStateClass != AGameStateBase::StaticClass())
		{
			if ( m->GameStateClass->GetDefaultObject<AReadyOrNotGameState>())
			{
				UrlToModeNameMap.Add(m->urlShortName, m->GameStateClass->GetDefaultObject<AReadyOrNotGameState>()->ModeName.ToString());
			}
		}
	}
}

void UReadyOrNotGameInstance::SetPresenceForLocalPlayers(const FString& StatusStr, FOnlineKeyValuePairs<FPresenceKey, FVariantData> PresenceProperties)
{
	const auto Presence = Online::GetPresenceInterface();
	if (Presence.IsValid())
	{
		for (int i = 0; i < LocalPlayers.Num(); ++i)
		{
			const FUniqueNetIdRepl UserId = LocalPlayers[i]->GetPreferredUniqueNetId();

			if (UserId.IsValid())
			{
				FOnlineUserPresenceStatus PresenceStatus;
				PresenceStatus.StatusStr = StatusStr;
				PresenceStatus.Properties = PresenceProperties;
				PresenceStatus.State = EOnlinePresenceState::Online;

				Presence->SetPresence(*UserId, PresenceStatus);
			}
		}
	}
}

void UReadyOrNotGameInstance::OnWorldPresave(uint32 Id, UWorld* World)
{
#if WITH_EDITOR
	if (World)
	{
		TArray<ULevelStreaming*> StreamedLevels = World->GetStreamingLevels();
		for (ULevelStreaming* Level : StreamedLevels)
		{
			UWorld* SubWorld = Level->GetWorldAsset().LoadSynchronous();
			if (SubWorld && SubWorld->GetMapName().EndsWith("_Core"))
			{
				if (SubWorld->PersistentLevel->PrecomputedVisibilityHandler.GetId() != World->PersistentLevel->PrecomputedVisibilityHandler.GetId())
				{
					// if the subworld name length is shorter then its the one we want to copy from
					// ie. RON_GAS_CORE vs RON_GAS_BARRICADEDSUSPECTS_CORE
					// if ron_gas_core is the persistent level then we'll want to copy it into all of the sublevels _Core persistent data
					// else if ron_gas_barricadedsuspects_core is the persistent level then we'll want to copy into that
					if (SubWorld->GetMapName().Len() < World->GetMapName().Len())
					{
						World->PersistentLevel->PrecomputedVisibilityHandler = SubWorld->PersistentLevel->PrecomputedVisibilityHandler;
						World->PersistentLevel->PrecomputedVisibilityHandler.UpdateVisibilityStats(true);
						World->PersistentLevel->PrecomputedVisibilityHandler.UpdateScene(World->Scene);
						World->Modify(true);
					}
				}
               
			}
		}
       
	}
#endif
}

void UReadyOrNotGameInstance::LoadComplete(const float LoadTime, const FString& MapName)
{
	Super::LoadComplete(LoadTime, MapName);
}

void UReadyOrNotGameInstance::ConnectSteamServer(FString serverConnect)
{
#if defined(WITH_STEAM)
	FString IPString;
	FString PortString;
	serverConnect.Split(TEXT(":"), &IPString, &PortString);

	uint16 Port = FCString::Atoi(*PortString);
	uint32 Address = 0;
#if PLATFORM_WINDOWS
	// Address = inet_addr(TCHAR_TO_UTF8(*IPString));
#endif
	// https://stackoverflow.com/questions/21038120/how-to-reverse-byte-of-a-hexadecimal-number
	uint32 AddressHostOrder = ((Address & 0x000000FF) << 24) | ((Address & 0x0000FF00) << 8) | ((Address & 0x00FF0000) >> 8) | ((Address & 0xFF000000) >> 24);
	if (SteamMatchmakingServers())
	{
		V_LOGM(LogReadyOrNot, "Pinging %s:%s", *IPString, *PortString);
		SteamMatchmakingServers()->PingServer(AddressHostOrder, Port, this);
	}
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->DestroySession(NAME_GameSession);
		}
	}
#else
	V_LOGM(LogReadyOrNot, "ConnectSteamServer() called for a build without Steam support!");
#endif
}

#if defined(WITH_STEAM)
void UReadyOrNotGameInstance::ServerResponded(gameserveritem_t& server)
{

	V_LOGM(LogReadyOrNot, "Server Responded To Ping");
	FString ConnectionAddr = server.m_NetAdr.GetConnectionAddressString();
	char steamidbuf[512];
	snprintf(steamidbuf, sizeof(steamidbuf), "%llu", server.m_steamID.ConvertToUint64());
	FString steamId(steamidbuf);
	FString ip, port, url;
	ConnectionAddr.Split(":", &ip, &port);
	url = "steam." + steamId + "?port=" + port;
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
 			APlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
			if (pc && pc->PlayerState)
			{
				const FUniqueNetId& MyNetID = *pc->PlayerState->GetUniqueId().GetUniqueNetId();
				SessionSearch = MakeShareable(new FOnlineSessionSearch());
				SessionSearch->bIsLanQuery = false;
				SessionSearch->MaxSearchResults = 2000;

				SessionSearch->QuerySettings.Set("OWNINGID", steamId, EOnlineComparisonOp::Equals);
				//GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Green, "Looking up servers with ID " + steamId + " from " + ip + ":" + port);
				SessionSearch->SearchResults.Empty();
				TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SessionSearch.ToSharedRef();
				OnFindSessionsCompleteDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);
				Sessions->CancelFindSessions();
				Sessions->FindSessions(MyNetID, SearchSettingsRef);
			}

		}
	}
	//OnConnectSteamServerByIP.Broadcast(url);
}

void UReadyOrNotGameInstance::ServerFailedToRespond()
{
	V_LOGM(LogReadyOrNot, "Server Failed to respond to ping");
}
#endif

void UReadyOrNotGameInstance::OnConnectSteamServer(FString url)
{
#if defined(WITH_STEAM)
	SteamFriends()->SetRichPresence("connect", TCHAR_TO_UTF8(*url));
	FString connectURL = "open " + url;
	GetWorld()->Exec(GetWorld(), *connectURL);
#else
	V_LOGM(LogReadyOrNot, "OnConnectSteamServer() called for a build without Steam support!");
#endif
}

void UReadyOrNotGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
			for (FOnlineSessionSearchResult Result : SessionSearch->SearchResults)
			{
				FString searchId;
				SessionSearch->QuerySettings.Get("OWNINGID", searchId);
				if (Result.Session.OwningUserName == searchId)
				{
					//GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Green, "Found Server with ID " + Result.Session.OwningUserName);
					AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
					if (pc)
					{
						const FUniqueNetId& MyNetID = *pc->PlayerState->GetUniqueId().GetUniqueNetId();
						OnJoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
						FString map, mode;
						Result.Session.SessionSettings.Get(SETTING_MAPNAME, map);
						Result.Session.SessionSettings.Get(SETTING_GAMEMODE, mode);
						FBlueprintSessionResult SessionResult;
						SessionResult.OnlineResult = Result;
						ULevelStreaming* OutStreamedLevel;
						pc->StreamInSession(SessionResult, OutStreamedLevel);
						break;
					}
				}
			}
		}
	}
}

void UReadyOrNotGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("OnJoinSessionComplete %s, %d"), *SessionName.ToString(), static_cast<int32>(Result)));

	// Get the OnlineSubsystem we want to work with
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		// Get SessionInterface from the OnlineSubsystem
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();

		if (Sessions.IsValid())
		{
			// Clear the Delegate again
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);

			// Get the first local PlayerController, so we can call "ClientTravel" to get to the Server Map
			// This is something the Blueprint Node "Join Session" does automatically!
			AReadyOrNotPlayerController* const PlayerController = Cast<AReadyOrNotPlayerController>(GetFirstLocalPlayerController());

			// We need a FString to use ClientTravel and we can let the SessionInterface contruct such a
			// String for us by giving him the SessionName and an empty String. We want to do this, because
			// Every OnlineSubsystem uses different TravelURLs
			FString TravelURL;

			if (PlayerController && Sessions->GetResolvedConnectString(SessionName, TravelURL))
			{
				// Finally call the ClienTravel. If you want, you could print the TravelURL to see
				// how it really looks like
				PlayerController->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
			}
		}
	}
}

FString UReadyOrNotGameInstance::GetNextLevel()
{
	return NextLevel;
}

bool UReadyOrNotGameInstance::IsSinglePlayer()
{
	return SessionType == ESessionType::ST_SinglePlayer;
}

void UReadyOrNotGameInstance::RemovePauseGameCondition_Implementation(const FString& PauseCondition)
{
}

void UReadyOrNotGameInstance::AddPauseGameCondition_Implementation(const FString&  PauseCondition)
{
}

FString UReadyOrNotGameInstance::GetAndClearMainMenuDisplayMessage()
{
	FString tempMsg = MainMenuDisplayMessage;
	MainMenuDisplayMessage = "";
	return tempMsg;
}

ELastMenuStateBeforeJoin UReadyOrNotGameInstance::GetAndClearLastJoinState()
{
	ELastMenuStateBeforeJoin tempJn = LastMenuStateBeforeJoining;
	LastMenuStateBeforeJoining = ELastMenuStateBeforeJoin::LM_None;
	return tempJn;
}

void UReadyOrNotGameInstance::SetLastJoinState(ELastMenuStateBeforeJoin LastJoiNState)
{
	LastMenuStateBeforeJoining = LastJoiNState;
}

void UReadyOrNotGameInstance::BuildMapList()
{
	// map list already built, don't rebuild...
	if (BuiltMapList.Num() > 0)
		return;

	auto ObjectLibrary = UObjectLibrary::CreateLibrary(UWorld::StaticClass(), false, true);
	ObjectLibrary->LoadAssetDataFromPath(TEXT("/Game/ReadyOrNot/Level"));
	TArray<FAssetData> AssetDatas;
	ObjectLibrary->GetAssetDataList(AssetDatas);
	UE_LOG(LogTemp, Warning, TEXT("Found maps: %d"), AssetDatas.Num());
 
	TArray<FString> MapNames = TArray<FString>();
	TArray<FString> LevelList = TArray<FString>();
 
	for (int32 i = 0; i < AssetDatas.Num(); ++i)
	{
		FAssetData& AssetData = AssetDatas[i];
 
		auto name = AssetData.AssetName.ToString();
		MapNames.Add(name);
	}
	
	BuiltMapList = MapNames;
}

TArray<FString> UReadyOrNotGameInstance::GetBuiltModdedMapList()
{
	return BuiltModdedMapList;
}

TArray<FString> UReadyOrNotGameInstance::GetBuiltMapList()
{
	if (BuiltMapList.Num() == 0)
	{
		BuildMapList();
	}
	return BuiltMapList;
}

FString UReadyOrNotGameInstance::GetBestGuessMapName(FString MapName)
{
	TArray<FString> MapList = GetBuiltMapList();
	for (int32 i = 0; i < MapList.Num(); i++)
	{
		FString MapURL = MapList[i];
		FString map, mode;
		MapURL.Split("?game=", &map, &mode, ESearchCase::IgnoreCase);
		if (map.Left(31) == MapName.Left(31))
		{
			return map;
		}
	}
	return MapName;
}

void UReadyOrNotGameInstance::HandleNetworkFailure(UWorld *World, UNetDriver *NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	FString Message = (World ? World->GetMapName() : "No World") + "\nDisconnected From Server: " + ErrorString;
	if (!MainMenuDisplayMessage.IsEmpty())
	{
		Message += "\nReason: " + MainMenuDisplayMessage; 
	}
	if (NetDriver)
	{
		Message += "\n" + NetDriver->GetDescription();
	}

	
#ifdef HOST_MIGRATION_ENABLED

	if (!HostMigrationManager)
		return;
	
	// don't fail while we're migrating
	if (HostMigrationManager->IsMigratingHost())
		return;
	
	if (FailureType == ENetworkFailure::ConnectionLost || FailureType == ENetworkFailure::ConnectionTimeout)
	{
		if (GetWorld())
		{
			if (GetWorld()->GetNetMode() == NM_Client)
			{
				StartHostMigration();
				return;
			}
		}
	}
#endif
 //	virtual bool FindSessionById(const FUniqueNetId& SearchingUserId, const FUniqueNetId& SessionId, const FUniqueNetId& FriendId, const FOnSingleSessionResultCompleteDelegate& CompletionDelegate) override;
	UReadyOrNotBackend::LogMessage(Message);
	// Don't override existing main menu display message incase we are already returning for another reason.
	if (GetWorld()->GetNetMode() == NM_Client)
	{
		if (MainMenuDisplayMessage.IsEmpty())
		{
			FString DisplayedErrorString = ErrorString;
			
			if (ErrorString.Contains("UNetConnection::Tick"))
			{
				DisplayedErrorString = XorString("Connection to the host has been lost.");
			}
			if (FailureType == ENetworkFailure::NetGuidMismatch || FailureType == ENetworkFailure::NetChecksumMismatch)
			{
				DisplayedErrorString = XorString("You have been kicked: Checksum mismatch");
			}
			MainMenuDisplayMessage = DisplayedErrorString;
		}
		
		APlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
		if (pc)
		{
			pc->ClientReturnToMainMenuWithTextReason(FText::FromString(MainMenuDisplayMessage));
		}
	}
}

void UReadyOrNotGameInstance::HandleTravelFailure(UWorld* InWorld, ETravelFailure::Type FailureType,
	const FString& ErrorString)
{
	UReadyOrNotBackend::LogMessage((InWorld ? InWorld->GetMapName() : "No World") + "\nTravel Failure: " + ErrorString);
}

void UReadyOrNotGameInstance::StartHostMigration()
{
	LeaveVoiceChannels();
	AReadyOrNotGameState* Gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (Gs)
	{
		AReadyOrNotPlayerController* PlayerController = UReadyOrNotStatics::GetReadyOrNotPlayerController();
		if (PlayerController)
		{
			if (HostMigrationManager->IsNextHost(PlayerController))
			{
				HostMigrationManager->StartMigration(true);
					
			} else
			{
				HostMigrationManager->StartMigration(false);
			}
		}
	}
}


// void URoNGameInstance::GS_IsSerialValid()
// {
// 	if (!UGameSparksModule::GetModulePtr())
// 	{
// 		UBpGameplayHelperLib::GetGameInstance()->OnSerialValidated.Broadcast(false, false, false, "Gamesparks Module Invalid.");
// 		return;
// 	}
// 	GameSparks::Core::GS& gs = UGameSparksModule::GetModulePtr()->GetGSInstance();
// 
// 	GameSparks::Api::Requests::LogEventRequest gsRequest(gs);
// 	gsRequest.SetEventKey("IS_SERIAL_VALID");
// 
// 	gsRequest.Send(OnIsSerialValid_Response);
// }
// 
// void URoNGameInstance::OnIsSerialValid_Response(GameSparks::Core::GS& gs, const GameSparks::Api::Responses::LogEventResponse& response)
// {
// 	if (!UBpGameplayHelperLib::GetGameInstance())
// 		return;
// 
// 
// 	if (!response.GetHasErrors())
// 	{
// 		bool SerialFound = response.GetBaseData().GetGSDataObject("scriptData").GetValue().GetBoolean("SERIAL_FOUND").GetValue();
// 		bool SerialValid = response.GetBaseData().GetGSDataObject("scriptData").GetValue().GetBoolean("SERIAL_VALID").GetValue();
// 		FString FailedReason(response.GetBaseData().GetGSDataObject("scriptData").GetValue().GetString("SERIAL_FAILED_REASON").GetValue().c_str());
// 		UBpGameplayHelperLib::GetGameInstance()->OnSerialValidated.Broadcast(true, SerialFound, SerialValid, FailedReason);
// 	}
// 	else
// 	{
// 		UBpGameplayHelperLib::GetGameInstance()->OnSerialValidated.Broadcast(false, false, false, "Response Invalid.");
// 	}
// }
// 
// void URoNGameInstance::GS_SaveSerial(FString Serial)
// {
// 	if (!UGameSparksModule::GetModulePtr())
// 	{
// 		UBpGameplayHelperLib::GetGameInstance()->OnSerialSaved.Broadcast(false, false, false, "Gamesparks Module Invalid.");
// 		return;
// 	}
// 	GameSparks::Core::GS& gs = UGameSparksModule::GetModulePtr()->GetGSInstance();
// 
// 	GameSparks::Api::Requests::LogEventRequest gsRequest(gs);
// 	gsRequest.SetEventKey("SAVE_SERIAL");
// 	gsRequest.SetEventAttribute("SERIAL", std::string(TCHAR_TO_UTF8(*Serial)));
// 
// 	gsRequest.Send(OnSaveSerial_Response);
// }
// 
// void URoNGameInstance::OnSaveSerial_Response(GameSparks::Core::GS& gs, const GameSparks::Api::Responses::LogEventResponse& response)
// {
// 	if (!UBpGameplayHelperLib::GetGameInstance())
// 		return;
// 
// 
// 	if (!response.GetHasErrors())
// 	{
// 		bool SerialFound = response.GetBaseData().GetGSDataObject("scriptData").GetValue().GetBoolean("SERIAL_FOUND").GetValue();
// 		bool SerialValid = response.GetBaseData().GetGSDataObject("scriptData").GetValue().GetBoolean("SERIAL_VALID").GetValue();
// 		FString FailedReason(response.GetBaseData().GetGSDataObject("scriptData").GetValue().GetString("SERIAL_FAILED_REASON").GetValue().c_str());
// 		UBpGameplayHelperLib::GetGameInstance()->OnSerialSaved.Broadcast(true, SerialFound, SerialValid, FailedReason);
// 	}
// 	else
// 	{
// 		UBpGameplayHelperLib::GetGameInstance()->OnSerialSaved.Broadcast(false, false, false, "Response Invalid.");
// 	}
// }
// 
// void URoNGameInstance::GS_GetPlayerDetails()
// {
// 	if (!UGameSparksModule::GetModulePtr())
// 	{
// 		UBpGameplayHelperLib::GetGameInstance()->OnPlayerDetailsRetrieved.Broadcast(false, false, "", "");
// 		return;
// 	}
// 
// 	GameSparks::Core::GS& gs = UGameSparksModule::GetModulePtr()->GetGSInstance();
// 
// 	GameSparks::Api::Requests::LogEventRequest gsRequest(gs);
// 	gsRequest.SetEventKey("GET_PLAYER_DETAILS");
// 
// 	gsRequest.Send(OnGetPlayerDetails_Response);
// }
// 
// void URoNGameInstance::OnGetPlayerDetails_Response(GameSparks::Core::GS& gs, const GameSparks::Api::Responses::LogEventResponse& response)
// {
// 	if (!UBpGameplayHelperLib::GetGameInstance())
// 		return;
// 
// 
// 	if (!response.GetHasErrors())
// 	{
// 		bool bDeveloper = response.GetBaseData().GetGSDataObject("scriptData").GetValue().GetBoolean("IS_DEVELOPER").GetValue();
// 		FString RealName(response.GetBaseData().GetGSDataObject("scriptData").GetValue().GetString("REAL_NAME").GetValue().c_str());
// 		FString EmailAddress(response.GetBaseData().GetGSDataObject("scriptData").GetValue().GetString("EMAIL_ADDRESS").GetValue().c_str());
// 		UBpGameplayHelperLib::GetGameInstance()->OnPlayerDetailsRetrieved.Broadcast(true, bDeveloper, EmailAddress, RealName);
// 	}
// 	else
// 	{
// 		UBpGameplayHelperLib::GetGameInstance()->OnPlayerDetailsRetrieved.Broadcast(false, false, "", "");
// 	}
// }
// 
// void URoNGameInstance::GS_SavePatchURL(FString url)
// {
// 	if (!UGameSparksModule::GetModulePtr())
// 	{
// 		UBpGameplayHelperLib::GetGameInstance()->OnPatchURLSaved.Broadcast(false);
// 		return;
// 	}
// 
// 	GameSparks::Core::GS& gs = UGameSparksModule::GetModulePtr()->GetGSInstance();
// 
// 	GameSparks::Api::Requests::LogEventRequest gsRequest(gs);
// 	gsRequest.SetEventKey("SAVE_PATCH_URL");
// 	gsRequest.SetEventAttribute("URL", std::string(TCHAR_TO_UTF8(*url)));
// 
// 	gsRequest.Send(SavePatchURL_Response);
// }
// 
// void URoNGameInstance::SavePatchURL_Response(GameSparks::Core::GS& gs, const GameSparks::Api::Responses::LogEventResponse& response)
// {
// 	if (!UBpGameplayHelperLib::GetGameInstance())
// 		return;
// 
// 
// 	UBpGameplayHelperLib::GetGameInstance()->OnPatchURLSaved.Broadcast(!response.GetHasErrors());
// }
// 
// void URoNGameInstance::GS_SavePCInformation()
// {
// 	if (!UGameSparksModule::GetModulePtr())
// 		return;
// 
// 	GameSparks::Core::GS& gs = UGameSparksModule::GetModulePtr()->GetGSInstance();
// 
// 	GameSparks::Api::Requests::LogEventRequest gsRequest(gs);
// 	gsRequest.SetEventKey("SAVE_PC_INFORMATION");
// 	gsRequest.SetEventAttribute("HWID", std::string(TCHAR_TO_UTF8(*FPlatformMisc::GetLoginId())));
// 
// 	gsRequest.Send();
// }
// 
// void URoNGameInstance::GS_GetNews()
// {
// 	if (!UGameSparksModule::GetModulePtr())
// 	{
// 		UBpGameplayHelperLib::GetGameInstance()->OnNewsReceived.Broadcast(false, "", "");
// 		return;
// 	}
// 
// 	GameSparks::Core::GS& gs = UGameSparksModule::GetModulePtr()->GetGSInstance();
// 
// 	GameSparks::Api::Requests::LogEventRequest gsRequest(gs);
// 	gsRequest.SetEventKey("GET_NEWS");
// 
// 	gsRequest.Send(OnGetNews_Response);
// }
// 
// void URoNGameInstance::OnGetNews_Response(GameSparks::Core::GS& gs, const GameSparks::Api::Responses::LogEventResponse& response)
// {
// 	if (!UBpGameplayHelperLib::GetGameInstance())
// 		return;
// 
// 
// 	if (!response.GetHasErrors())
// 	{
// 		FString NewsText(response.GetBaseData().GetGSDataObject("scriptData").GetValue().GetString("FRONTPAGE_NEWS_STRING").GetValue().c_str());
// 		FString NewsImageURL(response.GetBaseData().GetGSDataObject("scriptData").GetValue().GetString("FRONTPAGE_NEWS_IMAGE").GetValue().c_str());
// 		UBpGameplayHelperLib::GetGameInstance()->OnNewsReceived.Broadcast(true, NewsImageURL, NewsText);
// 	}
// 	else
// 	{
// 		UBpGameplayHelperLib::GetGameInstance()->OnNewsReceived.Broadcast(false, "", "");
// 	}
// }
// 
// void URoNGameInstance::GS_GetPatchURL()
// {
// 	if (!UGameSparksModule::GetModulePtr())
// 		return;
// 
// 	GameSparks::Core::GS& gs = UGameSparksModule::GetModulePtr()->GetGSInstance();
// 
// 	GameSparks::Api::Requests::LogEventRequest gsRequest(gs);
// 	gsRequest.SetEventKey("GET_PATCH_URL");
// 
// 	gsRequest.Send(OnGetPatchURL_Response);
// }
// 
// void URoNGameInstance::OnGetPatchURL_Response(GameSparks::Core::GS& gs, const GameSparks::Api::Responses::LogEventResponse& response)
// {
// 	if (!UBpGameplayHelperLib::GetGameInstance())
// 		return;
// 
// 	if (!response.GetHasErrors())
// 	{
// 		FString PatchURL(response.GetBaseData().GetGSDataObject("scriptData").GetValue().GetString("PATCH_URL").GetValue().c_str());
// 		bool bCanEditPatchURL = response.GetBaseData().GetGSDataObject("scriptData").GetValue().GetBoolean("CAN_EDIT_PATCH").GetValue();
// 		UBpGameplayHelperLib::GetGameInstance()->OnPatchURLRetrieved.Broadcast(true, PatchURL, bCanEditPatchURL);
// 	}
// 	else
// 	{
// 		UBpGameplayHelperLib::GetGameInstance()->OnPatchURLRetrieved.Broadcast(false, "", true);
// 	}
// }
// 
// void URoNGameInstance::GS_SubmitBugReport(FString Description)
// {
// 	if (!UGameSparksModule::GetModulePtr())
// 		return;
// 
// 	GameSparks::Core::GS& gs = UGameSparksModule::GetModulePtr()->GetGSInstance();
// 
// 	GameSparks::Api::Requests::LogEventRequest gsRequest(gs);
// 	gsRequest.SetEventKey("SUBMIT_BUG_REPORT");
// 	gsRequest.SetEventAttribute("Description", std::string(TCHAR_TO_UTF8(*Description)));
// 
// 	gsRequest.Send(OnSubmitBugReport_Response);
// }
// 
// void URoNGameInstance::OnSubmitBugReport_Response(GameSparks::Core::GS& gs, const GameSparks::Api::Responses::LogEventResponse& response)
// {
// 	if (!UBpGameplayHelperLib::GetGameInstance())
// 		return;
// 
// 	UBpGameplayHelperLib::GetGameInstance()->OnBugReportSubmited.Broadcast(!response.GetHasErrors());
// }
// 
// void URoNGameInstance::GS_SubmitHighScore(int32 Score, EHighScoreCategory Category)
// {
// 	if (!UGameSparksModule::GetModulePtr())
// 		return;
// 
// 	GameSparks::Core::GS& gs = UGameSparksModule::GetModulePtr()->GetGSInstance();
// 
// 	GameSparks::Api::Requests::LogEventRequest gsRequest(gs);
// 	
// 	switch (Category)
// 	{
// 	case EHighScoreCategory::HSC_COOP_DAILY: gsRequest.SetEventKey("ADD_SCORE_DAILY_LEADERBOARD"); break;
// 	}
// 	
// 	gsRequest.SetEventAttribute("SCORE", Score);
// 	gsRequest.Send(OnHighScoreSubmitted_Response);
// }
// 
// void URoNGameInstance::OnHighScoreSubmitted_Response(GameSparks::Core::GS& gs, const GameSparks::Api::Responses::LogEventResponse& response)
// {
// 	UBpGameplayHelperLib::GetGameInstance()->OnHighScoreSubmitted.Broadcast(!response.GetHasErrors());
// }
// 
// void URoNGameInstance::GS_SubmitCOOPStats(EHighScoreCategory Category, int32 Score /*= 0*/, int32 MissionsComplete /*= 0*/, int32 SuspectsArrested /*= 0*/, int32 SuspectsNeutralized /*= 0*/, int32 NoSuspectsNeutralizedBonus /*= 0*/, int32 NoCivilianInjuredBonus /*= 0*/, int32 OfficersAlive /*= 0*/, int32 TotalOfficers /*= 0*/, int32 TocReports /*= 0*/, int32 EvidenceSecured /*= 0*/, int32 UnjustifiedKills /*= 0*/, int32 InjuredCivilians /*= 0*/, int32 KilledCivilians /*= 0*/, int32 Deaths /*= 0*/, int32 Headshots /*= 0*/, int32 TotalBulletsFired /*= 0*/, int32 TimePlayed /*= in seconds  0*/, int32 TotalGrenadesThrown /*= 0*/, int32 TotalYellsAtEnemy /*= 0s*/, int32 TeamKills)
// {
// 
// 	if (!UGameSparksModule::GetModulePtr())
// 		return;
// 
// 	GameSparks::Core::GS& gs = UGameSparksModule::GetModulePtr()->GetGSInstance();
// 
// 	GameSparks::Api::Requests::LogEventRequest gsRequest(gs);
// 
// 	
// 	gsRequest.SetEventKey("ADD_PLAYER_COOP_STATS");
// 	// do some sanity checks on the data...
// 	ACoopGS* gstate = UBpGameplayHelperLib::GetGameInstance()->GetWorld()->GetGameState<ACoopGS>();
// 	if (!gstate)
// 		return;
// 
// 	bool bGG = false;
// 	FString reportString = "";
// 	reportString = "Adding COOP Player Stats to the leaderboard.  |";
// 		// todo: report these for validation.. whats the data like?
// 
// 	if (SuspectsArrested + SuspectsNeutralized > gstate->TotalSuspects)
// 	{
// 		bGG = true;
// 		reportString.Append("Report Reason: More Arrests/Kills than Total Suspects?");
// 	} 
// 	if (Score > 100)
// 	{
// 		bGG = true;
// 		reportString.Append("Report Reason: Score is greater than maximum allowed??");
// 	}
// 
// 	if (bGG)
// 	{
// 		reportString.Append("\r\rSuspicious Player data\r\r");
// 		reportString.Append("Dumping Score Values from game state.\r\r");
// 		for (TFieldIterator<UIntProperty> Prop(gstate->GetClass()); Prop; ++Prop)
// 		{
// 			int32 val = Prop->GetPropertyValue(gstate);
// 			FString PropertyName = Prop->GetName();
// 			reportString.Append(PropertyName + " : " + FString::FromInt(val));
// 		}
// 		GS_SubmitBugReport(reportString);
// 		return;
// 	}
// 
// 	gsRequest.SetEventAttribute("SCORE", Score);
// 	gsRequest.SetEventAttribute("MISSIONS_COMPLETE", MissionsComplete);
// 	gsRequest.SetEventAttribute("SUSPECTS_ARRESTED", SuspectsArrested);
// 	gsRequest.SetEventAttribute("SUSPECTS_NEUTRALIZED", SuspectsNeutralized);
// 	gsRequest.SetEventAttribute("NO_SUSPECTS_NEUTRALIZED_BONUS", NoSuspectsNeutralizedBonus);
// 	gsRequest.SetEventAttribute("NO_CIVILIAN_INJURED_BONUS", NoCivilianInjuredBonus);
// 	gsRequest.SetEventAttribute("OFFICERS_ALIVE", OfficersAlive);
// 	gsRequest.SetEventAttribute("TOTAL_OFFICERS", TotalOfficers);
// 	gsRequest.SetEventAttribute("TOC_REPORTS", TocReports);
// 	gsRequest.SetEventAttribute("EVIDENCE_SECURED", EvidenceSecured);
// 	gsRequest.SetEventAttribute("UNJUSTIFIED_KILL", UnjustifiedKills);
// 	gsRequest.SetEventAttribute("INJURED_CIVILIAN", InjuredCivilians);
// 	gsRequest.SetEventAttribute("KILLED_CIVILIAN", KilledCivilians);
// 	gsRequest.SetEventAttribute("DEATHS", Deaths);
// 	gsRequest.SetEventAttribute("HEADSHOTS", Headshots);
// 	gsRequest.SetEventAttribute("TOTAL_BULLETS_FIRED", TotalBulletsFired);
// 	gsRequest.SetEventAttribute("TIME_PLAYED", TimePlayed);
// 	gsRequest.SetEventAttribute("TOTAL_GRENADES_THROWN", TotalGrenadesThrown);
// 	gsRequest.SetEventAttribute("TOTAL_YELLS_AT_ENEMY", TotalYellsAtEnemy);
// 	gsRequest.SetEventAttribute("TEAM_KILLS", TeamKills);
// 
// 
// 	gsRequest.Send(OnStatsSubmitted_Response);
// }
// 
// void URoNGameInstance::OnStatsSubmitted_Response(GameSparks::Core::GS& gs, const GameSparks::Api::Responses::LogEventResponse& response)
// {
// 	if (!UBpGameplayHelperLib::GetGameInstance())
// 		return;
// 
// 	UBpGameplayHelperLib::GetGameInstance()->OnStatsSubmitted.Broadcast(!response.GetHasErrors());
// }

bool UReadyOrNotGameInstance::IsHostMigrationInProgress(FString& MigratedHostToName)
{
	if (HostMigrationManager)
	{
		MigratedHostToName = HostMigrationManager->NextHostName;
		return HostMigrationManager->IsMigratingHost();
	}
	return false;
}

void UReadyOrNotGameInstance::StopRecordingReplayFromBP()
{
	StopRecordingReplay();
}

void UReadyOrNotGameInstance::PlayReplayFromBP(FString ReplayName)
{
	if(GEngine)
	{
		GEngine->Exec(GetWorld(), TEXT("demo.AsyncLoadWorld 1"));
	}
	PlayReplay(ReplayName);
}

void UReadyOrNotGameInstance::OpenReplayFolder()
{
	FString URL = TEXT("file://") +  FPaths::Combine(FPaths::ProjectSavedDir(), "Demos/");
	FGenericPlatformProcess::LaunchURL(*URL, nullptr, nullptr);
}


TMap<FString, FReplayData> UReadyOrNotGameInstance::FindReplays()
{
	TMap<FString, FReplayData> FoundReplayMap;
	TArray<FString> FileNames;
	
	const FString Wildcard("replay");
	const FString SearchDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Demos/"));

	FPlatformFileManager& FileManager = FPlatformFileManager::Get();
	FileManager.GetPlatformFile().FindFiles(FileNames, *SearchDir, *Wildcard);

	for(auto f : FileNames)
	{
		FString FilePath = f.Replace(TEXT(".replay"), TEXT("")) + FString(".replaydata");
		TArray<FString> SplitFilePath;
		FilePath.ParseIntoArray(SplitFilePath, TEXT("/"));

		FString ReplayBaseName = SplitFilePath[SplitFilePath.Num()-1].Replace(TEXT(".replaydata"), TEXT(""));

		// Check for accompanying replaydata file. This file is not needed, but contains useful information for the user. 
		V_LOGM(LogReadyOrNot, "Searching for: %s", *FilePath);
		if(FileManager.GetPlatformFile().FileExists(*FilePath))
		{
			// Parse the file into the appropriate struct.
			FReplayData ReplayData;
			FString ReplayDataString;
			FFileHelper::LoadFileToString(ReplayDataString, *FilePath);
			FString UnEncodedReplayData;
			FBase64::Decode(ReplayDataString, UnEncodedReplayData);
			
			FJsonObjectConverter::JsonObjectStringToUStruct(UnEncodedReplayData, &ReplayData);

			V_LOGM(LogReadyOrNot, "Replay file found with .replaydata: %s", *f);
			
			FoundReplayMap.Add(ReplayBaseName, ReplayData);
		}
		else
		{
			FoundReplayMap.Add(ReplayBaseName, FReplayData());
			V_LOGM(LogReadyOrNot, "Replay file found without .replaydata: %s", *f);
		}
	}
	return FoundReplayMap;
}

void UReadyOrNotGameInstance::RenameReplay(const FString &ReplayName, const FString &NewFriendlyReplayName)
{
	const FString ReplayFilePath = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("Demos/")).Append(*ReplayName).Append(".replay");
	const FString ReplayDataFilePath = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("Demos/")).Append(*ReplayName).Append(".replaydata");

	const FString NewReplayFilePath = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("Demos/")).Append(*NewFriendlyReplayName).Append(".replay");
	const FString NewReplayDataFilePath = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("Demos/")).Append(*NewFriendlyReplayName).Append(".replaydata");


	IFileManager& FileManager = IFileManager::Get();

	// Rename the .replay file.
	if(FileManager.FileExists(*ReplayFilePath))
	{
		FileManager.Move(*NewReplayFilePath, *ReplayFilePath);
		V_LOGM(LogReadyOrNot, "Successfully renamed replay file: %s", *NewReplayFilePath);
	}
	else
	{
		V_LOGM(LogReadyOrNot, "Failed to rename replay file: %s", *ReplayFilePath);
	}

	// Rename the .replaydata file.
	if(FileManager.FileExists(*ReplayDataFilePath))
	{
		FileManager.Move(*NewReplayDataFilePath, *ReplayDataFilePath);
		V_LOGM(LogReadyOrNot, "Successfully renamed replaydata file: %s", *NewReplayDataFilePath);
	}
	else
	{
		V_LOGM(LogReadyOrNot, "Failed to rename replay file: %s", *ReplayDataFilePath);

	}
}

void UReadyOrNotGameInstance::DeleteReplay(const FString &ReplayName)
{
	FString JsonFilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Demos/")).Append(ReplayName);

	IFileManager& FileManager = IFileManager::Get();

	const FString ReplayFilePath = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("Demos/")).Append(*ReplayName).Append(".replay");
	const FString ReplayDataFilePath = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("Demos/")).Append(*ReplayName).Append(".replaydata");
	
	FileManager.Delete(*ReplayFilePath);
	FileManager.Delete(*ReplayDataFilePath);
}

void UReadyOrNotGameInstance::StartRecordingReplay()
{
	// Ensure we are not playing a replay
	if(GetWorld()->IsPlayingReplay() || GetWorld()->IsPlayingClientReplay())
	{
		return;
	}

	// Set replay smoothness
	if(GEngine)
	{
		GEngine->Exec(GetWorld(), TEXT("demo.recordhz 120"));
		GEngine->Exec(GetWorld(), TEXT("demo.RecordHzWhenNotRelevant 120"));
		GEngine->Exec(GetWorld(), TEXT("demo.UseAdaptiveReplayUpdateFrequency 0"));
		GEngine->Exec(GetWorld(), TEXT("demo.IncreaseRepPrioritizeThreshold 0.9"));
	}

	// Fetch the steam name.
	FString SteamId, SteamName; 
	APlayerController* pc = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (pc)
	{
		if (pc->PlayerState)
		{
			if (pc->PlayerState->GetUniqueId().IsValid())
			{
				SteamId = pc->PlayerState->GetUniqueId().ToString();
				SteamName = pc->PlayerState->GetPlayerName();
			}

		}
	}
	if (SteamName.IsEmpty())
	{
		SteamName = "Unknown";
	}

	bReplayBeginTime = GetWorld()->TimeSeconds;
	
	FString DateTime = FDateTime::Now().ToString();
	DateTime = DateTime.Replace(TEXT(":"), TEXT("_"));

	const FString WorldName = UReadyOrNotFunctionLibrary::GetCurrentLevelNameForLookupTable(this).ToString();
	FString ProjectVersionStr = UBpGameplayHelperLib::GetProjectVersion();
	
	FString Name = "[" + SteamName + "][" + DateTime + "]" + "[";

	const FLevelDataLookupTable* LevelDataRow = UBpGameplayHelperLib::GetLevelLookupDataTable()->FindRow<FLevelDataLookupTable>(*WorldName, "Replay Level Name Grab");
	if(LevelDataRow)
	{
		Name.Append(LevelDataRow->LevelNickname.ToString());
	}
	else
	{
		Name.Append(WorldName);
	}
		
	FString GamemodeName = GetFriendlyGamemodeName(GetWorld()->GetGameState()->GameModeClass->GetName());
	
		
	Name.Append("][" + GamemodeName + "]");

	ProjectVersionStr = ProjectVersionStr.Replace(TEXT("."), TEXT(""));
	Name = Name.Replace(TEXT("/"), TEXT(""));
	FString FullName = "[" + ProjectVersionStr + "]" + Name;
	
	CurrentlyRecordingReplayName = FullName;
	
	V_LOGM(LogReadyOrNot, "Replay recording started: %s", *FullName);
	Super::StartRecordingReplay(FullName, FullName, {}, nullptr);
}

void UReadyOrNotGameInstance::StartRecordingReplay(const FString& InName, const FString& FriendlyName, const TArray<FString>& AdditionalOptions, TSharedPtr<IAnalyticsProvider> AnalyticsProvider)
{
	StartRecordingReplay();
}

void UReadyOrNotGameInstance::StopRecordingReplay()
{
	if (!GetWorld())
		return;

	if(GetWorld()->IsPlayingReplay() || GetWorld()->IsPlayingClientReplay() || CurrentlyRecordingReplayName == "")
		return;

	AReadyOrNotGameState* GameState = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (!GameState)
		return;
	
	float ReplayLength = GetWorld()->TimeSeconds-bReplayBeginTime;
	
	// Get the scores.
	FString LetterScore = "F";
	int32 NumericalScore = 0;
	if(AScoringManager* ScoringManager = GameState->GetScoringManager())
	{
		LetterScore = ScoringManager->CalculateGradeLetterFromPlayerScore();
		NumericalScore = ScoringManager->CalculateTotalPlayerScore();
	}

	// Get the level name.
	const FString WorldName = UReadyOrNotFunctionLibrary::GetCurrentLevelNameForLookupTable(this).ToString();
	const FLevelDataLookupTable* LevelDataRow = UBpGameplayHelperLib::GetLevelLookupDataTable()->FindRow<FLevelDataLookupTable>(*WorldName, "Replay Level Name Grab");
	FString MapName;
	if(LevelDataRow)
	{
		MapName = LevelDataRow->LevelNickname.ToString();
	}
	else
	{
		MapName = WorldName;
	}
	
	FString GamemodeName = GetFriendlyGamemodeName(GetWorld()->GetGameState()->GameModeClass->GetName());

	FDateTime Date = FDateTime::Now();
	FString Month;

	switch (Date.GetMonthOfYear())
	{
		case EMonthOfYear::January:		Month = TEXT("Jan");	break;
		case EMonthOfYear::February:	Month = TEXT("Feb");	break;
		case EMonthOfYear::March:		Month = TEXT("Mar");	break;
		case EMonthOfYear::April:		Month = TEXT("Apr");	break;
		case EMonthOfYear::May:			Month = TEXT("May");	break;
		case EMonthOfYear::June:		Month = TEXT("Jun");	break;
		case EMonthOfYear::July:		Month = TEXT("Jul");	break;
		case EMonthOfYear::August:		Month = TEXT("Aug");	break;
		case EMonthOfYear::September:	Month = TEXT("Sep");	break;
		case EMonthOfYear::October:		Month = TEXT("Oct");	break;
		case EMonthOfYear::November:	Month = TEXT("Nov");	break;
		case EMonthOfYear::December:	Month = TEXT("Dec");	break;
	}
	
	FString Time = FString::Printf(TEXT("%02i:%02i:%02i "), Date.GetHour(), Date.GetMinute(), Date.GetSecond()).Append(Date.IsMorning() ? TEXT("AM") : TEXT("PM"));
	FString Timestamp =  FString::Printf(TEXT("%s %i, %i, %s"), *Month, Date.GetDay(), Date.GetYear(), *Time);

	FString JsonFilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Demos/"));
	JsonFilePath.Append(CurrentlyRecordingReplayName + ".replaydata");

	FString ProjectVersionStr = UBpGameplayHelperLib::GetProjectVersion();

	FReplayData ReplayData = FReplayData(ReplayLength, LetterScore, NumericalScore, ReplayNumPlayers, MapName, GamemodeName, WorldName, Timestamp, ProjectVersionStr, ReplayEvents);
	FString ReplayDataJson;
	FJsonObjectConverter::UStructToJsonObjectString(ReplayData, ReplayDataJson);

	ReplayDataJson = FBase64::Encode(ReplayDataJson);
		
	if(FFileHelper::SaveStringToFile(ReplayDataJson, *JsonFilePath))
	{
		V_LOGM(LogReadyOrNot, "Replay data file saved: %s", *JsonFilePath);
	}
	else
	{
		V_LOGM(LogReadyOrNot, "Replay data file failed to save.");
	}

	CurrentlyRecordingReplayName = "";
	ReplayEvents = TArray<FReplayEvent>();
	ReplayNumPlayers = 0;

	V_LOGM(LogReadyOrNot, "Replay recording stopped");
	Super::StopRecordingReplay();
}

void UReadyOrNotGameInstance::AddReplayEvent(TEnumAsByte<EReplayEventType> EventType, FVector Location, float Timestamp, FString AdditionalInformation)
{
	FReplayEvent ReplayEvent = FReplayEvent(EventType, Location, Timestamp, AdditionalInformation);
	ReplayEvents.Add(ReplayEvent);
}
TArray<FReplayEvent> UReadyOrNotGameInstance::GetReplayEvents()
{
	if (!GetWorld())
		return {};

	if (!GetWorld()->GetDemoNetDriver())
		return {};
	
	// Get the currently playing replay name;
	FString ReplayName = GetWorld()->GetDemoNetDriver()->GetActiveReplayName();
	const FString ReplayDataFilePath = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("Demos/")).Append(*ReplayName).Append(".replaydata");

	IFileManager& FileManager = IFileManager::Get();
	if(FileManager.FileExists(*ReplayDataFilePath))
	{
		// Now we can load the JSON.
		FReplayData ReplayData;
		FString ReplayDataString;
		FFileHelper::LoadFileToString(ReplayDataString, *ReplayDataFilePath);

		FString UnEncodedReplayData;
		
		FBase64::Decode(ReplayDataString, UnEncodedReplayData);
		FJsonObjectConverter::JsonObjectStringToUStruct(UnEncodedReplayData, &ReplayData);

		return ReplayData.ReplayEvents;
		
	}
	return TArray<FReplayEvent>(); 
}

FString UReadyOrNotGameInstance::GetFriendlyGamemodeName(FString UnfriendlyName)
{
	if(UnfriendlyName.Contains("BarricadedSuspects"))
	{
		return FString("Barricaded Suspects");
	}
	else if(UnfriendlyName.Contains("BombThreat"))
	{
		return FString("Bomb Threat");
	}
	else if(UnfriendlyName.Contains("ActiveShooter"))
	{
		return FString("Active Shooter");
	}
	else if(UnfriendlyName.Contains("HostageRescue"))
	{
		return FString("Hostage Rescue");
	}
	else if(UnfriendlyName.Contains("Raid"))
	{
		return FString("Raid");
	}
	else
	{
		return FString("None");
	}
}

void UReadyOrNotGameInstance::CreateReplayLoadingScreen()
{
	ReplayLoadingScreen = CreateWidget<UUserWidget>(this, UBpGameplayHelperLib::GetWidgetDataFromLookupData("ReplayLoadingScreen").WidgetClass);
	if(ReplayLoadingScreen)
	{
		GetGameViewportClient()->AddViewportWidgetContent(ReplayLoadingScreen->TakeWidget());
	}
}

void UReadyOrNotGameInstance::RemoveReplayLoadingScreen()
{
	//ReplayLoadingScreen->RemoveFromParent();
	if(ReplayLoadingScreen)
	{
		GetGameViewportClient()->RemoveViewportWidgetContent(ReplayLoadingScreen->TakeWidget());
		ReplayLoadingScreen = nullptr;
	}
}

bool UReadyOrNotGameInstance::IsReplaySystemEnabled()
{
#ifdef REPLAY_SYSTEM
	return true;
#else
	return false;
#endif	
}

void UReadyOrNotGameInstance::StartGeneratingPSOCache()
{
	GEngine->Exec(GetWorld(), TEXT("a.RonPauseSignificance 1"));
	GetTimerManager().SetTimer(GeneratePSOCache_Handle, this, &UReadyOrNotGameInstance::GeneratePSOCache, 0.0167f, true);
}

void UReadyOrNotGameInstance::StopGeneratingPSOCache()
{
	GetTimerManager().ClearTimer(GeneratePSOCache_Handle);
}

void UReadyOrNotGameInstance::GeneratePSOCache()
{
	if (!GetWorld() || GetWorld()->IsInSeamlessTravel())
		return;

	if (GetWorld()->GetMapName() != CurrentMapNamePSOCache)
	{
		CurrentMapNamePSOCache = GetWorld()->GetMapName();
	}
	

	AReadyOrNotGameMode* Mode = UReadyOrNotStatics::GetReadyOrNotGameMode();
	if (Mode && Mode->HasEveryoneFinishedLoading() && !Mode->HasMatchStarted())
	{
		Mode->StartMatch();
	}
	
	
	TArray<AThreatAwarenessActor*> Threats;
	for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
	{
		Threats.Add(*It);
	}

	for (TActorIterator<AReadyOrNotCharacter> It(GetWorld()); It; ++It)
	{
		AReadyOrNotCharacter* pc = *It;
		pc->GetHealthComponent()->SetCurrentResourceToMax();
	}

	bool bFinished = !Threats.IsValidIndex(PsoThreatIdx + 1);
	if (!bFinished)
	{
		if (Threats.IsValidIndex(PsoThreatIdx + 1))
		{
			PsoThreatIdx++;
		}
		AThreatAwarenessActor* SelectedThreat = Threats[FMath::RandRange(0, Threats.Num() - 1)];
		for (TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
		{
			It->SetActorLocation(SelectedThreat->GetActorLocation());
			It->Client_SetControlRotation(FRotator(0.0f, FMath::Rand(), 0.0f));
		}
	
	}
	else
	{
		AMissionPortal* MissionPortal = GetWorld()->SpawnActor<AMissionPortal>();
		MissionPortal->SelectRandomMission(false);
		if (PSOCacheMapList.IsValidIndex(PsoCacheMapIdx + 1))
		{
			PsoThreatIdx = 0;
			PsoCacheMapIdx++;
			GetWorld()->GetAuthGameMode()->ProcessServerTravel(PSOCacheMapList[PsoCacheMapIdx] + "?game=BS_COOP", true);
		}
	}
}

void UReadyOrNotGameInstance::CommanderDeleteProfile(int32 Slot)
{
	UCommanderProfile* Profile = UCommanderProfile::LoadProfile(Slot);
	if (Profile)
		Profile->DeleteProfile();
}

void UReadyOrNotGameInstance::CommanderCompleteMission(const FString& Mission)
{
	if (!GetWorld())
		return;

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
		return;

	ALobbyGM* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGM>();
	if (!LobbyGM || !LobbyGM->CommanderProfile)
	{
		PlayerController->ClientMessage("Not in commander lobby");
		return;
	}

	UCommanderProfile* Profile = LobbyGM->CommanderProfile;
	if (!Profile)
	{
		PlayerController->ClientMessage("Profile is not loaded, does not exist or is corrupt");
		return;
	}
	
	Profile->CompletedLevels.AddUnique(Mission);
	Profile->SaveProfile();
#if defined(TARGET_PS5)
	UPS5ActivitiesStatics::OnSaveGame(GetWorld(),Profile);
#endif
}

void UReadyOrNotGameInstance::CommanderGenerateProfile(int32 Slot)
{
	if (!GetWorld())
		return;

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
		return;
	
	if (UCommanderProfile::LoadProfile(Slot))
	{
		PlayerController->ClientMessage("Profile already exists");
		return;
	}
	
	UCommanderProfile* Profile = UCommanderProfile::CreateProfile(Slot);
	if (!Profile)
		return;
	
	if (Profile->Campaign && Profile->Campaign->Levels.Num() > 0)
	{
		Profile->CompletedLevels = Profile->Campaign->Levels;
		Profile->CompletedLevels.SetNum(FMath::RandRange(0, Profile->Campaign->Levels.Num())); // Random completion
	}
	Profile->SaveDate = FDateTime::Now() - FTimespan::FromSeconds(FMath::RandRange(0, 1209600)); // 0 - 14 days
	Profile->TotalPlaytime += FTimespan::FromSeconds(FMath::RandRange(0, 129600)); // 0 - 36 hours
	Profile->bIsModded = FMath::RandBool();
	Profile->GameChecksum = Checksum;
	
	UGameplayStatics::SaveGameToSlot(Profile, Profile->GetSlot(), 0);
#if defined(TARGET_PS5)
	UPS5ActivitiesStatics::OnSaveGame(GetWorld(), Profile);
#endif	
}




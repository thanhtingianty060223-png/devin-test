// Copyright Void Interactive, 2023

#ifndef __READYORNOT_H__
#define __READYORNOT_H__

#define WIN32_LEAN_AND_MEAN

#if !defined(__clang__)
#pragma warning(disable:4996)
#endif

#include "Engine.h"
#include "Net/UnrealNetwork.h"
#include "lib/ReadyOrNotStatics.h"
#include "Online.h"
#include "EngineGlobals.h"
#include "lib/XorString.h"
#include "EngineMinimal.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameplayConfig.h"
#include "ReadyOrNotGameInstance.h"
#include "ReadyOrNotCoreDelegates.h"
#include "LegacyCameraShake.h"

#include "ReadyOrNotVoiceConfig.h"

#include "lib/BpGameplayHelperLib.h"
#include "lib/BpVideoSettingsLib.h"
#include "lib/DataSingleton.h"
#include "lib/ReadyOrNotFunctionLibrary.h"

#include "Animation/SkeletalMeshActor.h"
#include "Engine/World.h"

#include "Actors/Gameplay/ReadyOrNotPlayerState.h"
#include "Characters/CyberneticCharacter.h"
#include "Characters/PlayerCharacter.h"
#include "Characters/ReadyOrNotPlayerController.h"
#include "Characters/SpectatePawn.h"
#include "ReadyOrNotGameState.h"

#include "Objectives/Objective.h"
#include "Objectives//BringOrderToChaos.h"
#include "Objectives/DontDie.h"
#include "Objectives/RescueAllOfTheCivilians.h"

#include "Actors/BaseItem.h"
#include "Actors/BaseArmour.h"
#include "Actors/BaseWeapon.h"
#include "Actors/BaseShell.h"
#include "Actors/BaseMagazineWeapon.h"
#include "Actors/Items/Zipcuffs.h"
#include "Actors/Items/Taser.h"

#include "ReadyOrNotLevelScript.h"
#include "Data/LevelData.h"

#include "Data/AIData.h"
#include "Data/ItemData.h"
#include "Data/LevelData.h"
#include "Data/SoundData.h"

#include "Structs.h"
#include "Enums.h"

#include "FMODBlueprintStatics.h"

#include "Log.h"

#include "ThirdParty/SteamworksIntegration.h"

#include "lib/AsyncLoader.h"

#include "Subsystems/InGameLogSubsystem.h"
#if !defined(__clang__)
#pragma warning(default:4996)
#endif

// See https://docs.unrealengine.com/latest/INT/Programming/Assertions/ 
// Enabled check(expression) for debug 
#ifndef DO_CHECK 
#define DO_CHECK 
#endif 

extern const TArray<FString> VALID_GAME_PAK_FILES;

#define BIG_DIST 9999999999.0f;
#define SETTING_GAMENAME "GAMENAME"
#define SETTING_VERSION "VERSION"
#define SETTING_CHECKSUM "CHECKSUM"
#define SETTING_PVP "PVP"
#define SETTING_PVP_ENABLED "PVP_ENABLED"
#define SETTING_PVP_DISABLED "PVP_DISABLED"

#define SETTING_CHECKSUM_ENABLED "CHECKSUM_ENABLED"
#define MIGRATION_GUID "MIGRATION_GUID"

#define MAP_MAINMENU "MainMenu_V2"

// Used when joining a server to make sure the client/server are running the same version
#define NETWORK_VERSION_NUMBER 49;

#define RON_ENUM_TO_STRING(etype, evalue) ((FindObject<UEnum>(ANY_PACKAGE, TEXT(#etype), true) != nullptr) ? FindObject<UEnum>(ANY_PACKAGE, TEXT(#etype), true)->GetNameStringByIndex((int32)evalue) : FString("_InvalidEnum_"))

#define CHECK_DEBUG_SUBSYSTEM (IsValid(GetWorld()) && IsValid(GetWorld()->GetGameInstance()) && IsValid(GetWorld()->GetGameInstance()->GetSubsystem<UReadyOrNotDebugSubsystem>()))
#define DEBUG_SUBSYSTEM GetWorld()->GetGameInstance()->GetSubsystem<UReadyOrNotDebugSubsystem>()
#define THREAT_AWARENESS_SUBSYSTEM GetWorld()->GetSubsystem<UThreatAwarenessSubsystem>()

#define INGAMELOG GetWorld()->GetSubsystem<UInGameLogSubsystem>()
#define ENQUEUE_INGAMELOG_MESSAGE GetWorld()->GetSubsystem<UInGameLogSubsystem>()->EnqueueLogMessage
#define ENQUEUE_INGAMELOG_MESSAGE_PVP GetWorld()->GetSubsystem<UInGameLogSubsystem>()->EnqueuePVPMessage

#define ENQUEUE_INGAMELOG_MESSAGE_ONETIME(TimerHandle, LogSeverity, TimeOnScreen, LogMessage) \
    if (!GetWorldTimerManager().IsTimerActive(TimerHandle)) \
    { \
        GetWorldTimerManager().SetTimer(TimerHandle, TimeOnScreen, false); \
        ENQUEUE_INGAMELOG_MESSAGE({LogSeverity, FText::FromString(LogMessage), 0.08f, 0.0f, TimeOnScreen}); \
    } \

#if !UE_BUILD_SHIPPING
#define LOG_CLASS_FUNC ULog::Info(CUR_CLASS_FUNC);
#else
#define LOG_CLASS_FUNC
#endif

#if WITH_EDITOR
#define START_MATCH_FADE_TIME 2.0f
#else
#define START_MATCH_FADE_TIME 3.0f
#endif

#define MAX_GAMEMODE_PLAYERS 16

#define MAX_LEAN_ANGLE 15.0f
#define MAX_LEAN_SPEED 50
		
#define MAX_FREE_LEAN_ANGLE 40.0f
#define MAX_FREE_LEAN_SPEED_AIMING 75
#define MAX_FREE_LEAN_SPEED_NOT_AIMING 100

// Disable this define to turn off modio
#define MODIO_ENABLED

#define MODIO_OVERIDE_INSTALL_DIRECTORY
#define MODIO_LIVE
#ifdef MODIO_LIVE
// production values taken from mod.io/g
#define MOD_IO_API_KEY TEXT("41c1a9a92cb222df61726049fb915bae")
#define MOD_IO_GAME_ID 3791
#else
// Test values
#define MOD_IO_API_KEY TEXT("b4ee7c8b4f3350270789a4a0010d237f")
#define MOD_IO_GAME_ID 907
#endif

// Checks to see if an actor is in a particular game mode. Destroy if not in specified game mode
#define GAMEMODE_CHECK(GameModeClass, GameStateClass) \
GameModeClass* GM = Cast<GameModeClass>(UGameplayStatics::GetGameMode(this)); \
GameStateClass* GS = Cast<GameStateClass>(UGameplayStatics::GetGameState(this)); \
\
if (!GM && !GS) \
{ \
    Destroy(); \
\
    return;\
} \

#define ISGAMEVIEWRETURN() \
FEditorViewportClient* Client = (FEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();\
if (Client && Client->IsInGameView())\
{\
	return;\
}; \

#define DESTROY_COMPONENT(Component)\
if (IsValid(Component))\
{\
    Component->DestroyComponent();\
    Component = nullptr;\
}\

#define GET_GAME_STATE(GameStateClass) GetWorld()->GetGameState<GameStateClass>()

#define ECC_PROJECTILE ECC_GameTraceChannel1
#define ECC_ITEM ECC_GameTraceChannel2
#define ECC_DOOR ECC_GameTraceChannel3
#define ECC_REVERB ECC_GameTraceChannel4
#define ECC_COVER ECC_GameTraceChannel5
#define ECC_DOORWAY ECC_GameTraceChannel6
#define ECC_YELLOUT ECC_GameTraceChannel7
#define ECC_BULLETEFFECT ECC_GameTraceChannel8
#define ECC_ONLYPLAYER ECC_GameTraceChannel9
#define ECC_VAULT ECC_GameTraceChannel10
#define ECC_PAIRED_INTERACTION_PAWN ECC_GameTraceChannel11
#define ECC_SOUND ECC_GameTraceChannel12
#define ECC_VOLUME ECC_GameTraceChannel13
#define ECC_OCCLUSION ECC_GameTraceChannel14



#define LOCAL_PLAYER APlayerCharacter* LocalPlayer = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0))

#define HIDE_ACTOR_CATEGORIES HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking")\

#define SOCKET_EYES_VIEW_POINT "eyesviewpoint"
#define BONE_FP_CAMERA "fp_camera"

DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNot, Log, All);                   // General logging
DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotLoadout, Log, All);            // Logging for loadout system
DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotInit, Log, All);               // Logging during game startup
DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotAI, Log, All);                 // Logging for AI
DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotAudio, Log, All);              // Logging for audio
DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotUI, Log, All);                 // Logging for UI
DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotSteam, Log, All);              // Logging for Steam
DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotModding, Log, All);            // Logging for mod management
DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotCriticalErrors, Log, All);     // Logging for Critical Errors that must always be addressed 
DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotServer, Log, All);             // Logging for dedicated server
DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotChat, Log, All);               // Logging for player chat
DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotProgression, Log, All);        // Logging for progression
DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotMatchmaking, Log, All);        // Logging for matchmaking
DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotSubtitles, Log, All);          // Logging for subtitles and their localization
DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotHitRegistration, Log, All);    // Logging for hit registration

#define RON_CUR_CLASS (FString(__FUNCTION__).Left(FString(__FUNCTION__).Find(TEXT(":"))) )
#define RON_CUR_LINE  (FString::FromInt(__LINE__))
#define RON_CUR_CLASS_LINE (RON_CUR_CLASS + "(" + RON_CUR_LINE + ")")

#define V_LOG(LogCat, Param1) 		UE_LOG(LogCat,Warning,TEXT("%s: %s"), *RON_CUR_CLASS_LINE, *FString(Param1))
#define V_LOG2(LogCat, Param1,Param2) 	UE_LOG(LogCat,Warning,TEXT("%s: %s %s"), *RON_CUR_CLASS_LINE, *FString(Param1),*FString(Param2))
#define V_LOGF(LogCat, Param1,Param2) 	UE_LOG(LogCat,Warning,TEXT("%s: %s %f"), *RON_CUR_CLASS_LINE, *FString(Param1),Param2)
#define V_LOGM(LogCat, FormatString , ...) UE_LOG(LogCat,Warning,TEXT("%s: %s"), *RON_CUR_CLASS_LINE, *FString::Printf(TEXT(FormatString), ##__VA_ARGS__ ) )

#define FUNC_NAME TEXT(__FUNCTION__)

#endif

#define TOKENPASTE(x, y) x##y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)

#define MAKE_VAR2(Type, Name) Type Name
#define MAKE_VAR(Type, Name) MAKE_VAR2(Type, TOKENPASTE2(Name, __LINE__))

#define ByteArrayToString_Offset(Array, Count, Offset, Result)\
Result.Empty(Count);\
MAKE_VAR(int32, CurrentCount) = Count;\
MAKE_VAR(uint8*, Data) = Array;\
\
while (TOKENPASTE2(CurrentCount, __LINE__))\
{\
int16 Value = *TOKENPASTE2(Data, __LINE__);\
\
Result += TCHAR(Value-(Offset));\
\
++TOKENPASTE2(Data, __LINE__);\
TOKENPASTE2(CurrentCount, __LINE__)--;\
}\

#define USE_NEW_ANIMSYSTEMd
#define USE_NEW_PVP_PREPLANNING

// Set this define from Build Server later
// No gameplay debug info on consoles for now.
#if !defined(__clang__)
#pragma warning(disable: 4668)
#endif
#if (__PROSPERO__) || (__ORBIS__) || (__SCARLETT__) || (__DURANGO__) 
#define GAMEPLAY_DEBUG 0
#else
#define GAMEPLAY_DEBUG 1
#endif

/*
Programmable Defines (set from jenkins - DO NOT set defines manually between PRG-DEFINES-BEGIN and PRG-DEFINES-END or your defines will be wiped during the build process)
DMO_BUILD (Disables debug overlay, allows offline play)
DMO_MATCHMAKE (Has matchmaking versus offline build)
DMO_PVP_ONLY (Has PVP Only, remove for COOP)
RTX_DMO (Shows RTX on/off)
PREMISSION_PLANNING (Shows the new preplanning instead of the existing pregame menu)
*/

#define STEAM_DLC_SUPPORTER 1844910
#define STEAM_DLC_PREORDER_BONUS 1845130 

#define STEAM_APPID_CORE_GAME 1144200
#define STEAM_APPID_DEMO_GAME 2532100

// No not put code below // PRG-DEFINES-END

#define USE_RAW_VO // use raw wav files for VO
#define RON_NO_JUMP
#define RON_NO_SPRINT
#define RON_NO_INCREMENTAL_SPEED
//#define RON_COMBINED_AI
#define ENHANCED_SIGHT_DETECTION
// PRG-DEFINES-BEGIN
#define REPLAY_SYSTEM
#define CARRY_ARRESTED
#define NVIDIA_REFLEX_ENABLED
#define AMD_FSR_ENABLED
#define PREMISSION_BRIEFING_BEFORE_LOADOUT
#define PREMISSION_PLANNING
#define DMO_MATCHMAKE
#define DMO_BUILD
#define RTX_DMO
// PRG-DEFINES-END

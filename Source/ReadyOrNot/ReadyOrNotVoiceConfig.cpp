// Copyright Void Interactive, 2022

#include "ReadyOrNotVoiceConfig.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Voice Config - Get Voiceline"), STAT_GetVoiceLine, STATGROUP_VoiceConfig);

static FString VORootPath = "";

UReadyOrNotVoiceConfig* UReadyOrNotVoiceConfig::Get()
{
	#if !UE_BUILD_SHIPPING
	UE_CLOG(FUObjectThreadContext::Get().IsInConstructor, LogConfig, Fatal, TEXT("Do not call UReadyOrNotVoiceConfig::Get() in constructors"));
	#endif

	const UWorld* World = UBpGameplayHelperLib::GetWorldStatic();
	
	if (!IsValid(World))
		return nullptr;
	
	const AReadyOrNotGameState* GS = World->GetGameState<AReadyOrNotGameState>();
	
	if (!IsValid(GS))
		return nullptr;

	return GS->VoiceConfig;
}

void UReadyOrNotVoiceConfig::Init()
{
	VoicelineMap.Empty(100);

#if defined(TARGET_PS5) || defined(TARGET_PS4)
	VORootPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()) + "VO_PS/";
	VORootPath = VORootPath.ToLower();
#elif defined(TARGET_XB1) || defined(TARGET_XSX)
	VORootPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()) + "VO_XBOX/"; 
#else
	VORootPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()) + GetString("Path", "VO/");
#endif
	IFileManager::Get().IterateDirectoryRecursively(*VORootPath, [&](const TCHAR* FilenameOrDirectory, bool bIsDirectory)
	{
		if (bIsDirectory)
		{
			FString SpeakerRoot = FilenameOrDirectory;
			
			int32 LastSlashIndex = 0;
			SpeakerRoot.FindLastChar('/', LastSlashIndex);
			
			const FString SpeakerDirectoryName = SpeakerRoot.Right((SpeakerRoot.Len() - LastSlashIndex)-1);
			
			SpeakerRoot += "/";

			#if WITH_EDITOR
			FileExtension = "*.wav";
			#elif defined(TARGET_PS5) || defined(TARGET_PS4) || defined(TARGET_XB1) || defined(TARGET_XSX)
			FileExtension = "*.fsb";
			#else
			FileExtension = GetString("Ext", "*.ogg");
			#endif

			const FString FullPath = SpeakerRoot + FileExtension;

			TArray<FString> Files;
			IFileManager::Get().FindFiles(Files, *FullPath, true, false);
			
			VoicelineMap.Add(SpeakerDirectoryName, Files);
		}
		
		return true;
	});
}

FString UReadyOrNotVoiceConfig::GetConfigFileName() const
{
	return "DefaultVoice.ini";
}

FString UReadyOrNotVoiceConfig::GetConfigSectionName() const
{
	return "Settings";
}

FString UReadyOrNotVoiceConfig::GetFallbackConfigSectionName() const
{
	return "Settings";
}

int32 UReadyOrNotVoiceConfig::GetNextPlayIdx(FString Id, int32 FileCount)
{
	if (SequencedLookup.Find(Id))
	{
		FSequencedVOLookup* Lookup = &SequencedLookup[Id];
		Lookup->IncrementLookupIdx();
		
		#if WITH_EDITOR
		ensure(Lookup->LookupOrder.Num() == FileCount);
		#endif
		
		return Lookup->GetNextLookupIdx();
	}
	
	FSequencedVOLookup Lookup = FSequencedVOLookup(FileCount);
	Lookup.IncrementLookupIdx();
	
	#if WITH_EDITOR
	ensure(Lookup.LookupOrder.Num() == FileCount);
	#endif
	
	SequencedLookup.Add(Id, Lookup);
	return Lookup.GetNextLookupIdx();
}

bool UReadyOrNotVoiceConfig::GetRandomVoiceLine(FString Line, FString Speaker, FString& OutFilePath, FString& OutFileName)
{
	SCOPE_CYCLE_COUNTER(STAT_GetVoiceLine)
	
	if (Line.IsEmpty())
		return false;

	if (Speaker.IsEmpty())
		return false;

	if (TArray<FString>* VoiceLines = VoicelineMap.Find(Speaker))
	{
		TArray<const FString*, TFixedAllocator<1024>> CompatibleVoiceLines; // to avoid heap allocs, use a fixed size. 1024 voicelines is reasonable enough
		
		for (const FString& VoiceLine : *VoiceLines)
		{
			//V_LOGM(LogReadyOrNot, "Found line: %s speaker: %s", *VoiceLine, *Speaker)
			if (VoiceLine.Contains(Line + "_"))
			{
				CompatibleVoiceLines.Add(&VoiceLine);
			}
		}
		
		if (CompatibleVoiceLines.Num() > 0)
		{
			OutFileName = *CompatibleVoiceLines[GetNextPlayIdx(Line+Speaker, CompatibleVoiceLines.Num())];
			OutFilePath = VORootPath + Speaker + "/" + OutFileName;
			return true;
		}
	}

	#if !UE_BUILD_SHIPPING
	V_LOGM(LogReadyOrNot, "Unable to find voice lines for line: %s speaker: %s", *Line, *Speaker)
	#endif
	
	return false;
}

bool UReadyOrNotVoiceConfig::GetRandomVoiceLineForSpeaker(FString Speaker, FString& OutFilePath, FString& OutFileName)
{
	if (Speaker.IsEmpty())
		return false;

	if (TArray<FString>* Files = VoicelineMap.Find(Speaker))
	{
		if (Files->Num()>0)
		{
			OutFileName = (*Files)[FMath::RandRange(0, Files->Num() - 1)];
			OutFilePath = VORootPath + Speaker + "/" + OutFileName;
			return true;
		}
	}

	#if !UE_BUILD_SHIPPING
	V_LOGM(LogReadyOrNot, "Unable to find voice lines for speaker: %s", *Speaker)
	#endif
	
	return false;
}

bool UReadyOrNotVoiceConfig::GetSpecificVoiceLine(FString FileName, FString Speaker, FString& OutFilePath, FString& OutFileName)
{
	if (FileName.IsEmpty())
		return false;

	if (Speaker.IsEmpty())
		return false;
	
	if (TArray<FString>* VoiceLines = VoicelineMap.Find(Speaker))
	{
		for (const FString& VoiceLine : *VoiceLines)
		{
			if (VoiceLine == FileName)
			{
				OutFileName = VoiceLine;
				OutFilePath = VORootPath + Speaker + "/" + OutFileName;
				return true;
			}
		}
	}
	
	return false;
}


	// Prefixes
	const FString VO_PREFIXES::BARK = "[BARK]";
	const FString VO_PREFIXES::TELL = "[TELL]";
	const FString VO_PREFIXES::REPLY = "[REPLY]";

	// SWAT Commands
	const FString VO_SWAT_COMMAND::CALL_SC_MOVE_TO = "[CALL]SCMoveTo";
	const FString VO_SWAT_COMMAND::CALL_SC_FALL_IN = "[CALL]SCFallIn";
	const FString VO_SWAT_COMMAND::CALL_SC_FALL_IN_SNAKE = "[CALL]SCFallInSnake";
	const FString VO_SWAT_COMMAND::CALL_SC_FALL_IN_HALF_SNAKE = "[CALL]SCFallInHalfSnake";
	const FString VO_SWAT_COMMAND::CALL_SC_FALL_IN_DIAMOND = "[CALL]SCFallInDiamond";
	const FString VO_SWAT_COMMAND::CALL_SC_FALL_IN_FLOCK = "[CALL]SCFallInFlock";
	const FString VO_SWAT_COMMAND::CALL_SC_COVER = "[CALL]SCCover";
	const FString VO_SWAT_COMMAND::CALL_SC_HOLD = "[CALL]SCHold";
	const FString VO_SWAT_COMMAND::CALL_SC_RESUME = "[CALL]SCResume";
	const FString VO_SWAT_COMMAND::CALL_SC_DEPLOY_FLASHBANG = "[CALL]SCDeployFlashbang";
	const FString VO_SWAT_COMMAND::CALL_SC_DEPLOY_STINGER = "[CALL]SCDeployStinger";
	const FString VO_SWAT_COMMAND::CALL_SC_DEPLOY_CSGAS = "[CALL]SCDeployCSGas";
	const FString VO_SWAT_COMMAND::CALL_SC_DEPLOY_CHEMLIGHT = "[CALL]SCDeployChemlight";
	const FString VO_SWAT_COMMAND::CALL_SC_DEPLOY_SHIELD = "[CALL]Shield";
	const FString VO_SWAT_COMMAND::CALL_SC_ARREST = "[CALL]SCArrest";
	const FString VO_SWAT_COMMAND::CALL_SC_ARREST_MALE = "[CALL]ArrestMale";
	const FString VO_SWAT_COMMAND::CALL_SC_ARREST_FEMALE = "[CALL]ArrestFemale";
	const FString VO_SWAT_COMMAND::CALL_SC_COLLECT_EVIDENCE = "[CALL]SCCollectEvidence";
	const FString VO_SWAT_COMMAND::CALL_SC_DISARM_TRAP = "[CALL]SCDisarmTrap";
	const FString VO_SWAT_COMMAND::CALL_SC_DO_REPORT_TARGET = "[CALL]SCDoReportTarget";
	const FString VO_SWAT_COMMAND::CALL_SC_REPORT = "[CALL]SCReport";

	const FString VO_SWAT_COMMAND::CALL_SC_ENGAGE_LETHAL = "[CALL]SCEngageLethal";
	const FString VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL = "[CALL]SCEngageLessLethal";
	const FString VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL_PEPPER = "[CALL]EngageLessLethalPepper";
	const FString VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL_SPRAY = "[CALL]EngageLessLethalSpray";
	const FString VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL_BEANBAG = "[CALL]EngageLessLethalBeanbag";
	const FString VO_SWAT_COMMAND::CALL_SC_ENGAGE_LESS_LETHAL_TASER = "[CALL]EngageLessLethalTaser";

	const FString VO_SWAT_COMMAND::CALL_SC_CHASE = "[CALL]SCChase";
	const FString VO_SWAT_COMMAND::CALL_SC_STOP_CHASE = "[CALL]SCStopChase";

	const FString VO_SWAT_COMMAND::CALL_SC_TURN_AROUND = "[CALL]OrderTurnAround";

	const FString VO_SWAT_COMMAND::CALL_FOCUS = "[CALL]Focus";
	const FString VO_SWAT_COMMAND::CALL_UNFOCUS = "[CALL]Unfocus";
	const FString VO_SWAT_COMMAND::CALL_FOCUS_DOOR = "[CALL]FocusDoor";
	const FString VO_SWAT_COMMAND::CALL_FOCUS_OPENING = "[CALL]FocusOpening";
	const FString VO_SWAT_COMMAND::CALL_FOCUS_HALLWAY = "[CALL]FocusHallway";
	const FString VO_SWAT_COMMAND::CALL_FOCUS_TARGET = "[CALL]FocusTarget";

	const FString VO_SWAT_COMMAND::CALL_EXECUTE = "[CALL]Execute";
	const FString VO_SWAT_COMMAND::CALL_CANCEL = "[CALL]Cancel";

	const FString VO_SWAT_COMMAND::CALL_SC_FORGET = "[CALL]SCForget";

	const FString VO_SWAT_COMMAND::CALL_SC_STACK_UP = "[CALL]SCStackUp";
	const FString VO_SWAT_COMMAND::CALL_SC_STACK_UP_SPLIT = "[CALL]SCStackUpSplit";
	const FString VO_SWAT_COMMAND::CALL_SC_STACK_UP_LEFT = "[CALL]SCStackUpLeft";
	const FString VO_SWAT_COMMAND::CALL_SC_STACK_UP_RIGHT = "[CALL]SCStackUpRight";
	const FString VO_SWAT_COMMAND::CALL_SC_STACK_UP_SHIFT_SPLIT = "[CALL]SCStackUpShiftSplit";
	const FString VO_SWAT_COMMAND::CALL_SC_STACK_UP_SHIFT_LEFT = "[CALL]SCStackUpShiftLeft";
	const FString VO_SWAT_COMMAND::CALL_SC_STACK_UP_SHIFT_RIGHT = "[CALL]SCStackUpShiftRight";

	const FString VO_SWAT_COMMAND::CALL_SC_CHECK_DOOR = "[CALL]SCCheckDoor";
	const FString VO_SWAT_COMMAND::CALL_SC_PICK_LOCK = "[CALL]SCPickLock";
	const FString VO_SWAT_COMMAND::CALL_SC_REMOVE_DOOR_JAM = "[CALL]SCRemoveDoorJam";
	const FString VO_SWAT_COMMAND::CALL_SC_DEPLOY_MIRRORGUN = "[CALL]SCDeployMirrorgun";
	const FString VO_SWAT_COMMAND::CALL_SC_DEPLOY_DOOR_JAM = "[CALL]SCDeployDoorJam";
	const FString VO_SWAT_COMMAND::CALL_SC_CHECK_FOR_TRAP = "[CALL]SCCheckForTrap";

	const FString VO_SWAT_COMMAND::CALL_SC_OPEN_DOOR = "[CALL]SCOpenDoor";
	const FString VO_SWAT_COMMAND::CALL_SC_CLOSE_DOOR = "[CALL]SCCloseDoor";

	const FString VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR = "[CALL]SCMoveAndClear";
	const FString VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_FLASHBANG = "[CALL]SCMoveAndClearFlashbang";
	const FString VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_STINGER = "[CALL]SCMoveAndClearStinger";
	const FString VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_CSGAS = "[CALL]SCMoveAndClearCSGas";
	const FString VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_LAUNCHER = "[CALL]SCMoveAndClearLauncher";
	const FString VO_SWAT_COMMAND::CALL_SC_MOVE_AND_CLEAR_LEADER = "[CALL]SCMoveAndClearLeader";

	const FString VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR = "[CALL]SCOpenAndClear";
	const FString VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_FLASHBANG = "[CALL]SCOpenAndClearFlashbang";
	const FString VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_STINGER = "[CALL]SCOpenAndClearStinger";
	const FString VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_CSGAS = "[CALL]SCOpenAndClearCSGas";
	const FString VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_LAUNCHER = "[CALL]SCOpenAndClearLauncher";
	const FString VO_SWAT_COMMAND::CALL_SC_OPEN_AND_CLEAR_LEADER = "[CALL]SCOpenAndClearLeader";

	const FString VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR = "[CALL]SCKickAndClear";
	const FString VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_FLASHBANG = "[CALL]SCKickAndClearFlashbang";
	const FString VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_STINGER = "[CALL]SCKickAndClearStinger";
	const FString VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_CSGAS = "[CALL]SCKickAndClearCSGas";
	const FString VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_LAUNCHER = "[CALL]SCKickAndClearLauncher";
	const FString VO_SWAT_COMMAND::CALL_SC_KICK_AND_CLEAR_LEADER = "[CALL]SCKickAndClearLeader";

	const FString VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR = "[CALL]SCRamAndClear";
	const FString VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_FLASHBANG = "[CALL]SCRamAndClearFlashbang";
	const FString VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_STINGER = "[CALL]SCRamAndClearStinger";
	const FString VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_CSGAS = "[CALL]SCRamAndClearCSGas";
	const FString VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_LAUNCHER = "[CALL]SCRamAndClearLauncher";
	const FString VO_SWAT_COMMAND::CALL_SC_RAM_AND_CLEAR_LEADER = "[CALL]SCRamAndClearLeader";

	const FString VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR = "[CALL]SCShotgunClear";
	const FString VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_FLASHBANG = "[CALL]SCShotgunClearFlashbang";
	const FString VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_STINGER = "[CALL]SCShotgunClearStinger";
	const FString VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_CSGAS = "[CALL]SCShotgunClearCSGas";
	const FString VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_LAUNCHER = "[CALL]SCShotgunClearLauncher";
	const FString VO_SWAT_COMMAND::CALL_SC_SHOTGUN_AND_CLEAR_LEADER = "[CALL]SCShotgunClearLeader";

	const FString VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR = "[CALL]SCC2Clear";
	const FString VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_FLASHBANG = "[CALL]SCC2ClearFlashbang";
	const FString VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_STINGER = "[CALL]SCC2ClearStinger";
	const FString VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_CSGAS = "[CALL]SCC2ClearCSGas";
	const FString VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_LAUNCHER = "[CALL]SCC2AndClearLauncher";
	const FString VO_SWAT_COMMAND::CALL_SC_C2_AND_CLEAR_LEADER = "[CALL]SCC2AndClearLeader";

	const FString VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR = "[CALL]SCLeaderClear";
	const FString VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR_FLASHBANG = "[CALL]SCLeaderClearFlashbang";
	const FString VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR_STINGER = "[CALL]SCLeaderClearStinger";
	const FString VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR_CSGAS = "[CALL]SCLeaderClearCSGas";
	const FString VO_SWAT_COMMAND::CALL_SC_LEADER_AND_CLEAR_LAUNCHER = "[CALL]SCLeaderClearLauncher";

	const FString VO_SWAT_COMMAND::CALL_SC_SEARCH_AND_SECURE = "[CALL]SearchAndSecure";

	const FString VO_SWAT_COMMAND::CALL_SC_SWAP = "[CALL]SwapPositionGeneric";
	const FString VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_ALPHA = "[CALL]SwapPositionAlpha";
	const FString VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_BETA = "[CALL]SwapPositionBravo";
	const FString VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_CHARLIE = "[CALL]SwapPositionCharlie";
	const FString VO_SWAT_COMMAND::CALL_SC_SWAP_WITH_DELTA = "[CALL]SwapPositionDelta";

	const FString VO_SWAT_COMMAND::CALL_SCAN_DOOR = "[CALL]ScanDoorGeneric";
	const FString VO_SWAT_COMMAND::CALL_SCAN_DOOR_PIE = "[CALL]ScanDoorPIE";
	const FString VO_SWAT_COMMAND::CALL_SCAN_DOOR_SLIDE = "[CALL]Slide";
	const FString VO_SWAT_COMMAND::CALL_SCAN_DOOR_PEEK = "[CALL]Peek";

	const FString VO_SWAT_COMMAND::CALL_KILL_ME = "[CALL]KillMe";

	const FString VO_SWAT_GENERAL::RESPONSE_MOVE_TO = "[RESPONSE]MoveTo";
	const FString VO_SWAT_GENERAL::RESPONSE_FALL_IN = "[RESPONSE]FallIn";
	const FString VO_SWAT_GENERAL::RESPONSE_COVER = "[RESPONSE]Cover";
	const FString VO_SWAT_GENERAL::RESPONSE_HOLD = "[RESPONSE]Hold";
	const FString VO_SWAT_GENERAL::RESPONSE_DEPLOY_FLASHBANG = "[RESPONSE]DeployFlashbang";
	const FString VO_SWAT_GENERAL::RESPONSE_DEPLOY_STINGER = "[RESPONSE]DeployStinger";
	const FString VO_SWAT_GENERAL::RESPONSE_DEPLOY_CS_GAS = "[RESPONSE]DeployCSGas";
	const FString VO_SWAT_GENERAL::RESPONSE_DEPLOY_CHEMLIGHT = "[RESPONSE]DeployChemlight";
	const FString VO_SWAT_GENERAL::CALL_ARRESTING_SUSPECT = "[CALL]ArrestingSuspect";
	const FString VO_SWAT_GENERAL::CALL_ARRESTING_SUSPECT_MOVE = "[CALL]ArrestingSuspectMove";
	const FString VO_SWAT_GENERAL::CALL_ARRESTING_SUSPECT_COMPLETE = "[CALL]ArrestingSuspectComplete";
	const FString VO_SWAT_GENERAL::CALL_ARRESTING_CIVILIAN = "[CALL]ArrestingCivilian";
	const FString VO_SWAT_GENERAL::CALL_ARRESTING_CIVILIAN_MOVE = "[CALL]ArrestingCivilianMove";
	const FString VO_SWAT_GENERAL::CALL_ARRESTING_CIVILIAN_COMPLETE = "[CALL]ArrestingCivilianComplete";
	const FString VO_SWAT_GENERAL::CALL_DISARM_TRAP = "[CALL]DisarmTrap";
	const FString VO_SWAT_GENERAL::CALL_COLLECT_EVIDENCE = "[CALL]CollectEvidence";
	const FString VO_SWAT_GENERAL::CALL_COLLECT_EVIDENCE_MOVE = "[CALL]CollectEvidenceMove";
	const FString VO_SWAT_GENERAL::RESPONSE_ENGAGE_LETHAL = "[RESPONSE]EngageLethal";
	const FString VO_SWAT_GENERAL::RESPONSE_ENGAGE_LESS_LETHAL = "[RESPONSE]EngageLessLethal";
	const FString VO_SWAT_GENERAL::RESPONSE_CHASE = "[RESPONSE]Chase";
	const FString VO_SWAT_GENERAL::RESPONSE_FOCUS = "[RESPONSE]Focus";
	const FString VO_SWAT_GENERAL::RESPONSE_FORGOT = "[RESPONSE]Forget";
	const FString VO_SWAT_GENERAL::RESPONSE_STACK_UP = "[RESPONSE]StackUp";
	const FString VO_SWAT_GENERAL::RESPONSE_MIRRORGUN = "[RESPONSE]Mirrorgun";
	const FString VO_SWAT_GENERAL::RESPONSE_DOOR_JAM = "[RESPONSE]DoorJam";
	const FString VO_SWAT_GENERAL::RESPONSE_CLOSE_DOOR = "[RESPONSE]CloseDoor";
	const FString VO_SWAT_GENERAL::RESPONSE_OPEN_CLEAR_DOOR = "[RESPONSE]OpenClear";
	const FString VO_SWAT_GENERAL::RESPONSE_BREACH_KICK = "[RESPONSE]BreachKick";
	const FString VO_SWAT_GENERAL::RESPONSE_BREACH_SHOTGUN = "[RESPONSE]BreachShotgun";
	const FString VO_SWAT_GENERAL::RESPONSE_BREACH_C2 = "[RESPONSE]BreachC2";
	const FString VO_SWAT_GENERAL::CALL_SPOTTED_NONE = "[CALL]SpottedNone";
	const FString VO_SWAT_GENERAL::CALL_SPOTTED_CIVILIAN = "[CALL]SpottedCivilian1";
	const FString VO_SWAT_GENERAL::CALL_SPOTTED_MULTIPLE_CIVILIANS = "[CALL]SpottedCivilian2";
	const FString VO_SWAT_GENERAL::CALL_SPOTTED_SUSPECT = "[CALL]SpottedSuspect1";
	const FString VO_SWAT_GENERAL::CALL_SPOTTED_MULTIPLE_SUSPECTS = "[CALL]SpottedSuspect1";
	const FString VO_SWAT_GENERAL::CALL_SPOTTED_CIVILIAN_AND_SUSPECT = "[CALL]SpottedCivilianAndSuspect";
	const FString VO_SWAT_GENERAL::CALL_SPOTTED_MULTIPLE_CIVILIANS_AND_SUSPECTS = "[CALL]SpottedCivilianandSuspect2";
	const FString VO_SWAT_GENERAL::CALL_BLOCKED_BY_PLAYER = "[CALL]BlockedByPlayer";
	const FString VO_SWAT_GENERAL::CALL_WAITING = "[CALL]Waiting";
	const FString VO_SWAT_GENERAL::CALL_FRIENDLY_FIRE = "[CALL]FriendlyFire";
	const FString VO_SWAT_GENERAL::CALL_PAIN_GRUNT = "[CALL]Pain";
	const FString VO_SWAT_GENERAL::CALL_SHOT_AT_BY_SUSPECT = "[CALL]ShotAtBySuspect";
	const FString VO_SWAT_GENERAL::CALL_SUSPECT_SURRENDER_GUN_EXIT = "[CALL]SuspectSurrenderExit_Gun";
	const FString VO_SWAT_GENERAL::CALL_SUSPECT_SURRENDER_KNIFE_EXIT = "[CALL]SuspectSurrenderExit_Knife";
	const FString VO_SWAT_GENERAL::CALL_YELL_AT_SUSPECT = "[CALL]YellAtSuspect";
	const FString VO_SWAT_GENERAL::CALL_YELL_AT_CIVILIAN = "[CALL]YellAtCivilian";
	const FString VO_SWAT_GENERAL::CALL_YELL_HIDING = "[CALL]YellHiding";
	const FString VO_SWAT_GENERAL::CALL_REPORT_ARRESTED_SUSPECT = "[CALL]ReportArrestedSuspect";
	const FString VO_SWAT_GENERAL::CALL_REPORT_ARRESTED_CIVILIAN = "[CALL]ReportArrestedCivilian";
	const FString VO_SWAT_GENERAL::CALL_REPORT_DEAD_SUSPECT = "[CALL]ReportDeadSuspect";
	const FString VO_SWAT_GENERAL::CALL_REPORT_DEAD_CIVILIAN = "[CALL]ReportDeadCivilian";
	const FString VO_SWAT_GENERAL::CALL_REPORT_DEAD_SWAT = "[CALL]ReportDeadSWAT";
	const FString VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_SUSPECT = "[CALL]ReportIncapacitatedSuspect";
	const FString VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_CIVILIAN = "[CALL]ReportIncapacitatedCivilian";
	const FString VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_SWAT = "[CALL]ReportIncapacitatedSWAT";
	const FString VO_SWAT_GENERAL::RESPONSE_CHECK_FOR_TRAPS = "[RESPONSE]CheckForTraps";
	const FString VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_DOOR_JAM = "[RESPONSE]NegativeNoDoorJam";
	const FString VO_SWAT_GENERAL::RESPONSE_REMOVE_DOOR_JAM = "[RESPONSE]RemoveDoorJam";
	const FString VO_SWAT_GENERAL::RESPONSE_OPEN_DOOR = "[RESPONSE]OpenDoor";
	const FString VO_SWAT_GENERAL::RESPONSE_PICK_LOCK = "[RESPONSE]PickLock";
	const FString VO_SWAT_GENERAL::RESPONSE_CHECK_DOOR = "[RESPONSE]CheckDoor";
	const FString VO_SWAT_GENERAL::CALL_DOOR_LOCKED = "[CALL]DoorLocked";
	const FString VO_SWAT_GENERAL::CALL_DOOR_UNLOCKED = "[CALL]DoorUnlocked";
	const FString VO_SWAT_GENERAL::CALL_DOOR_JAMMED = "[CALL]DoorJammed";
	const FString VO_SWAT_GENERAL::CALL_DOOR_BLOCKED = "[CALL]DoorBlocked";
	const FString VO_SWAT_GENERAL::CALL_DOOR_LOCK_PICKED = "[CALL]DoorLockPicked";
	const FString VO_SWAT_GENERAL::CALL_SPOTTED_NO_TRAP = "[CALL]SpottedNoTrap";
	const FString VO_SWAT_GENERAL::CALL_SPOTTED_TRAP_NO_MIRRORGUN = "[CALL]SpottedTrapNoMirrorgun";
	const FString VO_SWAT_GENERAL::CALL_SPOTTED_TRAP_FLASHBANG = "[CALL]SpottedTrapFlashbang";
	const FString VO_SWAT_GENERAL::CALL_SPOTTED_TRAP_ALARM = "[CALL]SpottedTrapAlarm";
	const FString VO_SWAT_GENERAL::CALL_SPOTTED_TRAP_EXPLOSIVE = "[CALL]SpottedTrapExplosive";
	const FString VO_SWAT_GENERAL::CALL_TRAP_DISARMED = "[CALL]TrapDisarmed";
	const FString VO_SWAT_GENERAL::CALL_EVIDENCE_SPOTTED = "[CALL]EvidenceSpotted";
	const FString VO_SWAT_GENERAL::CALL_DOOR_WEDGE_PLACED = "[CALL]DoorWedgePlaced";
	const FString VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_FLASHBANG = "[RESPONSE]NegativeNoFlashbang";
	const FString VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_CSGAS = "[RESPONSE]NegativeNoCSGas";
	const FString VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_STINGER = "[RESPONSE]NegativeNoStinger";
	const FString VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_CHEMLIGHT = "[RESPONSE]NegativeNoChemlight";
	const FString VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_C2 = "[RESPONSE]NegativeNoC2";
	const FString VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_SHOTGUN = "[RESPONSE]NegativeNoShotgun";
	const FString VO_SWAT_GENERAL::RESPONSE_NEGATIVE_NO_MIRRORGUN = "[RESPONSE]NegativeNoMirrorgun";
	const FString VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC = "[RESPONSE]NegativeGeneric";
	const FString VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC = "[RESPONSE]RogerGeneric";
	const FString VO_SWAT_GENERAL::CALL_C2_PLACED = "[CALL]C2Placed";
	const FString VO_SWAT_GENERAL::CALL_SUSPECT_KILLED = "[CALL]SuspectKilled";
	const FString VO_SWAT_GENERAL::RESPONSE_ACKNOWLEDGE_COMMAND = "[RESPONSE]Command";
	const FString VO_SWAT_GENERAL::RESPONSE_FOLLOWING_COMMAND = "[RESPONSE]FollowingCommand";
	const FString VO_SWAT_GENERAL::CALL_INCAPACITATED_TARGET = "[CALL]IncapacitatedTarget";
	const FString VO_SWAT_GENERAL::CALL_BREACH_DONE = "[CALL]BreachDone";
	const FString VO_SWAT_GENERAL::RESPONSE_NEGATIVE_POSITION = "[RESPONSE]NegativePosition";

	const FString VO_SWAT_GENERAL::CALL_ESCORT_CIVILIAN = "[CALL]EscortCivilian";
	const FString VO_SWAT_GENERAL::CALL_ESCORT_SUSPECT = "[CALL]EscortSuspect";
	const FString VO_SWAT_GENERAL::CALL_EXIT = "[CALL]Exit";

	const FString VO_SWAT_GENERAL::CALL_ORDER_MOVE = "[CALL]OrderMove";
	const FString VO_SWAT_GENERAL::CALL_ORDER_MOVE_TO_ME = "[CALL]OrderMoveToMe";

	const FString VO_SWAT_GENERAL::CALL_TOC = "[CALL]PrefixTOC";
	const FString VO_SWAT_GENERAL::CALL_GOLD_TEAM = "[CALL]PrefixGoldTeam";
	const FString VO_SWAT_GENERAL::CALL_RED_TEAM = "[CALL]PrefixRedTeam";
	const FString VO_SWAT_GENERAL::CALL_BLUE_TEAM = "[CALL]PrefixBlueTeam";

	const FString VO_SWAT_GENERAL::CALL_WEAPON_DROP_GENERIC = "[CALL]WeaponDropGeneric";
	const FString VO_SWAT_GENERAL::CALL_WEAPON_DROP_KNIFE = "[CALL]WeaponDropKnife";

	const FString VO_SWAT_GENERAL::CALL_RED_OCCUPIES_DOOR = "[CALL]RedOccupiesDoor";
	const FString VO_SWAT_GENERAL::CALL_BLUE_OCCUPIES_DOOR = "[CALL]BlueOccupiesDoor";

	const FString VO_SWAT_GENERAL::CALL_COVER_AREA = "[CALL]CoverArea";

	const FString VO_SWAT_GENERAL::CALL_OPENING_FRONT = "[CALL]Opening";
	const FString VO_SWAT_GENERAL::CALL_OPENING_LEFT = "[CALL]OpeningLeft";
	const FString VO_SWAT_GENERAL::CALL_OPENING_RIGHT = "[CALL]OpeningRight";
	const FString VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_FRONT = "[CALL]OpeningHallwayInFront";
	const FString VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_LEFT = "[CALL]OpeningHallwayLeft";
	const FString VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_RIGHT = "[CALL]OpeningHallwayRight";
	const FString VO_SWAT_GENERAL::CALL_OPENING_MULTIPLE = "[CALL]OpeningMultiple";
	const FString VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_MULTIPLE = "[CALL]OpeningHallwayMultiple";

	const FString VO_SWAT_GENERAL::CALL_SEARCHING = "[CALL]Search";
	const FString VO_SWAT_GENERAL::CALL_SEARCHING_BED = "[CALL]SearchingBed";
	const FString VO_SWAT_GENERAL::CALL_SEARCHING_CLOSET = "[CALL]SearchingCloset";
	const FString VO_SWAT_GENERAL::CALL_SEARCHING_TABLE = "[CALL]SearchingTable";

	const FString VO_SWAT_GENERAL::CALL_SWAP = "[CALL]Swap";

	const FString VO_SWAT_GENERAL::CALL_TRAIT_KICKER = "[CALL]Kicker";
	const FString VO_SWAT_GENERAL::CALL_TRAIT_PARAMEDIC = "[CALL]Paramedic";
	const FString VO_SWAT_GENERAL::CALL_TRAIT_PACIFIER = "[CALL]Pacifier";
	const FString VO_SWAT_GENERAL::CALL_TRAIT_NEGOTIATOR = "[CALL]Negotiator";
	const FString VO_SWAT_GENERAL::CALL_TRAIT_EOD = "[CALL]EOD";
	const FString VO_SWAT_GENERAL::CALL_TRAIT_BORDER_PATROL = "[CALL]BorderPatrol";
	const FString VO_SWAT_GENERAL::CALL_TRAIT_MORAL_OFFICER = "[CALL]MoralOfficer";
	const FString VO_SWAT_GENERAL::CALL_TRAIT_BREACHER = "[CALL]Breacher";
	const FString VO_SWAT_GENERAL::CALL_TRAIT_VETERAN = "[CALL]Veteran";
	const FString VO_SWAT_GENERAL::CALL_TRAIT_SBAGS = "[CALL]SBAGS";

	// Suspect And Civilian Lines
	const FString VO_SUSPECTS_AND_CIVILIAN::IDLE = "Idle";
	const FString VO_SUSPECTS_AND_CIVILIAN::IDLE_ARRESTED = "[BARK]ArrestIdle";
	const FString VO_SUSPECTS_AND_CIVILIAN::IDLE_BOMB_VEST = "[BARK]BombVestIdle";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_BOMB_VEST_DETONATE = "[BARK]BombVestDet";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_BREATHING = "[BARK]Breathing";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_SHOOTING = "[TELL]Shooting";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_SHOOTING = "[BARK]Shooting";
	const FString VO_SUSPECTS_AND_CIVILIAN::REPLY_SHOOTING = "[REPLY]Shooting";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_PAIN = "[BARK]Pain";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_INCAP_IDLE = "[BARK]IncapIdle";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_DEATH = "[BARK]Death";
	const FString VO_SUSPECTS_AND_CIVILIAN::PLAYER_NEARLY_KILLS_ENEMY = "PlayerKillsEnemy";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_PLAYER_KILLS_ENEMY = "[TELL]PlayerKillsEnemy";
	const FString VO_SUSPECTS_AND_CIVILIAN::REPLY_PLAYER_KILLS_ENEMY = "[REPLY]PlayerKillsEnemy";
	const FString VO_SUSPECTS_AND_CIVILIAN::RELOADING = "Reloading";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_RELOADING = "[TELL]Reloading";
	const FString VO_SUSPECTS_AND_CIVILIAN::REPLY_RELOADING = "[REPLY]Reloading";
	const FString VO_SUSPECTS_AND_CIVILIAN::COVER = "Cover";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_COVER = "[TELL]Cover";
	const FString VO_SUSPECTS_AND_CIVILIAN::REPLY_COVER = "[REPLY]Cover";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_TRIGGER_ACTIVATED = "[BARK]TriggerActivated";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_GOING_TO_COVER = "[BARK]GoingToCover";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_COVER_TRANSITION = "[BARK]CoverTransition";
	const FString VO_SUSPECTS_AND_CIVILIAN::FLEEING = "Fleeing";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_FLEEING = "[TELL]Fleeing";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_PLAYER_RELOADING = "[BARK]PlayerReload";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_COMPLIANT = "[BARK]Compliant";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_NON_COMPLIANT = "[BARK]NonCompliant";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_CALL_OUT_FOR_HELP = "[BARK]CallOutForHelp";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_ARRESTED = "[BARK]Arrested";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_PLAYER_SEEN = "[BARK]PlayerSeen";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_DOOR_SEEN_USED = "[BARK]DoorSeenUsed";
	const FString VO_SUSPECTS_AND_CIVILIAN::DOOR_SEEN_USED = "DoorSeenUsed";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_FLASHLIGHT_SEEN = "[BARK]FlashlightSeen";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_BASHED = "[BARK]Bashed";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_STUNNED = "[BARK]Stunned";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_GASSED = "[BARK]Gassed";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_PEPPERED = "[BARK]Peppered";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_TASERED = "[BARK]Tased";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_FLASHED = "[BARK]Flashed";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_IMMUNE = "[BARK]Immune";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_BASH_IMMUNE = "[BARK]BashImmune";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_STUN_IMMUNE = "[BARK]StunImmune";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_GAS_IMMUNE = "[BARK]GasImmune";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_PEPPER_IMMUNE = "[BARK]PepperImmune";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_TASER_IMMUNE = "[BARK]TaserImmune";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_FLASH_IMMUNE = "[BARK]FlashImmune";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_HEARD_SWAT = "[BARK]HeardSWAT";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_HEARD_SWAT = "[TELL]HeardSWAT";
	const FString VO_SUSPECTS_AND_CIVILIAN::REPLY_HEARD_SWAT = "[REPLY]HeardSWAT";
	const FString VO_SUSPECTS_AND_CIVILIAN::HEARD_SWAT = "HeardSWAT";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_GETTING_FLANKED = "[BARK]GettingFlanked";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_GETTING_FLANKED = "[TELL]GettingFlanked";
	const FString VO_SUSPECTS_AND_CIVILIAN::REPLY_GETTING_FLANKED = "[REPLY]GettingFlanked";
	const FString VO_SUSPECTS_AND_CIVILIAN::GETTING_FLANKED = "GettingFlanked";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_FLANKING = "[Bark]Flanking";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_CHARGING = "[BARK]Charging";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_CHARGING = "[TELL]Charging";
	const FString VO_SUSPECTS_AND_CIVILIAN::REPLY_CHARGING = "[REPLY]Charging";
	const FString VO_SUSPECTS_AND_CIVILIAN::CHARGING = "Charging";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_HIT_THE_PLAYER = "[BARK]HitThePlayer";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_HIT_THE_PLAYER = "[TELL]HitThePlayer";
	const FString VO_SUSPECTS_AND_CIVILIAN::REPLY_HIT_THE_PLAYER = "[REPLY]HitThePlayer";
	const FString VO_SUSPECTS_AND_CIVILIAN::HIT_THE_PLAYER = "HitThePlayer";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_FELLOW_AI_KILLED = "[BARK]FellowAIKilled";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_FELLOW_AI_KILLED = "[TELL]FellowAIKilled";
	const FString VO_SUSPECTS_AND_CIVILIAN::REPLY_FELLOW_AI_KILLED = "[REPLY]FellowAIKilled";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_MID_COMBAT_NO_COVER_CALLOUT = "[BARK]MidCombatNoCoverCallout";
	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_PLAYER_KILLS_CIVILIAN = "[BARK]PlayerKillsCivilian";

	const FString VO_SUSPECTS_AND_CIVILIAN::BARK_HESITATE = "[BARK]Hesitating";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_HESITATION_GREY = "[TELL]HesitationGrey";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_HESITATION_RED = "[TELL]HesitationRed";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_HESITATION_GREEN = "[TELL]HesitationGreen";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_DUELING = "[TELL]Dueling";
	const FString VO_SUSPECTS_AND_CIVILIAN::REPLY_DUELING = "[REPLY]Dueling";
	const FString VO_SUSPECTS_AND_CIVILIAN::DUELING = "Dueling";

	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_FLANKING = "[TELL]Flanking";
	const FString VO_SUSPECTS_AND_CIVILIAN::REPLY_FLANKING = "[REPLY]Flanking";
	const FString VO_SUSPECTS_AND_CIVILIAN::FLANKING = "Flanking";

	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_HIDING = "[TELL]Hiding";
	const FString VO_SUSPECTS_AND_CIVILIAN::REPLY_HIDING = "[REPLY]Hiding";
	const FString VO_SUSPECTS_AND_CIVILIAN::HIDING = "Hiding";

	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_TRACKING = "[TELL]Tracking";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_WAITING = "[TELL]Waiting";
	const FString VO_SUSPECTS_AND_CIVILIAN::REPLY_WAITING = "[REPLY]Waiting";
	const FString VO_SUSPECTS_AND_CIVILIAN::WAITING = "Waiting";
	const FString VO_SUSPECTS_AND_CIVILIAN::TELL_SUPPRESSION = "[TELL]Suppression";
	const FString VO_SUSPECTS_AND_CIVILIAN::REPLY_SUPPRESSION = "[REPLY]Suppression";
	const FString VO_SUSPECTS_AND_CIVILIAN::SUPPRESSION = "Suppression";

    const FString VO_SUSPECTS_AND_CIVILIAN::KNIFE_THE_PLAYER = "[BARK]KnifeThePlayer";

    const FString VO_SUSPECTS_AND_CIVILIAN::BARK_PICKED_UP = "[BARK]PickedUp";
    const FString VO_SUSPECTS_AND_CIVILIAN::BARK_HOSTAGE = "[BARK]Hostage";
    const FString VO_SUSPECTS_AND_CIVILIAN::BARK_KILL_CIVILIAN = "[BARK]KillCivilian";
    const FString VO_SUSPECTS_AND_CIVILIAN::BARK_SUSPICIOUS = "[BARK]Suspicious";
    const FString VO_SUSPECTS_AND_CIVILIAN::SUSPICIOUS = "Suspicious";

    const FString VO_SUSPECTS_AND_CIVILIAN::BARK_ANNOUNCE_KILL = "[BARK]AnnounceKill";

	// TOC Lines
	const FString VO_TOC::TOC_CHARACTER_NAME = "toc";
	const FString VO_TOC::TOC_PREFIX = "prefix";
	const FString VO_TOC::TOC_ARREST = "arrest";
	const FString VO_TOC::TOC_INCAPACITATED = "wounded";
	const FString VO_TOC::TOC_ROE_VIOLATE = "roeviolate";
	const FString VO_TOC::TOC_DEATH = "death";
	const FString VO_TOC::TOC_MISSION_COMPLETION = "completion";
	const FString VO_TOC::TOC_MISSION_FAILED = "failed";
	const FString VO_TOC::TOC_HOTEL_ENTRY = "hotel";
	const FString VO_TOC::TOC_HOTEL_VEHICLE_REPORT = "hotel3b";
	const FString VO_TOC::TOC_SUSPECT_IN_CUSTODY = "story3";

// Copyright Void Interactive, 2022

#pragma once

#include "GameplayConfig.h"
#include "ReadyOrNotVoiceConfig.generated.h"

USTRUCT(BlueprintType)
struct FSequencedVOLookup
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<int32> LookupOrder;

	FSequencedVOLookup()
	{
		LookupOrder = {};
	}

	FSequencedVOLookup(int32 InFileCount)
	{
		while (LookupOrder.Num() < InFileCount)
		{
			LookupOrder.AddUnique(FMath::RandRange(0, InFileCount-1));
		}
	}

	int32 GetNextLookupIdx()
	{
		return LookupOrder.Num() > 0 ? LookupOrder[0] : -1;
	}

	void IncrementLookupIdx()
	{
		#if WITH_EDITOR
		ensure(LookupOrder.Num() > 0);
		#endif
		
		if (LookupOrder.Num() > 0)
		{
			int32 Idx = LookupOrder[0];
			LookupOrder.Add(Idx);
			LookupOrder.RemoveAt(0);
		}
	}
};

DECLARE_STATS_GROUP(TEXT("ReadyOrNotVoiceConfig"), STATGROUP_VoiceConfig, STATCAT_Advanced);

UCLASS()
class READYORNOT_API UReadyOrNotVoiceConfig final : public UGameplayConfig
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, DisplayName = "Get Voice Config")
	static UReadyOrNotVoiceConfig* Get();

	void Init();
	
protected:
	virtual FString GetConfigFileName() const override;
	virtual FString GetConfigSectionName() const override;
	virtual FString GetFallbackConfigSectionName() const override;

	UPROPERTY()
	TMap<FString, FSequencedVOLookup> SequencedLookup;

	int32 GetNextPlayIdx(FString Id, int32 FileCount);

	TMap<FString, TArray<FString>> VoicelineMap;
	FString FileExtension;
	
public:
	// returns a random voice line using the line type
	bool GetRandomVoiceLine(FString Line, FString Speaker, FString& OutFilePath, FString& OutFileName);
	bool GetRandomVoiceLineForSpeaker(FString Speaker, FString& OutFilePath, FString& OutFileName);

	// returns a specific voice line using the line file name
	bool GetSpecificVoiceLine(FString FileName, FString Speaker, FString& OutFilePath, FString& OutFileName);

	FString GetVoiceFileExtension() const { return FileExtension; }
};

class READYORNOT_API VO_PREFIXES
{
public:
	static const FString BARK;
	static const FString TELL;
	static const FString REPLY;
};

class READYORNOT_API VO_SWAT_COMMAND
{
public:
	static const FString CALL_SC_MOVE_TO;
	static const FString CALL_SC_FALL_IN;
	static const FString CALL_SC_FALL_IN_SNAKE;
	static const FString CALL_SC_FALL_IN_HALF_SNAKE;
	static const FString CALL_SC_FALL_IN_DIAMOND;
	static const FString CALL_SC_FALL_IN_FLOCK;
	static const FString CALL_SC_COVER;
	static const FString CALL_SC_HOLD;
	static const FString CALL_SC_RESUME;
	static const FString CALL_SC_DEPLOY_FLASHBANG;
	static const FString CALL_SC_DEPLOY_STINGER;
	static const FString CALL_SC_DEPLOY_CSGAS;
	static const FString CALL_SC_DEPLOY_CHEMLIGHT;
	static const FString CALL_SC_DEPLOY_SHIELD;
	static const FString CALL_SC_ARREST;
	static const FString CALL_SC_ARREST_MALE;
	static const FString CALL_SC_ARREST_FEMALE;
	static const FString CALL_SC_COLLECT_EVIDENCE;
	static const FString CALL_SC_DISARM_TRAP;
	static const FString CALL_SC_DO_REPORT_TARGET;
	static const FString CALL_SC_REPORT;
	
	static const FString CALL_SC_ENGAGE_LETHAL;
	static const FString CALL_SC_ENGAGE_LESS_LETHAL;
	static const FString CALL_SC_ENGAGE_LESS_LETHAL_PEPPER;
	static const FString CALL_SC_ENGAGE_LESS_LETHAL_SPRAY;
	static const FString CALL_SC_ENGAGE_LESS_LETHAL_BEANBAG;
	static const FString CALL_SC_ENGAGE_LESS_LETHAL_TASER;
	
	static const FString CALL_SC_CHASE;
	static const FString CALL_SC_STOP_CHASE;
	
	static const FString CALL_SC_TURN_AROUND;
	
	static const FString CALL_SC_FORGET;
	
	static const FString CALL_FOCUS;
	static const FString CALL_UNFOCUS;
	static const FString CALL_FOCUS_OPENING;
	static const FString CALL_FOCUS_DOOR;
	static const FString CALL_FOCUS_HALLWAY;
	static const FString CALL_FOCUS_TARGET;
	
	static const FString CALL_EXECUTE;
	static const FString CALL_CANCEL;
	
	static const FString CALL_SC_STACK_UP;
	static const FString CALL_SC_STACK_UP_SPLIT;
	static const FString CALL_SC_STACK_UP_LEFT;
	static const FString CALL_SC_STACK_UP_RIGHT;
	static const FString CALL_SC_STACK_UP_SHIFT_SPLIT;
	static const FString CALL_SC_STACK_UP_SHIFT_LEFT;
	static const FString CALL_SC_STACK_UP_SHIFT_RIGHT;
	static const FString CALL_SC_CHECK_DOOR;
	static const FString CALL_SC_PICK_LOCK;
	static const FString CALL_SC_REMOVE_DOOR_JAM;
	static const FString CALL_SC_DEPLOY_MIRRORGUN;
	static const FString CALL_SC_DEPLOY_DOOR_JAM;
	static const FString CALL_SC_CHECK_FOR_TRAP;
	
    static const FString CALL_SC_OPEN_DOOR;
	static const FString CALL_SC_CLOSE_DOOR;
	
	static const FString CALL_SC_OPEN_AND_CLEAR;
	static const FString CALL_SC_OPEN_AND_CLEAR_FLASHBANG;
	static const FString CALL_SC_OPEN_AND_CLEAR_STINGER;
	static const FString CALL_SC_OPEN_AND_CLEAR_CSGAS;
	static const FString CALL_SC_OPEN_AND_CLEAR_LAUNCHER;
	static const FString CALL_SC_OPEN_AND_CLEAR_LEADER;
	
	static const FString CALL_SC_KICK_AND_CLEAR;
	static const FString CALL_SC_KICK_AND_CLEAR_FLASHBANG;
    static const FString CALL_SC_KICK_AND_CLEAR_STINGER;
    static const FString CALL_SC_KICK_AND_CLEAR_CSGAS;
    static const FString CALL_SC_KICK_AND_CLEAR_LAUNCHER;
    static const FString CALL_SC_KICK_AND_CLEAR_LEADER;
	
	static const FString CALL_SC_RAM_AND_CLEAR;
	static const FString CALL_SC_RAM_AND_CLEAR_FLASHBANG;
    static const FString CALL_SC_RAM_AND_CLEAR_STINGER;
    static const FString CALL_SC_RAM_AND_CLEAR_CSGAS;
    static const FString CALL_SC_RAM_AND_CLEAR_LAUNCHER;
    static const FString CALL_SC_RAM_AND_CLEAR_LEADER;
	
	static const FString CALL_SC_SHOTGUN_AND_CLEAR;
	static const FString CALL_SC_SHOTGUN_AND_CLEAR_FLASHBANG;
	static const FString CALL_SC_SHOTGUN_AND_CLEAR_STINGER;
	static const FString CALL_SC_SHOTGUN_AND_CLEAR_CSGAS;
	static const FString CALL_SC_SHOTGUN_AND_CLEAR_LAUNCHER;
	static const FString CALL_SC_SHOTGUN_AND_CLEAR_LEADER;
	
	static const FString CALL_SC_C2_AND_CLEAR;
	static const FString CALL_SC_C2_AND_CLEAR_FLASHBANG;
	static const FString CALL_SC_C2_AND_CLEAR_STINGER;
	static const FString CALL_SC_C2_AND_CLEAR_CSGAS;
	static const FString CALL_SC_C2_AND_CLEAR_LAUNCHER;
	static const FString CALL_SC_C2_AND_CLEAR_LEADER;
	
	static const FString CALL_SC_LEADER_AND_CLEAR;
	static const FString CALL_SC_LEADER_AND_CLEAR_FLASHBANG;
	static const FString CALL_SC_LEADER_AND_CLEAR_STINGER;
	static const FString CALL_SC_LEADER_AND_CLEAR_CSGAS;
	static const FString CALL_SC_LEADER_AND_CLEAR_LAUNCHER;
	
	static const FString CALL_SC_MOVE_AND_CLEAR;
	static const FString CALL_SC_MOVE_AND_CLEAR_FLASHBANG;
	static const FString CALL_SC_MOVE_AND_CLEAR_STINGER;
	static const FString CALL_SC_MOVE_AND_CLEAR_CSGAS;
	static const FString CALL_SC_MOVE_AND_CLEAR_LAUNCHER;
	static const FString CALL_SC_MOVE_AND_CLEAR_LEADER;
	
	static const FString CALL_SC_SEARCH_AND_SECURE;
	
	static const FString CALL_SC_SWAP;
	static const FString CALL_SC_SWAP_WITH_ALPHA;
	static const FString CALL_SC_SWAP_WITH_BETA;
	static const FString CALL_SC_SWAP_WITH_CHARLIE;
	static const FString CALL_SC_SWAP_WITH_DELTA;
	
	static const FString CALL_SCAN_DOOR;
	static const FString CALL_SCAN_DOOR_PIE;
	static const FString CALL_SCAN_DOOR_SLIDE;
	static const FString CALL_SCAN_DOOR_PEEK;
	
	static const FString CALL_KILL_ME;
};

class READYORNOT_API VO_SWAT_GENERAL
{
public:
	static const FString RESPONSE_MOVE_TO;
	static const FString RESPONSE_FALL_IN;
	static const FString RESPONSE_COVER;
	static const FString RESPONSE_HOLD;
	static const FString RESPONSE_DEPLOY_FLASHBANG;
	static const FString RESPONSE_DEPLOY_STINGER;
	static const FString RESPONSE_DEPLOY_CS_GAS;
	static const FString RESPONSE_DEPLOY_CHEMLIGHT;
	static const FString CALL_ARRESTING_SUSPECT;
	static const FString CALL_ARRESTING_SUSPECT_MOVE;
	static const FString CALL_ARRESTING_SUSPECT_COMPLETE;
	static const FString CALL_ARRESTING_CIVILIAN;
	static const FString CALL_ARRESTING_CIVILIAN_MOVE;
	static const FString CALL_ARRESTING_CIVILIAN_COMPLETE;
	static const FString CALL_DISARM_TRAP;
	static const FString CALL_COLLECT_EVIDENCE;
	static const FString CALL_COLLECT_EVIDENCE_MOVE;
	static const FString RESPONSE_ENGAGE_LETHAL;
	static const FString RESPONSE_ENGAGE_LESS_LETHAL;
	static const FString RESPONSE_CHASE;
	static const FString RESPONSE_FOCUS;
	static const FString RESPONSE_FORGOT;
	static const FString RESPONSE_STACK_UP;
	static const FString RESPONSE_MIRRORGUN;
	static const FString RESPONSE_DOOR_JAM;
	static const FString RESPONSE_CLOSE_DOOR;
	static const FString RESPONSE_OPEN_CLEAR_DOOR;
	static const FString RESPONSE_BREACH_KICK;
	static const FString RESPONSE_BREACH_SHOTGUN;
	static const FString RESPONSE_BREACH_C2;
	static const FString CALL_SPOTTED_NONE;
	static const FString CALL_SPOTTED_CIVILIAN;
	static const FString CALL_SPOTTED_MULTIPLE_CIVILIANS;
	static const FString CALL_SPOTTED_SUSPECT;
	static const FString CALL_SPOTTED_MULTIPLE_SUSPECTS;
	static const FString CALL_SPOTTED_CIVILIAN_AND_SUSPECT;
	static const FString CALL_SPOTTED_MULTIPLE_CIVILIANS_AND_SUSPECTS;
	static const FString CALL_BLOCKED_BY_PLAYER;
	static const FString CALL_WAITING;
	static const FString CALL_FRIENDLY_FIRE;
	static const FString CALL_PAIN_GRUNT;
	static const FString CALL_SHOT_AT_BY_SUSPECT;
	static const FString CALL_SUSPECT_SURRENDER_GUN_EXIT;
	static const FString CALL_SUSPECT_SURRENDER_KNIFE_EXIT;
	static const FString CALL_YELL_AT_SUSPECT;
	static const FString CALL_YELL_AT_CIVILIAN;
	static const FString CALL_YELL_HIDING;
	static const FString CALL_REPORT_ARRESTED_SUSPECT;
    static const FString CALL_REPORT_ARRESTED_CIVILIAN;
    static const FString CALL_REPORT_DEAD_SUSPECT;
    static const FString CALL_REPORT_DEAD_CIVILIAN;
    static const FString CALL_REPORT_DEAD_SWAT;
    static const FString CALL_REPORT_INCAPACITATED_SUSPECT;
    static const FString CALL_REPORT_INCAPACITATED_CIVILIAN;
    static const FString CALL_REPORT_INCAPACITATED_SWAT;
	static const FString RESPONSE_CHECK_FOR_TRAPS;                    
	static const FString RESPONSE_NEGATIVE_NO_DOOR_JAM;              
	static const FString RESPONSE_REMOVE_DOOR_JAM;         
	static const FString RESPONSE_OPEN_DOOR;            
	static const FString RESPONSE_PICK_LOCK;           
	static const FString RESPONSE_CHECK_DOOR;    
	static const FString CALL_DOOR_LOCKED;   
	static const FString CALL_DOOR_UNLOCKED;        
	static const FString CALL_DOOR_JAMMED;       
	static const FString CALL_DOOR_BLOCKED;           
	static const FString CALL_DOOR_LOCK_PICKED;
    static const FString CALL_SPOTTED_NO_TRAP;
    static const FString CALL_SPOTTED_TRAP_NO_MIRRORGUN;
	static const FString CALL_SPOTTED_TRAP_FLASHBANG;
	static const FString CALL_SPOTTED_TRAP_ALARM;
	static const FString CALL_SPOTTED_TRAP_EXPLOSIVE;
	static const FString CALL_TRAP_DISARMED;
	static const FString CALL_EVIDENCE_SPOTTED;
	static const FString CALL_DOOR_WEDGE_PLACED;
	static const FString RESPONSE_NEGATIVE_NO_FLASHBANG;
	static const FString RESPONSE_NEGATIVE_NO_CSGAS;
	static const FString RESPONSE_NEGATIVE_NO_STINGER;
	static const FString RESPONSE_NEGATIVE_NO_CHEMLIGHT;
	static const FString RESPONSE_NEGATIVE_NO_C2;
	static const FString RESPONSE_NEGATIVE_NO_SHOTGUN;
	static const FString RESPONSE_NEGATIVE_NO_MIRRORGUN;
	static const FString RESPONSE_NEGATIVE_GENERIC;
	static const FString RESPONSE_ROGER_GENERIC;
	static const FString CALL_C2_PLACED;
	static const FString CALL_SUSPECT_KILLED;
	static const FString RESPONSE_ACKNOWLEDGE_COMMAND;
	static const FString RESPONSE_FOLLOWING_COMMAND;
	static const FString CALL_INCAPACITATED_TARGET;
	static const FString CALL_BREACH_DONE;
	static const FString RESPONSE_NEGATIVE_POSITION;
	
	static const FString CALL_ESCORT_CIVILIAN;
	static const FString CALL_ESCORT_SUSPECT;
	static const FString CALL_EXIT;
	
	static const FString CALL_ORDER_MOVE;
	static const FString CALL_ORDER_MOVE_TO_ME;
	
	static const FString CALL_TOC;
	static const FString CALL_GOLD_TEAM;
	static const FString CALL_RED_TEAM;
	static const FString CALL_BLUE_TEAM;
	
	static const FString CALL_WEAPON_DROP_GENERIC;
	static const FString CALL_WEAPON_DROP_KNIFE;
	
	static const FString CALL_RED_OCCUPIES_DOOR;
	static const FString CALL_BLUE_OCCUPIES_DOOR;
	
	static const FString CALL_COVER_AREA;
	
	static const FString CALL_OPENING_FRONT;
	static const FString CALL_OPENING_LEFT;
	static const FString CALL_OPENING_RIGHT;
	static const FString CALL_OPENING_HALLWAY_FRONT;
	static const FString CALL_OPENING_HALLWAY_LEFT;
	static const FString CALL_OPENING_HALLWAY_RIGHT;
	static const FString CALL_OPENING_MULTIPLE;
	static const FString CALL_OPENING_HALLWAY_MULTIPLE;
	
	static const FString CALL_SEARCHING;
	static const FString CALL_SEARCHING_BED;
	static const FString CALL_SEARCHING_CLOSET;
	static const FString CALL_SEARCHING_TABLE;
	
	static const FString CALL_SWAP;

	static const FString CALL_TRAIT_KICKER;
	static const FString CALL_TRAIT_PARAMEDIC;
	static const FString CALL_TRAIT_PACIFIER;
	static const FString CALL_TRAIT_NEGOTIATOR;
	static const FString CALL_TRAIT_EOD;
	static const FString CALL_TRAIT_BORDER_PATROL;
	static const FString CALL_TRAIT_MORAL_OFFICER;
	static const FString CALL_TRAIT_BREACHER;
	static const FString CALL_TRAIT_VETERAN;
	static const FString CALL_TRAIT_SBAGS;
};

class READYORNOT_API VO_SUSPECTS_AND_CIVILIAN
{
public:
	static const FString IDLE;
	static const FString IDLE_ARRESTED;
	static const FString IDLE_BOMB_VEST;
	static const FString BARK_BOMB_VEST_DETONATE;
	static const FString BARK_BREATHING;
	static const FString TELL_SHOOTING;
	static const FString BARK_SHOOTING;
	static const FString REPLY_SHOOTING;
	static const FString BARK_DEATH;
	static const FString BARK_PAIN;
	static const FString BARK_INCAP_IDLE;
	static const FString PLAYER_NEARLY_KILLS_ENEMY;
	static const FString TELL_PLAYER_KILLS_ENEMY;
	static const FString REPLY_PLAYER_KILLS_ENEMY;
	static const FString RELOADING;
	static const FString TELL_RELOADING;
	static const FString REPLY_RELOADING;
	static const FString COVER;
	static const FString TELL_COVER;
	static const FString REPLY_COVER;
	static const FString BARK_TRIGGER_ACTIVATED;
	static const FString BARK_GOING_TO_COVER;
	static const FString BARK_COVER_TRANSITION;
	static const FString FLEEING;
	static const FString TELL_FLEEING;
	static const FString BARK_PLAYER_RELOADING;
	static const FString BARK_COMPLIANT;
	static const FString BARK_NON_COMPLIANT;
	static const FString BARK_CALL_OUT_FOR_HELP;
	static const FString BARK_ARRESTED;
	static const FString BARK_PLAYER_SEEN;
	static const FString BARK_DOOR_SEEN_USED;
	static const FString DOOR_SEEN_USED;
	static const FString BARK_FLASHLIGHT_SEEN;
	static const FString BARK_BASHED;
	static const FString BARK_STUNNED;
	static const FString BARK_GASSED;
	static const FString BARK_PEPPERED;
	static const FString BARK_TASERED;
	static const FString BARK_FLASHED;
	static const FString BARK_IMMUNE;
	static const FString BARK_BASH_IMMUNE;
	static const FString BARK_STUN_IMMUNE;
	static const FString BARK_GAS_IMMUNE;
	static const FString BARK_PEPPER_IMMUNE;
	static const FString BARK_TASER_IMMUNE;
	static const FString BARK_FLASH_IMMUNE;
	static const FString BARK_HEARD_SWAT;
	static const FString TELL_HEARD_SWAT;
	static const FString REPLY_HEARD_SWAT;
	static const FString HEARD_SWAT;
	static const FString BARK_GETTING_FLANKED;
	static const FString TELL_GETTING_FLANKED;
	static const FString REPLY_GETTING_FLANKED;
	static const FString GETTING_FLANKED;
	static const FString BARK_FLANKING;
	static const FString BARK_CHARGING;
	static const FString TELL_CHARGING;
	static const FString REPLY_CHARGING;
	static const FString CHARGING;
	static const FString BARK_HIT_THE_PLAYER;
	static const FString TELL_HIT_THE_PLAYER;
	static const FString REPLY_HIT_THE_PLAYER;
	static const FString HIT_THE_PLAYER;
	static const FString BARK_FELLOW_AI_KILLED;
	static const FString TELL_FELLOW_AI_KILLED;
	static const FString REPLY_FELLOW_AI_KILLED;
	static const FString BARK_MID_COMBAT_NO_COVER_CALLOUT;
	static const FString BARK_PLAYER_KILLS_CIVILIAN;
	static const FString BARK_HESITATE;
	static const FString TELL_HESITATION_GREY;
	static const FString TELL_HESITATION_RED;
	static const FString TELL_HESITATION_GREEN;
	static const FString TELL_DUELING;
	static const FString REPLY_DUELING;
	static const FString DUELING;
	static const FString TELL_FLANKING;
	static const FString REPLY_FLANKING;
	static const FString FLANKING;
	static const FString TELL_HIDING;
	static const FString REPLY_HIDING;
	static const FString HIDING;
	static const FString TELL_TRACKING;
	static const FString TELL_WAITING;
	static const FString REPLY_WAITING;
	static const FString WAITING;
	static const FString TELL_SUPPRESSION;
	static const FString REPLY_SUPPRESSION;
	static const FString SUPPRESSION;
	static const FString KNIFE_THE_PLAYER;
	static const FString BARK_PICKED_UP;
	static const FString BARK_HOSTAGE;
	static const FString BARK_ANNOUNCE_KILL;
	static const FString BARK_KILL_CIVILIAN;
	static const FString BARK_SUSPICIOUS;
	static const FString SUSPICIOUS;
};

class READYORNOT_API VO_TOC
{
public:
	static const FString TOC_CHARACTER_NAME;
	static const FString TOC_PREFIX;
	static const FString TOC_ARREST;
	static const FString TOC_INCAPACITATED;   
	static const FString TOC_ROE_VIOLATE;   
	static const FString TOC_DEATH;
	static const FString TOC_MISSION_COMPLETION;
	static const FString TOC_MISSION_FAILED;
	static const FString TOC_HOTEL_ENTRY;
	static const FString TOC_HOTEL_VEHICLE_REPORT;
	static const FString TOC_SUSPECT_IN_CUSTODY;
};

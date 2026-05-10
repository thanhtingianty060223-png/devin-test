// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Enums.h"
#include "Engine/DataAsset.h"
#include "DeployableData.h"
#include "Engine/DataTable.h"
#include "Sound/SoundWave.h"
#include "FMODEvent.h"
#include "LevelData.generated.h"

USTRUCT(BlueprintType)
struct FAIDataPick
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Chance; // If the Chance fields from all SpawnAI add up to 100, this gives you a percent chance for this AI to spawn.

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString AILookupTag;
};

USTRUCT(BlueprintType)
struct FTranscript
{
	GENERATED_USTRUCT_BODY()

	// The point in seconds the speaker begins speaking
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float AudioTimestamp;

	// Time in seconds the entry lasts, can be used to parse out text with audio
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float Duration;

	// The name of the character speaking
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText SpeakerName;

	// Transcript of current line
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (multiline = true))
		FText Transcription;
};

USTRUCT(BlueprintType)
struct FDynamicLevelDataPick
{
	GENERATED_USTRUCT_BODY()

	// The label that is associated with the dynamic level data.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName Label;

	// If the chance fields from all fields add up to 100, this gives you a percent chance for this dynamic level data to spawn.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Chance;
};

// A level has multiple Spawn Rosters, each one being spawned at map start.
// AI rosters are for suspects and civilians/hostages
USTRUCT(BlueprintType)
struct FAIRoster
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MinimumSpawn;	// the minimum amount that will spawn from this roster

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaximumSpawn;	// the maximum amount that will spawn from this roster

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SpawnGroup;	// this roster will spawn AI at the spawners with the associated SpawnGroup

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FAIDataPick> SpawnAI; // the kind of AI that can spawn from this roster

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bAffectedByNegotiator;	// if true, Negotiator reduces the spawn count by 10%
};

// Trap rosters are for traps.
USTRUCT(BlueprintType)
struct FTrapRoster
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MinimumSpawn;	// the minimum amount that will spawn from this roster

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaximumSpawn;	// the maximum amount that will spawn from this roster

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SpawnGroup;	// this roster will spawn AI at the spawners with the associated SpawnGroup
};

// Dynamic Level rosters are used for anything in the world that we spawn dynamically.
USTRUCT(BlueprintType)
struct FDynamicLevelRoster
{
	GENERATED_USTRUCT_BODY()

	// The chance (0..1) of this roster spawning to begin with.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float OverallSpawnPercent;

	// The minimum number of picks that are chosen to dynamically spawn.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MinimumPicks;

	// The maximum number of picks that are chosen to dynamically spawn.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MaximumPicks;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FDynamicLevelDataPick> SpawnLevel;
};

USTRUCT(BlueprintType)
struct FMissionAudio
{
	GENERATED_USTRUCT_BODY()

	// The name of this audio option
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Audio)
	FText AudioName;

	// The description of this audio option
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Audio)
	FText AudioDescription;

	// Internal string identifier of this audio option
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Audio)
	FString AudioInternalName;

	// The sound associated with this audio option.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Audio)
	UFMODEvent* SoundFile;

	// Use to have accompanying transcript feed when playing
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (multiline = true), Category = Audio)
	FText AudioTranscript;
};

USTRUCT(BlueprintType)
struct FCriminalRecord
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FString Date;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText Crime;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FString CriminalCode;
};

USTRUCT(BlueprintType)
struct FCharacterBio
{
	GENERATED_USTRUCT_BODY()
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bio)
		FString Name;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bio)
		FString IdNumber;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bio)
		TArray<FText> Aliases;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bio, meta = (multiline = true))
		FText Bio;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bio)
		TSoftObjectPtr<UTexture2D> ProfileImage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bio)
		FText Sex;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bio)
		FText Build;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bio)
		FText Height;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bio)
		FText Weight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bio)
		FText Hair;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bio)
		FText Eyes;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bio)
		FText DateOfBirth;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bio)
		FText Age;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Bio)
		TArray<FCriminalRecord> CriminalRecord;

	FCharacterBio()
	{
		Name = "TEMP (Joe Bloggs)";
		Bio = FText::FromString("TEMP (Fill in the Bio Description)");
		Sex = FText::FromString("TEMP (M)");
		Build = FText::FromString("TEMP (Medium)");
		Height = FText::FromString("TEMP (5'11\")");
		Weight = FText::FromString("TEMP (130)");
		Hair = FText::FromString("TEMP (Black)");
		Eyes = FText::FromString("TEMP (Brown)");
		DateOfBirth = FText::FromString("TEMP (09/01/2000)");
	}
};

UENUM(BlueprintType)
enum class EPersonnel : uint8
{
	PERS_None,
	PERS_TruckDriver,
	PERS_NoisemakerOperator,
	PERS_VentilationExpert,
	PERS_Spotter,
	PERS_Marksman,
	PERS_Sniper,
	PERS_FloodlightOperator,
	PERS_PowerCrew,
	PERS_Negotiator,
};

// Each Personnel put in the map (except Power Crew and Negotiator) is assigned a Map Point. Each map point can have one personnel on them at a time. There can be up to 32 map points on a map.
USTRUCT(BlueprintType)
struct FPersonnelMapPoint
{
	GENERATED_USTRUCT_BODY()

	// The volume associated with this map point.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Personnel)
	FName VolumeLabel;

	// The actor associated with this map point. (Noisemaker, Floodlight, Spotter/Marksman/Sniper, CS gas grenade)
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Personnel)
	FName ActorLabel;

	// The name of this map point as it appears in the GUI
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Personnel)
	FText MapPointName;

	// The description of this map point as it appears in the GUI
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Personnel)
	FText MapPointDescription;

	// The floor to switch to when this map element is selected.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Personnel)
	int32 FloorNum;

	// The shift that occurs when this map element is selected.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Personnel)
	FVector2D MapShiftPosition;

	// The zoom level that occurs when this map element is selected.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Personnel)
	float MapShiftZoom = 1.0f;

	// The coordinates to place the map zone.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Personnel)
	FVector2D MapCoordinates;

	// Whether to draw this as a single point, or as a zone.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Personnel)
	bool bMapZone;

	// The size of the zone, if there is one.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Personnel)
	FVector2D MapZoneSize;
};

// This is the Personnel that we have available on the map. There can be up to 32 Personnel on a single map.
USTRUCT(BlueprintType)
struct FPersonnelEntry
{
	GENERATED_USTRUCT_BODY()

	// What type of Personnel it is
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Personnel)
	EPersonnel PersonnelType;

	// Which of the map entries (on the map data, indexed from 0) we are allowed to put the personnel on.
	// Though this is BlueprintReadWrite, DO NOT modify the values from a Blueprint.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Personnel)
	TArray<int32> AvailableMapPoints;

	// How many points it costs to activate this
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Personnel)
	int32 PtsCost;

	// The texture to use for this personnel entry
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Personnel)
	TSoftObjectPtr<UTexture2D> PersonnelTexture;
};

USTRUCT(BlueprintType)
struct FTimelineEvent
{
	GENERATED_USTRUCT_BODY()

	// Shown below the main timeline display as the title
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline)
	FText EventTitle;

	// The name of the buttons to click on the timeline selection
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline)
	//FText EventShortTitle;

	// The fictional time of day the event took place
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline)
	FText EventTime;

	// The description for the event
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline)
	FText EventDescription;

	// What point in seconds the event description begins in the audio.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline)
	float EventTimestamp;

	// Duration in seconds the event description lasts in the audo. End marks the event.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline)
	float EventDuration;
};

USTRUCT(BlueprintType)
struct FMissionTimeline
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Timeline, meta = (multiline = true))
		TArray<FTimelineEvent> EventsList;
};

USTRUCT(BlueprintType)
struct FLevelDeployableData
{
	GENERATED_USTRUCT_BODY()

		// The data associated with this deployable.
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		class UDeployableData* DeployableData;

	// How many points this deployable costs to use.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int32 PtsCost;
};

USTRUCT(BlueprintType)
struct FLevelDeployableDepot
{
	GENERATED_USTRUCT_BODY()

		// The label on the depot that will be used to spawn.
		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FName DepotLabel;

	// How much the depot costs
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int32 DepotCost;

	// The short name of the depot, used on the button
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText DepotShortName;

	// The long name of the depot, used on the title
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText DepotLongName;

	// The description of the depot
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FText DepotDescription;

	// The floor number for the map display
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int32 MapFloorNum;

	// The coordinates on the map display
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FVector2D MapCoordinates;

	// The shift amount for the preview
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FVector2D MapShiftAmount;

	// The zoom level for the preview
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float MapZoomLevel = 1.0f;
};

USTRUCT(BlueprintType)
struct FEntryPoint
{
	GENERATED_BODY()

	// Name for this spawn point to use in planning menus
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Name;

	// The tag used for this start on the destination level
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName Tag;
	
	// Description for this spawn point to use in planning menus
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;
	
	// Image for this spawn point to use in planning menus
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Image;

	// Entry point coordinates on the planning map
	FVector2D Position;

	// Game modes to exclude this entry point from
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<ECOOPMode> ExcludedGameModes;

	FEntryPoint()
	{
		Name = FText::FromString("Entry Point");
		Description = FText::FromString("No description provided");
	}
};

USTRUCT(BlueprintType)
struct FSpawnPoints
{
	GENERATED_USTRUCT_BODY()
	
	// The name of the spawn point as it appears in the Planning interface.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Planning)
		FText SpawnPointName;

	// The image of the spawn point when it is selected in the Planning interface.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Planning)
		TSoftObjectPtr<UTexture2D> SpawnImage;

	// The description of the spawn point when it is selected in the Planning interface.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Planning, meta = (multiline = true))
		FText SpawnDescription;

	// The recommended equipment/deployables, keep it to a handful
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Planning)
		TArray<FLevelDeployableData> RecommendedDeployables;

	// The floor that the spawn point is on (0 = first floor in level data, 1 = second floor in level data, ...)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Planning)
		int32 PlanningFloorNum;

	// How much to zoom the map camera in to when we have highlighted this choice.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Planning)
		float PlanningZoomLevel = 1.0f;

	// Where to pan the camera to (check the SHIFT value) when we have highlighted this choice.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Planning)
		FVector2D PlanningShift;

	// The coordinates to place the spawn, on the planning map.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Planning)
		FVector2D PlanningCoordinate;

	// The number of points that this spawn point costs to use.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Spawning)
		int32 PtsCost;

	// Whether this spawn point is used or not. Won't show up in the menus if true.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Spawning)
		bool bSpawnDisabled;

	// The label assigned to the spawn point objects to make 'em work. Was something like SPAWN_1, SPAWN_2, etc. before I changed this --eez
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Spawning)
	FName SpawnLabel;

	FSpawnPoints()
	{
		SpawnPointName = FText::FromString("NORTH CARPARK");
		SpawnDescription = FText::FromString("NORTH ENTRANCE VIA THE CARPARK.\r\n\r\nLEADS TO BACK OF RESTAURANT.BE SURE TO NOT SET OFF ANY ALARMS!");
		PtsCost = 1;
	}

};

USTRUCT(BlueprintType)
struct FLevelFloor
{
	GENERATED_BODY()

	// Name of this floor to use in planning menus
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Name;

	// This floor's number in planning menus
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Number;
	
	// Texture to use on the map when this floor is selected in planning menus
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> FloorMap;
	
	// Entry points for this floor, if applicable
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TArray<FEntryPoint> EntryPoints;
	
	// Game modes to exclude this floor from
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<ECOOPMode> ExcludedGameModes;
};

USTRUCT(BlueprintType)
struct FLevelFloorData
{
	GENERATED_USTRUCT_BODY()

	// The name of this floor in longform, as it appears on top of the map display
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Floor)
	FText FloorFullName;

	// The name of this floor in short form, as it appears on the floor select buttons
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Floor)
	FText FloorShortName;

	// The texture to use as this floor's layout. Uses alpha.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Floor)
	TSoftObjectPtr<UTexture2D> FloorLayout;

	// The material to apply to the map object
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Floor)
	TSoftObjectPtr<UMaterialInterface> FloorplanMaterial;
};

USTRUCT(BlueprintType)
struct FMissionDoc
{
	GENERATED_BODY()

	// The title of this document
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Document)
	FText DocumentTitle;

	// The text of this document
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Document, meta = (multiline = true))
	FText DocumentText;

	// The text to display on the button for this document
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Document)
	FText DocumentButtonText;

	// The description text to display on the button for this document
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Document)
	FText DocumentButtonDescriptionText;
};

USTRUCT(BlueprintType)
struct FMissionPhoto
{
	GENERATED_USTRUCT_BODY()

	// The title of this photo
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Photo)
	FText PhotoTitle;

	// The photo texture
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Photo)
	TSoftObjectPtr<UTexture2D> Photo;

	// The photo caption
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Photo, meta = (multiline = true))
	FText PhotoCaption;
};

USTRUCT(BlueprintType)
struct FLoadingScreenAnimated
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Foreground;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Background;

	FLoadingScreenAnimated()
	{
		Foreground = nullptr;
		Background = nullptr;
	}
	
};

USTRUCT(BlueprintType)
struct FMapLayout
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FVector2D MapOrigin = FVector2D::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	float MapSize = 5000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TSoftObjectPtr<UTexture2D> MapOverviewTexture = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TMap<FString,TSoftObjectPtr<UTexture2D>> MapCells;
};

USTRUCT(BlueprintType)
struct FUnlockRequirements
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	FString MapURL = "";
	
	UPROPERTY(EditAnywhere,  meta = (ClampMin = "0", ClampMax = "1", UIMin = "1", UIMax = "1"))
	float Score = 0.0f;

	FUnlockRequirements()
	{
		
	};
};

USTRUCT(BlueprintType)
struct FNVGPostProcessSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> DirtMaskTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EDepthOfFieldMethod> DepthOfFieldMethod = DOFM_CircleDOF;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BloomIntensity = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BloomThreshold = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EAutoExposureMethod> AutoExposureMethod = AEM_Histogram;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BloomDirtMaskIntensity = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DepthOfFieldFstop = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LensFlareIntensity = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LensFlareBokehSize = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AmbientOcclusionIntensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float IndirectLightingIntensity = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FilmSlope = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FilmToe = 0.2;
};

/** Structure to store the lookup of GameObjects for use in a UDataTable */
USTRUCT(BlueprintType)
struct FLoadAdditionalLevels
{
	GENERATED_USTRUCT_BODY();
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<FString> SubLevelNames;

	FLoadAdditionalLevels()
	{
		SubLevelNames = {};
	}
};

/** Structure to store the lookup of GameObjects for use in a UDataTable */
USTRUCT(BlueprintType)
struct FLevelDataLookupTable : public FTableRowBase
{
	GENERATED_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Required")
	FText FriendlyLevelName;

	// Used on the overview screen
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Required")
	FText LevelNickname;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Required")
    TSoftObjectPtr<UTexture2D> LevelPicture;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Required")
	FText LevelDesignation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Required")
	FText TimeOfDayText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Required")
	FText Description;
	
	// Tag used in automatic completion tag generation (i.e. <tag>_grade_s, <tag>_grade_b)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Required")
	FName ProgressionTagPrefix;
	
	// Map to show in the mission selection screen
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Required")
	TSoftObjectPtr<UWorld> MissionSelectMap;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map", meta=(ArrayClamp="Floors"))
	int32 DefaultFloorIndex = 0;

	// List of floors for this level, including their associated maps and info text
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TArray<FLevelFloor> Floors;

	// Entry points available for this level not associated with a specific floor
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TArray<FEntryPoint> EntryPoints;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tablet")
	float TabletScreenBrightness = 3.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tablet")
	uint8 bUseFixedExposureWhenViewingTablet : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tablet", meta = (EditCondition = "bUseFixedExposureWhenViewingTablet"))
	float MinEV100 = 6.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tablet", meta = (EditCondition = "bUseFixedExposureWhenViewingTablet"))
	float MaxEV100 = 9.0f;
	
	// Specify the sublevel that we should load for each level
	UPROPERTY(EditAnywhere, BlueprintReadWrite , Category = "COOP Modes")
	TMap<ECOOPMode, FString> COOPModesLevelMap;

	// Specify any additional levels that should be loaded (added seperately for compatability reasons)
	UPROPERTY(EditAnywhere, BlueprintReadWrite , Category = "COOP Modes")
	TMap<ECOOPMode, FLoadAdditionalLevels> COOPModesLevelAdditionalMaps;
	
	// map url in string, score as float
	// all must be completed to unlock this level
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite , Category = "Legacy")
	FUnlockRequirements UnlockRequirements;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Legacy")
	FMapLayout MapLayout;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Legacy")
	FLoadingScreenAnimated LoadingScreen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	TArray<TSoftClassPtr<class AReadyOrNotGameMode>> SupportedGameModes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	FText Author;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	FText AuthorContact;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	FText RecommendedPlayerCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	bool bIsTestLevel = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	bool bVisible = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NVG")
	FNVGPostProcessSettings NVG_PostProcessOverride;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Sequencer)
	FVector MVPSequenceLocation = FVector::ZeroVector;

	// Mission objectives for this mission
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Data)
	TArray<TSoftClassPtr<class AObjective>> Objectives;

	// Challenges that are available on this mission
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Challenges)
	TArray<TSoftClassPtr<class UChallenge>> Challenges;

	// Used in the Briefing page, underneath the mission name.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Level)
	FText LevelLocationText;

	// Used in the planning page, next to the OVERVIEW text.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Level)
	FText LevelMonthNum;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Level)
	FText LevelDayNum;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Level)
	FText LevelYearNum;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Briefing", meta = (multiline = true))
	FMissionAudio TocBriefingAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Briefing")
	TArray<FMissionAudio> MissionAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Briefing")
	TArray<FMissionDoc> Documents;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Briefing")
	TArray<FMissionPhoto> Photos;

	// Overrides the TOC voiceover for this level
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Briefing")
	FName MissionStartTocVoiceLine;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bios", meta = (multiline = true))
	TArray<FCharacterBio> SuspectsBios;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bios", meta = (multiline = true))
	TArray<FCharacterBio> CiviliansBios;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline")
	FMissionTimeline MissionTimeline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legacy")
	int32 BaseSquadPts = 10;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legacy")
	TArray<FLevelFloorData> MapFloors;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legacy")
	FSpawnPoints Spawn_1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legacy")
	FSpawnPoints Spawn_2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legacy")
	FSpawnPoints Spawn_3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legacy")
	FSpawnPoints Spawn_4;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legacy")
	TArray<FAIRoster> AISpawnRosters;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legacy")
	TArray<FDynamicLevelRoster> DynamicLevelSpawnRosters;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legacy")
	TArray<FTrapRoster> TrapRosters;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legacy")
	TArray<FPersonnelMapPoint> AllPersonnelMapPoints;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legacy")
	TArray<FPersonnelEntry> AllPersonnel;

	// The list of deployables that can spawn in this level. You can have up to 32 deployables on any mission.
	// The deployable number in the array (0..31) corresponds to the Deployable Number field in the Deployable component. --eez
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legacy")
	TArray<FLevelDeployableData> Deployables;

	// The different deployable depots that can spawn on this level. The depots that are placed in the world must have the proper labels.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Legacy")
	TArray<FLevelDeployableDepot> DeployableDepots;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prop Hunt")
	TArray<UStaticMesh*> PropHuntMeshes;
	
	FORCEINLINE bool operator==(const FLevelDataLookupTable& Other) const
	{
		return FriendlyLevelName.ToString() == Other.FriendlyLevelName.ToString()
			&& Description.ToString() == Other.Description.ToString()
			&& LevelPicture == Other.LevelPicture;
	}
};

/**
 * 
 */
UCLASS(BlueprintType)
class READYORNOT_API UModLevelData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName LevelName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bShowInMissionSelect = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ShowOnlyInnerProperties))
	FLevelDataLookupTable Data;
};

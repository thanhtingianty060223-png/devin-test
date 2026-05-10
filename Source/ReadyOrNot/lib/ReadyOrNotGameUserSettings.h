// Copyright Void Interactive, 2024

#pragma once

#include "GameFramework/GameUserSettings.h"
#include "Enums.h"
#include "Actors/Gameplay/ReadyOrNotPlayerState.h"
#include "ReadyOrNotGameUserSettings.generated.h"

UENUM(BlueprintType)
enum class ECommandInterfaceType : uint8
{
	CI_GraphicCommandInterface,
	CI_ClassicCommandInterface
};

UENUM(BlueprintType)
enum class EGrenadeThrowSettingType : uint8
{
	GUT_QuickGrenadeThrow,
	GUT_ClassicGrenadeThrow,
};

UENUM(BlueprintType)
enum class EShotgunReloadType : uint8
{
	SRT_SingleLoad,	// One tap = load one shotgun shell
	SRT_MultiLoad,	// One tap = load the entire shotgun, tap again to cancel
};

UENUM(BlueprintType)
enum class EEmptyMagReloadType : uint8
{
	RegularReload,
    FastReload,
};

UENUM(BlueprintType)
enum class EScoreReadoutMode : uint8
{
	AllScores,
	OnlyPositive,
	OnlyNegative,
	Disabled
};

USTRUCT(BlueprintType)
struct FMirrorReflectionSettings
{
	GENERATED_BODY()

	UPROPERTY(config)
	float MirrorResolutionScale = 0.8f;
	UPROPERTY(config)
	uint8 bShowAntiAliasing : 1;
	UPROPERTY(config)
	uint8 bShowDecals : 1;
	UPROPERTY(config)
	uint8 bShowDynamicShadows : 1;
};

UENUM(BlueprintType)
enum class ENVGStyle : uint8
{
	GreenPhosphor,
	WhitePhosphor
};

UENUM(BlueprintType)
enum class ESubtitlesSize : uint8
{
	Small,
	Normal,
	Large,
	ExtraLarge
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UReadyOrNotGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE(FOnSettingsSaved);
	FOnSettingsSaved OnSettingsSaved;

	virtual void SaveSettings() override;
	
	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;
	void ApplyCommandLineOverrides();

	UFUNCTION(BlueprintCallable)
	static void ResetKeybinds();

	UFUNCTION(BlueprintCallable)
	void ResetGamepadControlsSettings();
	
	UPROPERTY(config)
	float MasterSoundVolume = 1.0f;

	UPROPERTY(config)
	float MusicSoundVolume = 1.0f;

	UPROPERTY(config)
	float UISoundVolume = 1.0f;

	UPROPERTY(config)
	float SFXSoundVolume = 1.0f;

	UPROPERTY(config)
	float VOIPVolume = 1.0f;

	UPROPERTY(config)
	EVoiceType DefaultVOIPChannel = EVoiceType::VT_Team;

	UPROPERTY(config)
	bool bHitmarkerSfxEnabled = true;

	UPROPERTY(config)
	float MouseSensitivity = 0.25f;

	UPROPERTY(config)
	float FreelookSensitivity = 1.0f;

	UPROPERTY(config)
	float GamepadLookSensitivity = 0.25f;

	UPROPERTY(config)
	float GamepadAimSensitivity = 0.25f;

	UPROPERTY(config)
	FString AimAssistIntensity = "off";

	UPROPERTY(config)
	FString TargetLocale = "en";

	UPROPERTY(config)
	float FieldofView = 90.0f;

	UPROPERTY(config)
	float LadderRollSensitivity = 2.0f;

	UPROPERTY(config)
	bool bToggleADS = false;

	UPROPERTY(config)
	bool bHoldCrouch = false;

	UPROPERTY(config)
	bool bTogglePS5Gyro = false;

	UPROPERTY(config)
	bool bUsingAlternateControls = false;

	UPROPERTY(config)
	int32 MaxShellsInWorld = 100.0f;

	UPROPERTY(config)
	float MaxShellLifeTime = 300.0f;

	UPROPERTY(config)
	bool bUseMeshPainting = true;

	UPROPERTY(config)
	FString LastConnectedServerIP = "";

	UPROPERTY(config)
	float IconScale = 1.0f;

	UPROPERTY(config)
	float QuickThrowScale = 1.0f;

	UPROPERTY(config)
	bool bWorldDecalsEnabled = true;

	UPROPERTY(config)
	float WorldDecalScreenFadeSize = 0.99f;

	UPROPERTY(config)
	float WorldDecalDensity = 0.9f;
	
	UPROPERTY(config)
	int32 FireModeDisplayOption = 1;

	UPROPERTY(config)
	int32 TeamViewFPS = 60.0f;
	
	UPROPERTY(config)
	bool bTeamViewFPSEnabled = false;
	
	UPROPERTY(config)
	bool bShowFPS = 0;

	UPROPERTY(config)
	bool bShowHUD = 1;

	UPROPERTY(config)
	bool bShowCompass = 0;

	UPROPERTY(config)
	bool bShowWeaponHUD = 1;

	UPROPERTY(config)
	bool bShowMagazineHUD = 1;

	UPROPERTY(config)
	bool bShowChat = 1;

	UPROPERTY(config)
	bool bEnableHUDSwaying = 1;
	
	UPROPERTY(config)
	bool bShowHotkeyHints = 1;
	
	UPROPERTY(config)
	bool bShowHealthIcons = 1;
	
	UPROPERTY(config)
	bool bShowCommandContextHint = 1;
	
	UPROPERTY(config)
	bool bZoomADS = 1;
	
	UPROPERTY(config)
	bool bSendMapStatistics = false;
	
	UPROPERTY(config)
	EScoreReadoutMode ScoreReadoutMode = EScoreReadoutMode::AllScores;
	
	UPROPERTY(config)
	ESwatCommand DefaultCommand = ESwatCommand::SC_FallIn;
	
	//UPROPERTY(config)
	//ESwatCommand DefaultHumanCommand = ESwatCommand::SC_Focus;

	UPROPERTY(config)
	ESwatCommand DefaultDoorUnknownCommand = ESwatCommand::SC_StackUp;
	
	UPROPERTY(config)
	ESwatCommand DefaultDoorOpenCommand = ESwatCommand::SC_MoveAndClear;
	
	UPROPERTY(config)
	ESwatCommand DefaultDoorLockedCommand = ESwatCommand::SC_PickLock;
	
	UPROPERTY(config)
	ESwatCommand DefaultDoorUnlockedCommand = ESwatCommand::SC_OpenAndClear;
	
	UPROPERTY(config)
	int32 DefaultCommandOption = 0;

	UPROPERTY(Config)
	int32 ViewDistanceQuality = -1;
	UPROPERTY(Config)
	int32 AntiAliasingQuality = -1;
	UPROPERTY(Config)
	int32 ShadowQuality = -1;
	UPROPERTY(Config)
	int32 PostProcessQuality = -1;
	UPROPERTY(Config)
	int32 TextureQuality = -1;
	UPROPERTY(Config)
	int32 EffectsQuality = -1;

	UPROPERTY(Config)
	bool bEnablePerObjectShadows = true;
	
	//UPROPERTY(config)
	//int32 DefaultHumanCommandOption = 0;
	
	UPROPERTY(config)
	int32 DefaultDoorUnknownCommandOption = 0;
	
	UPROPERTY(config)
	int32 DefaultDoorOpenCommandOption = 3;
	
	UPROPERTY(config)
	int32 DefaultDoorLockedCommandOption = 0;
	
	UPROPERTY(config)
	int32 DefaultDoorUnlockedCommandOption = 0;
	
	UPROPERTY(config)
	bool bCurvedHUD = 0;

	UPROPERTY(config)
	bool b2DReloadIcons = 0;
	
	UPROPERTY(config)
	bool bShowPlayerNamePlates = false;
	
	UPROPERTY(config)
	bool bShowPlayerIcon = false;

	UPROPERTY(config)
	bool bShowTeamStatus = true;
	
	UPROPERTY(config)
	bool bMirrorReflectionEnabled = true;
	
	UPROPERTY(config)
	bool bMirrorInLobbyOnly = true;
	
	UPROPERTY(config)
	float MirrorFPS = 60.0f;
	
	UPROPERTY(config)
	float PiPFPS = 60.0f;
	
	UPROPERTY(config)
	bool bPiPFPSEnabled = false;

	UPROPERTY(config)
	bool bDepthOfField = false;

	UPROPERTY(config)
	bool bMotionBlur = true;

	UPROPERTY(config)
	float MotionBlurStrength = 0.24f;
	
	UPROPERTY(config)
	float PiPResolutionScale = 1.0f;

	UPROPERTY(config)
	float SafeZoneX = 0.3f;

	UPROPERTY(config)
	float SafeZoneY = 0.2f;

	UPROPERTY(config)
	int32 GraphicsPresetIndex = 0;

	UPROPERTY(config)
	bool bShowButtonPrompts = true;

	UPROPERTY(config)
	bool bInvertMousePitch = false;

	UPROPERTY(config)
	bool bInvertMouseYaw = false;

	UPROPERTY(config)
	bool bInvertGamepadHorizontal = false;

	UPROPERTY(config)
	bool bInvertGamepadVertical = false;

	UPROPERTY(config)
	bool bExperimentalFeatures = true;	// Remember to turn this off once alpha rolls around!

	UPROPERTY(config)
	bool bShowControlsOnScreen = true;

	UPROPERTY(config)
	bool bShowHesitationBar = true;

	UPROPERTY(config)
	bool bRayTracingEnabled = false;
	
	UPROPERTY(config)
	bool bRayTracingReflectionsEnabled = false;

	UPROPERTY(config)
	bool bRayTracingShadowsEnabled = false;

	UPROPERTY(config)
	bool bRayTracingAmbientOcclusionEnabled = false;

	// Removed from config and disabled
	bool bRTXGlobalIllumination = false;
	bool bRTXTranslucency = false;

	UPROPERTY(config)
	int32 DlssQualitySetting = 0;

	UPROPERTY(config)
	int32 ExperimentalFrameGenerationSetting = 0;
	
	UPROPERTY(config)
	int32 FSRQualitySetting = 0;

	UPROPERTY(config)
	float MicInputGain = 1.0f;

	UPROPERTY(config)
	FString InputAudioDevice;

	UPROPERTY(config)
	bool bFrameLimitEnabled = false;
	
	UPROPERTY(config)
	EItemSelectionInterfaceType ItemSelectionInterface = EItemSelectionInterfaceType::Panel;

	UPROPERTY(config)
	EGrenadeThrowSettingType GrenadeThrowType = EGrenadeThrowSettingType::GUT_QuickGrenadeThrow;

	UPROPERTY(config)
	EShotgunReloadType ShotgunLoadType = EShotgunReloadType::SRT_MultiLoad;

	UPROPERTY(config)
	EEmptyMagReloadType EmptyMagReloadType = EEmptyMagReloadType::FastReload;
	
	UPROPERTY(config)
	EOptiwandViewMode OptiwandViewMode = EOptiwandViewMode::Fullscreen;
	
	UPROPERTY(config)
	FString DMOAddress;

	UPROPERTY(config)
	FString DMOGameMode = "tow";

	UPROPERTY(config)
	ETeamType DMOTeamType;
	
	UPROPERTY(config)
	FMirrorReflectionSettings MirrorReflectionSettings;
	
	UPROPERTY(config)
	ENVGStyle NVGStyle = ENVGStyle::GreenPhosphor;

	UPROPERTY(config)
	bool bUseHighReadyStyle = false;
	
	UPROPERTY(config)
	bool bBounceLightEnabled = true;

	UPROPERTY(config)
	bool bFlashlightShadowsEnabled = true;
	
	UPROPERTY(config)
	bool bServerSideChecksum = true;

	UPROPERTY(config)
	uint8 ReflexMode = 0;

	UPROPERTY(config)
	bool bReflexGameToRenderLatencyInMSEnabled = false;

	UPROPERTY(config)
	bool bReflexGameLatencyInMSEnabled = false;
	
	UPROPERTY(config)
	bool bReflexRenderLatencyInMSEnabled = false;

	UPROPERTY(config)
	bool bReflexFlashIndicatorEnabled = false;

	UPROPERTY(config)
	EColorVisionDeficiency ColorVisionDeficiency = EColorVisionDeficiency::NormalVision;

	UPROPERTY(config)
	float ColorVisionDeficiencyStrength = 1.0f;

	UPROPERTY(config)
	bool bHighlightWeapons = false;

	UPROPERTY(config)
	bool bWorldSpaceActionPrompts = true;

	UPROPERTY(config)
	float InterfaceAspectRatio = -1.0f;
	
	/* Subtitles */
	
	UPROPERTY(config)
	bool bEnableSubtitles = false;

	UPROPERTY(config)
	ESubtitlesSize SubtitlesSize = ESubtitlesSize::Normal;

	UPROPERTY(config)
	FString SubtitlesLocale = "en";

	UPROPERTY(config)
	float SubtitlesBackgroundOpacity = 0.0f;
	
	UPROPERTY(config)
	float SubtitlesSpeed = 1.0f;

	/* Replay */
	bool bReplayEnabled = false;
};

// Copyright Void Interactive, 2024

#pragma once

#include "Actors/MusicSequencerBase.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DataSingleton.h"
#include "HUD/WidgetsData.h"
#include "Data/LevelData.h"
#include "Data/MusicData.h"
#include "ReadyOrNotGameState.h"
#include "ReadyOrNotGameUserSettings.h"
#include "Data/WidgetDataTable.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/SaveGame.h"
#include "BpGameplayHelperLib.generated.h"

USTRUCT(BlueprintType)
struct FSavedTransforms
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FTransform SavedTransform;
};

UENUM()
enum class EStructureCastPathway : uint8
{
	CastSuccess,
	CastFailed
};

UCLASS()
class READYORNOT_API UBpGameplayHelperLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Game Instance")
	static UReadyOrNotGameInstance* GetRONGameInstance();
	UFUNCTION(BlueprintPure, Category = "World")
	static UWorld* GetWorldStatic();
	UFUNCTION(BlueprintPure, Category = "Data")
	static UDataSingleton* GetRoNData();

	UFUNCTION(BlueprintPure, Category = "Data")
	static UCampaignData* GetCampaignData();
	
	UFUNCTION(BlueprintPure, Category="Ready or Not", meta=(WorldContext="WorldContextObject"))
	static bool IsLeadPlayer(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category = "Widget")
	static FWidgetLookupData GetWidgetDataFromLookupData(FString WidgetName, bool bWarnIfMissing = true);
	
	UFUNCTION(BlueprintPure, Category = "Widget")
	static UWidgetsData* GetWidgetData();

	UFUNCTION(BlueprintPure, Category = "Widget")
	static class UHumanCharacterHUD_V2* GetHUDWidget();
	
	UFUNCTION(BlueprintPure, Category = "Widget")
	static bool HasWidgetInViewport(FString WidgetName);

	UFUNCTION(BlueprintCallable, Category = "Widget")
	static void RemoveWidgetFromViewport(FString WidgetName);

	static UDataTable* GetAnimationDataTable();

	static bool IsVoiceOverSuspended(UObject* WorldContextObject);
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	static void EnableInteractionFor(AActor* InInteractableActor, APlayerCharacter* InPlayerCharacter);
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	static void DisableInteractionFor(AActor* InInteractableActor, APlayerCharacter* InPlayerCharacter);
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	static void EnableInteractionForController(AActor* InInteractableActor, APlayerController* InPlayerCharacter);
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	static void DisableInteractionForController(AActor* InInteractableActor, APlayerController* InPlayerCharacter);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	static void EnableInteractionCompForController(UInteractableComponent* InteractableComponent, APlayerController* InPlayerController);
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	static void DisableInteractionCompForController(UInteractableComponent* InteractableComponent, APlayerController* InPlayerController);
	
	UFUNCTION(BlueprintCallable, Category = "Widget")
    static TArray<UUserWidget*> GetWidgetsFromViewport(FString WidgetName);

	UFUNCTION(BlueprintCallable, Category = "Widget")
	static bool IsWidgetOfClassInViewport(UObject* WorldContextObject, TSubclassOf<UUserWidget> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "Widget")
    static UUserWidget* GetFirstWidgetFromViewport(FString WidgetName);

	UFUNCTION(BlueprintPure, Category = "Maps & Modes")
	static TArray<FLevelDataLookupTable> GetLevels();

	UFUNCTION(BlueprintPure, Category = "Maps & Modes")
	static void GetFriendlyMapAndModeFromName(FString InUrl, FString &OutInternalMapName, FString& OutFriendlyMap, FString& OutFriendlyMode);

	UFUNCTION(BlueprintPure, Category = "Maps & Modes")
	static FString GetFriendlyModeFromECoopMode(const ECOOPMode InCoopMode);

	UFUNCTION(BlueprintPure, Category = "Maps & Modes")
	static ECOOPMode GetCoopModeFromModeName(const FString& InCoopName);
	
	UFUNCTION(BlueprintPure, Category = "Data")
	static FLevelDataLookupTable GetMapDetailsFromName(FString MapName);

	UFUNCTION(BlueprintPure, Category = "Data")
		static FString GetLoadURLFromData(FLevelDataLookupTable Lookup);

	static class AReadyOrNotPlayerController* GetLocalRoNPlayerController(UWorld* World = nullptr);

	static int32 GetNumberOfVisibleSwat(class ACyberneticController* CyberneticController);


	static bool HasMapLoadedInMainMenu(FString MapName, UWorld* World = nullptr);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	static APlayerCharacter* GetFirstAlivePlayerControlledCharacter(UWorld* WorldContext = nullptr);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
		static bool IsFriendly(AReadyOrNotGameState* GameState, ETeamType TeamOne, ETeamType TeamTwo);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	static bool IsFriendlyWithMe(AReadyOrNotGameState* GameState, ETeamType TeamType);
	
	UFUNCTION(BlueprintPure, Category = "Gameplay")
	static bool IsEnemy(ETeamType TeamOne, ETeamType TeamTwo);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
		static bool HasLineOfSight(AActor* Observer, AActor* b);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
		static bool HasLineOfSightExt(AActor* Observer, AActor* b, FHitResult& HitResult);


	UFUNCTION(BlueprintCallable, Category = "MVP Sequence")
		void ShowLoadoutOnMeshes(FSavedLoadout Loadout, USkeletalMeshComponent* BodyMesh, USkeletalMeshComponent* HeadMesh, USkeletalMeshComponent* ArmorMesh, USkeletalMeshComponent* ItemMesh, UStaticMeshComponent* ItemMagMesh);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
		static bool HasLineOfSightLoc(UWorld* worldContext, FVector a, FVector b, TArray<AActor*> ignoredActors, ECollisionChannel CollisionChannel = ECollisionChannel::ECC_Visibility);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
		static float GetDistanceBetweenActors(AActor* Actor1, AActor* Actor2);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
		static float GetDistanceBetweenActors2D(AActor* Actor1, AActor* Actor2);

	UFUNCTION(BlueprintCallable, Category = "Graphics")
		static bool SetUseMeshpainting(bool bUseMeshPainting);

	UFUNCTION(BlueprintPure, BlueprintPure, Category = "Graphics")
		static bool GetUseMeshpainting(bool& bUseMeshPainting);

	UFUNCTION(BlueprintCallable, Category = "Graphics")
		static bool SetShellLifetime(float ShellLifeTime);

	UFUNCTION(BlueprintPure, BlueprintPure, Category = "Graphics")
		static bool GetShellLifetime(float& ShellLifeTime);

	UFUNCTION(BlueprintCallable, Category = "Input")
		static bool SetMouseSensitivity(float MouseSensitivity);

	UFUNCTION(BlueprintPure, Category = "Input")
		static bool GetMouseSensitivity(float& MouseSensitvity);

	UFUNCTION(BlueprintCallable, Category = "Input")
		static bool SetFreelookSensitivity(float Sensitivity);

	UFUNCTION(BlueprintPure, Category = "Input")
		static bool GetFreelookSensitivity(float& Sensitivity);

	UFUNCTION(BlueprintCallable, Category = "Input")
		static bool SetGamepadLookSensitivity(float GamepadLookSensitivity);

	UFUNCTION(BlueprintPure, Category = "Input")
		static bool GetGamepadLookSensitivity(float& GamepadLookSensitivity);

	UFUNCTION(BlueprintCallable, Category = "Input")
		static bool SetGamepadAimSensitivity(float GamepadAimSensitivity);

	UFUNCTION(BlueprintPure, Category = "Input")
		static bool GetGamepadAimSensitivity(float& GamepadAimSensitivity);

	UFUNCTION(BlueprintCallable, Category = "Input")
		static bool SetMouseInverted(bool bInvertVertical, bool bInvertHorizontal);

	UFUNCTION(BlueprintPure, Category = "Input")
		static bool GetMouseInverted(bool& bInvertVertical, bool& bInvertHorizontal);

	UFUNCTION(BlueprintCallable, Category = "Input")
		static bool SetGamepadInverted(bool bInvertVertical, bool bInvertHorizontal);

	UFUNCTION(BlueprintPure, Category = "Input")
		static bool GetGamepadInverted(bool& bInvertVertical, bool& bInvertHorizontal);

	UFUNCTION(BlueprintCallable, Category = "Input|Aim Assist")
	static bool SetGamepadAimAssistIntensity(FString AimAssistIntensity);
	
	UFUNCTION(BlueprintPure, Category = "Input|Aim Assist")
	static bool GetGamepadAimAssistIntensity(FString& AimAssistIntensity);

	UFUNCTION(BlueprintCallable, Category = "Graphics")
		static bool SetFoV(float FOV);

	UFUNCTION(BlueprintPure, Category = "Graphics")
		static bool GetFoV(float& FOV);
	
	UFUNCTION(BlueprintCallable)
    static bool SetPublicLobbyCooldown(int32 Seconds);
	
    UFUNCTION(BlueprintPure)
    static bool IsInPublicLobbyCooldown(float& SecondsRemaining);

	UFUNCTION(BlueprintCallable, Category = "VOIP")
	static bool SetMicInputGain(float MicInputGain);

	UFUNCTION(BlueprintPure, Category = "VOIP")
		static bool GetMicInputGain(float& MicInputGain);

	UFUNCTION(BlueprintCallable)
	static bool SaveSelectedAudioDevice(FString InAudioDevice);

	UFUNCTION(BlueprintPure)
	static bool LoadSelectedAudioDevice(FString& OutAudioDevice);
	
	UFUNCTION(BlueprintCallable, Category = "Graphics")
	static bool SetBounceLightEnabled(bool bBounceLightEnabled);

	UFUNCTION(BlueprintPure, Category = "Graphics")
		static bool GetBounceLightEnabled(bool& bBounceLightEnabled);

	UFUNCTION(BlueprintCallable, Category = "Graphics")
	static bool SetFlashlightShadows(bool bFlashlightShadows);

	UFUNCTION(BlueprintPure, Category = "Graphics")
	static bool GetFlashlightShadows(bool& bFlashLightShadows);
	
	UFUNCTION(BlueprintCallable, Category = "Player")
	static bool SaveLoadout(FSavedLoadout Loadout, FString LoadoutName);

	UFUNCTION(BlueprintPure, Category = "Player")
	static bool LoadLoadout(FSavedLoadout& Loadout, FString LoadoutName);

	UFUNCTION(BlueprintCallable, Category = "Player")
	static void DeleteLoadout(FString LoadoutName);
	
	UFUNCTION(BlueprintPure, Category = "Player")
	static void LoadDefaultLoadout(FSavedLoadout& OutLoadout, FString LoadoutName);

	UFUNCTION(BlueprintCallable, Category = "Player")
	static bool LoadLoadoutAndEquipPlayer(FSavedLoadout& Loadout, AReadyOrNotCharacter* EquipPlayer, FString LoadoutName);

	UFUNCTION(BlueprintCallable, Category = "Player")
	static bool EquipLoadoutOnPlayer(FSavedLoadout Loadout, AReadyOrNotCharacter* EquipPlayer, FLoadoutEquipOptions LoadoutEquipOptions);
	
	// sanitize loadout (don't allow invalid options, no option etc)
	static bool SanitizeLoadout(FSavedLoadout& InLoadout);

	// Sanitize loadout slots, should be called after loadout's armour is set. Returns true if loadout was modified
	static bool SanitizeSlots(FSavedLoadout& InLoadout, int32 MaxSlots);

	static bool RemoveLockedDLC(TSubclassOf<ABaseItem>& Item);

	UFUNCTION(BlueprintCallable)
	static bool IsDLCLocked(TSubclassOf<ABaseItem> Item);

	UFUNCTION(BlueprintPure, Category = "Player")
		static bool GetLoadoutNames(TArray<FString>& LoadoutNames);
	
	UFUNCTION(BlueprintCallable, Category = "Player")
		static void AddDefaultItemsToPlayer(AReadyOrNotCharacter* Player);
	
	UFUNCTION(BlueprintPure, Category = "Strings")
		static FString ConvertFloatToStringMinutes(float Val);

	UFUNCTION(BlueprintPure, Category = "Strings")
    static FString ConvertFloatToStringMinutes_Detail(float Val);
	
	UFUNCTION(BlueprintPure, Category = "Math")
		static FVector2D ConvertSquareVectorToCircle(FVector2D SquareVector);

	UFUNCTION(BlueprintCallable, Category = "Sound")
		static bool SaveMasterVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Sound")
		static bool SaveUIVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Sound")
		static bool SaveSFXVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Sound")
		static bool SaveMusicVolume(float Volume);

	UFUNCTION(BlueprintCallable, Category = "Sound")
		static bool SaveVOIPVolume(float Volume);

	UFUNCTION(BlueprintPure, BlueprintPure, Category = "Sound")
		static bool GetHitmarkerSfxEnabled(bool& bHitmarkerSfxEnabled);

	UFUNCTION(BlueprintCallable, Category = "Sound")
		static bool SaveHitmarkerSfxEnabled(bool bHitmarkerSfxEnabled);

	UFUNCTION(BlueprintCallable, Category = "Performance")
		static bool SaveMaxShellsInWorld(int32 NewMaxShells);

	UFUNCTION(BlueprintPure, Category = "Performance")
		static bool LoadMaxShellsInWorld(int32& MaxShells);

	UFUNCTION(BlueprintPure, Category = "Sound")
		static bool GetVolumes(float& MasterVolume, float& UIVolume, float& SFXVolume, float& MusicVolume, float& VOIPVolume);

	UFUNCTION(BlueprintPure, Category = "Sound")
		static bool GetVoiceType(EVoiceType& OutVoiceType);

	UFUNCTION(BlueprintCallable, Category = "Sound")
		static void SetVoiceType(EVoiceType InVoiceType);
	
	UFUNCTION(BlueprintCallable, Category = "Localization")
		static void ChangeLocalization(FString Target);

	UFUNCTION(BlueprintPure, Category = "Localization")
		static bool GetLocalization(FString& Target);

	UFUNCTION(BlueprintPure, Category = "Loading Screen")
		static bool GetRandomLoadingScreenTip(FText& Tip);

	UFUNCTION(BlueprintPure, Category = "Check")
	static bool IsSupporterOnlyBuild();

	UFUNCTION(BlueprintPure, Category = "Loading Screen")
		static UTexture2D* GetLoadingScreenLevelImage(FString Level);

	UFUNCTION(BlueprintCallable, Category = "Input")
		static bool SaveToggleADS(bool ToggleADS);

	UFUNCTION(BlueprintPure, Category = "Input")
		static bool LoadToggleADS(bool& ToggleADS);

	UFUNCTION(BlueprintCallable, Category = "Input")
		static bool SaveHoldCrouch(bool HoldCrouch);

	UFUNCTION(BlueprintPure, Category = "Input")
		static bool LoadHoldCrouch(bool& HoldCrouch);

	UFUNCTION(BlueprintCallable, Category = "Input")
		static bool SaveTogglePS5Gyro(bool TogglePS5Gyro);

	UFUNCTION(BlueprintPure, Category = "Input")
		static bool LoadTogglePS5Gyro(bool& TogglePS5Gyro);

		UFUNCTION(BlueprintCallable, Category = "Input")
		static bool SaveControlScheme(bool UsingAlternateControls);

		UFUNCTION(BlueprintPure, Category = "Input")
		static bool LoadControlScheme(bool& UsingAlternateControls);

		UFUNCTION(BlueprintCallable, Category = "Checksum")
		static bool SaveServersideChecksum(bool bServerSideChecksumEnabled);

		UFUNCTION(BlueprintPure, Category = "Checksum")
		static bool LoadServersideChecksum(bool& bServerSideChecksumEnabled);

		UFUNCTION(BlueprintCallable, Category = "Performance")
		static bool SaveMirrorResolutionScale(float ResolutionScale);

		UFUNCTION(BlueprintPure, Category = "Performance")
		static bool LoadMirrorResolutionScale(float& ResolutionScale);

		UFUNCTION(BlueprintCallable, Category = "Performance")
		static bool SaveMirrorAntiAliasEnabled(bool bShowAntiAlias);

		UFUNCTION(BlueprintPure, Category = "Performance")
		static bool LoadMirrorAntiAliasEnabled(bool& bShowAntiAlias);

		UFUNCTION(BlueprintCallable, Category = "Performance")
		static bool SaveMirrorDecalsEnabled(bool bShowDecals);

		UFUNCTION(BlueprintPure, Category = "Performance")
		static bool LoadMirrorDecalsEnabled(bool& bShowDecals);

		UFUNCTION(BlueprintCallable, Category = "Performance")
		static bool SaveMirrorDynamicShadowsEnabled(bool bShowDynamicShadows);

		UFUNCTION(BlueprintPure, Category = "Performance")
		static bool LoadMirrorDynamicShadowsEnabled(bool& bShowDynamicShadows);

		UFUNCTION(BlueprintCallable, Category = "Performance")
		static bool SaveMirrorReflectionEnabled(bool bEnabled);

		UFUNCTION(BlueprintPure, Category = "Performance")
		static bool LoadMirrorReflectionEnabled(bool& bEnabled);

		UFUNCTION(BlueprintCallable, Category = "Performance")
		static bool SaveMirrorEnabledOnlyInLobby(bool bEnabled);

		UFUNCTION(BlueprintPure, Category = "Performance")
		static bool LoadMirrorEnabledOnlyInLobby(bool& bEnabled);

		UFUNCTION(BlueprintCallable, Category = "Performance")
		static bool SavePiPFPS(bool bEnabled, float FPS);

		UFUNCTION(BlueprintPure, Category = "Performance")
		static bool LoadPiPFPS(bool& bEnabled, float& FPS);

		UFUNCTION(BlueprintCallable, Category = "Performance")
		static bool SavePiPResolutionScale(float ResolutionScale);

	UFUNCTION(BlueprintPure, Category = "Performance")
		static bool LoadPiPResolutionScale(float& ResolutionScale);
	
	UFUNCTION(BlueprintCallable, Category = "Performance")
	static bool SaveOptiwandViewMode(EOptiwandViewMode OptiwandViewMode);
	
	UFUNCTION(BlueprintPure, Category = "Performance")
	static bool LoadOptiwandViewMode(EOptiwandViewMode& OptiwandViewMode);

	UFUNCTION(BlueprintPure)
		static bool IsDMOBuild();

	UFUNCTION(BlueprintPure)
	static bool IsDMOPVPOnly();

	UFUNCTION(BlueprintPure)
	static bool IsDMOMatchMake();

	UFUNCTION(BlueprintPure)
	static bool IsRTXDMOBuild();

	UFUNCTION(BlueprintPure)
	static bool IsPreMissionBriefingBeforeLoadout();

	UFUNCTION(BlueprintCallable)
	static bool SaveKeybinds();

	UFUNCTION(BlueprintPure, Category = "HUD")
    static bool IsShowHUDEnabled();

	UFUNCTION(BlueprintCallable, Category = "HUD")
		static bool SaveShowHUD(bool bShowHUD = false);

	UFUNCTION(BlueprintPure, Category = "HUD")
		static bool LoadShowHUD(bool& bShowHud);

	UFUNCTION(BlueprintCallable, Category = "HUD")
		static bool SaveCurvedHUD (bool bCurvedHUD = true);

	UFUNCTION(BlueprintPure, Category = "HUD")
		static bool LoadCurvedHUD(bool& bCurvedHUD);

	UFUNCTION(BlueprintCallable, Category = "HUD")
		static bool SaveShowCompass(bool bShowCompass = false);

	UFUNCTION(BlueprintPure, Category = "HUD")
		static bool LoadShowCompass(bool& bShowCompass);

	UFUNCTION(BlueprintCallable, Category = "HUD")
		static bool SaveShowWeaponHUD(bool ShowWeaponHUD);

	UFUNCTION(BlueprintPure, Category = "HUD")
		static bool LoadShowWeaponHUD(bool& ShowWeaponHUD);

	UFUNCTION(BlueprintCallable, Category = "HUD")
		static bool SaveShowMagazineHUD(bool bShowMagazineHUD = false);

	UFUNCTION(BlueprintPure, Category = "HUD")
		static bool LoadShowMagazineHUD(bool& bShowMagazineHUD);

	UFUNCTION(BlueprintCallable, Category = "HUD")
		static bool SaveShowChat(bool bShowChat = false);

	UFUNCTION(BlueprintPure, Category = "HUD")
		static bool LoadShowChat(bool& bShowChat);

	UFUNCTION(BlueprintCallable, Category = "HUD")
		static bool SaveSwayHUD(bool bSwayHUD = false);

	UFUNCTION(BlueprintPure, Category = "HUD")
		static bool LoadSwayHUD(bool& bSwayHUD);

	UFUNCTION(BlueprintCallable, Category = "HUD")
		static bool Save2DReload(bool b2DReload = false);

	UFUNCTION(BlueprintPure, Category = "HUD")
		static bool Load2DReload(bool& b2DReload);

	UFUNCTION(BlueprintCallable, Category = "HUD")
		static bool  SaveIconScale(float IconScale = 1.0f);

	UFUNCTION(BlueprintPure, Category = "HUD")
		static bool LoadIconScale(float& IconScale);

	UFUNCTION(BlueprintCallable, Category = "HUD")
		static bool SaveQuickThrowScale(float QuickThrowScale = 1.0f);

	UFUNCTION(BlueprintPure, Category = "HUD")
		static bool LoadQuickThrowScale(float& QuickThrowScale);

	UFUNCTION(BlueprintCallable, Category = "HUD")
		static bool SaveFireModeDisplayOption(int32 FireModeDisplayOption = 0);

	UFUNCTION(BlueprintPure, Category = "HUD")
		static bool LoadFireModeDisplayOption(int32& FireModeDisplayOption);

	UFUNCTION(BlueprintCallable, Category = "HUD")
		static bool SaveShowMultiplayerNames(bool bShowMultiplayerNames = true);

	UFUNCTION(BlueprintPure, Category = "HUD")
		static bool LoadShowMultiplayerNames(bool& bShowMultiplayerNames);

	UFUNCTION(BlueprintCallable, Category = "HUD")
		static bool SaveShowButtonPrompts(bool bShowButtonPrompts = true);

	UFUNCTION(BlueprintPure, Category = "HUD")
		static bool LoadShowButtonPrompts(bool& bShowButtonPrompts);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	static bool SaveHUDSettings(bool bShowHUD = false, bool bCurvedHUD = true, bool bShowCompass = false, bool ShowWeaponHUD = false, bool bShowMagazineHUD = false, bool bShowChat = false, bool bSwayHUD = false, bool b2DReload = false, float IconScale = 1.0f, float QuickThrowScale = 1.0f, int32 FireModeDisplayOption = 0, bool bShowMultiplayerNames = true, bool bShowButtonPrompts = true);

	UFUNCTION(BlueprintPure, Category = "HUD")
	static bool LoadHUDSettings(bool& bShowHud, bool& bCurvedHUD, bool& bShowCompass, bool& bShowWeaponHUD, bool& bShowMagazineHUD, bool& bShowChat, bool& bSwayHUD, bool& b2DReload, float& IconScale, float& QuickThrowScale, int32& FireModeDisplayOption, bool& bShowPlayerNames, bool& bShowButtonPrompts);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	static bool SaveShowHUDSetting(bool bShowHUD = false);
	
	UFUNCTION(BlueprintPure, Category = "HUD")
	static bool LoadShowHUDSetting(bool& bShowHud);
	
	UFUNCTION(BlueprintPure, Category = "HUD")
	static bool LoadShowPlayerNamesSetting(bool& bShowPlayerNames);
	
	UFUNCTION(BlueprintCallable, Category = "HUD")
	static bool SaveShowHesitationBarSetting(bool bShowHesitationBar);
	
	UFUNCTION(BlueprintPure, Category = "HUD")
	static bool LoadShowHesitationBarSetting(bool& bShowHesitationBar);
	
	UFUNCTION(BlueprintCallable, Category = "HUD")
	static bool SaveShowPlayerIconSetting(bool bShowPlayerIcon);

	UFUNCTION(BlueprintPure, Category = "HUD")
	static bool LoadShowPlayerIconSetting(bool& bShowPlayerIcon);
	
	UFUNCTION(BlueprintPure, Category = "HUD")
	static bool LoadSafeZoneSettings(float& SafeZoneX, float& SafeZoneY);

	UFUNCTION(BlueprintCallable, Category = "HUD")
    static bool SaveSafeZoneSettings(float SafeZoneX, float SafeZoneY);

	UFUNCTION(BlueprintPure, Category = "HUD")
	static bool LoadTeamViewFPSSetting(bool& bEnabled, int32& TeamViewFPS);

	UFUNCTION(BlueprintCallable, Category = "HUD")
    static bool SaveTeamViewSetting(bool bEnabled, int32 TeamViewFPS);
	
	UFUNCTION(BlueprintCallable, Category = "HUD")
	static bool SaveScoreReadoutSetting(EScoreReadoutMode InScoreReadoutMode);
	
	UFUNCTION(BlueprintPure, Category = "HUD")
	static bool LoadScoreReadoutSetting(EScoreReadoutMode& OutScoreReadoutMode);
	
	UFUNCTION(BlueprintCallable, Category = "HUD")
	static bool SaveShowCommandContextHintSetting(bool bShowCommandContextHint);
	
	UFUNCTION(BlueprintPure, Category = "HUD")
	static bool LoadShowCommandContextHintSetting(bool& bShowCommandContextHint);
	
	UFUNCTION(BlueprintCallable, Category = "HUD")
	static bool SaveZoomADSSetting(bool bZoomADS);
	
	UFUNCTION(BlueprintPure, Category = "HUD")
	static bool LoadZoomADSSetting(bool& bZoomADS);
	
	UFUNCTION(BlueprintCallable, Category = "HUD")
	static bool SaveHotkeyHintSetting(bool bShowHotkeyHint);
	
	UFUNCTION(BlueprintPure, Category = "HUD")
	static bool LoadHotkeyHintSetting(bool& bShowHotkeyHint);
	
	UFUNCTION(BlueprintCallable, Category = "HUD")
	static bool SaveShowHealthIconSetting(bool bShowHealthIcons);
	
	UFUNCTION(BlueprintPure, Category = "HUD")
	static bool LoadShowHealthIconSetting(bool& bShowHealthIcons);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	static bool SaveShowTeamStatus(bool bShowTeamStatus);
	
	UFUNCTION(BlueprintPure, Category = "HUD")
	static bool LoadShowTeamStatus(bool& bShowTeamStatus);

	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	static bool SaveColorblindStrength(float ColorblindStrength);
	
	UFUNCTION(BlueprintPure, Category = "Accessibility")
	static bool LoadColorblindStrength(float& ColorblindStrength);

	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	static bool SaveColorblindMode(EColorVisionDeficiency ColorVisionDeficiency);
	
	UFUNCTION(BlueprintPure, Category = "Accessibility")
	static bool LoadColorblindMode(EColorVisionDeficiency& ColorVisionDeficiency);

	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	static bool SaveHighlightWeapons(bool bHighlightWeapons);

	UFUNCTION(BlueprintPure, Category = "Accessibility")
	static bool LoadHighlightWeapons(bool& bHighlightWeapons);

	UFUNCTION(BlueprintCallable, Category = "Accessibility")
	static bool SaveWorldSpaceActionPrompts(bool bWorldSpaceActionPrompts);

	UFUNCTION(BlueprintPure, Category = "Accessibility")
	static bool LoadWorldSpaceActionPrompts(bool& bWorldSpaceActionPrompts);
	
	UFUNCTION(BlueprintCallable, Category = "Command")
    static bool SaveItemSelectionStyleSettings(EItemSelectionInterfaceType ItemSelectionInterface = EItemSelectionInterfaceType::Wheel);

	UFUNCTION(BlueprintPure, Category = "Command")
    static bool LoadItemSelectionStyleSettings(EItemSelectionInterfaceType& ItemSelectionInterface);

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
		static bool SaveGrenadeSettings(EGrenadeThrowSettingType GrenadeThrowType = EGrenadeThrowSettingType::GUT_QuickGrenadeThrow);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
		static bool LoadGrenadeSettings(EGrenadeThrowSettingType& GrenadeThrowType);

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
		static bool SaveShotgunSettings(EShotgunReloadType ShotgunReloadType = EShotgunReloadType::SRT_SingleLoad);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
		static bool LoadShotgunSettings(EShotgunReloadType& ShotgunReloadType);

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
		static bool SaveEmptyMagReloadSettings(EEmptyMagReloadType EmptyMagReloadType = EEmptyMagReloadType::FastReload);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
		static bool LoadEmptyMagReloadSettings(EEmptyMagReloadType& EmptyMagReloadType);
	
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	static bool SaveDefaultCommand(ESwatCommand DefaultCommand/*, ESwatCommand DefaultHumanCommand*/, ESwatCommand DefaultDoorUnknownCommand, ESwatCommand DefaultDoorOpenCommand, ESwatCommand DefaultDoorLockedCommand, ESwatCommand DefaultDoorUnlockedCommand);
	
	UFUNCTION(BlueprintPure, Category = "Gameplay")
	static bool LoadDefaultCommands(ESwatCommand& DefaultCommand/*, ESwatCommand& DefaultHumanCommand*/, ESwatCommand& DefaultDoorUnknownCommand, ESwatCommand& DefaultDoorOpenCommand, ESwatCommand& DefaultDoorLockedCommand, ESwatCommand& DefaultDoorUnlockedCommand);
	
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
		static bool SaveDefaultSurfaceCommand(ESwatCommand DefaultCommand, int32 DefaultCommandIndex);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
		static bool LoadDefaultSurfaceCommand(ESwatCommand& DefaultCommand, int32& DefaultCommandIndex);

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
		static bool SaveDefaultDoorUnknownCommand(ESwatCommand DefaultDoorUnknownCommand, int32 DefaultDoorUnknownCommandIndex);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
		static bool LoadDefaultDoorUnknownCommand(ESwatCommand& DefaultDoorUnknownCommand, int32& DefaultDoorUnknownCommandIndex);

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
		static bool SaveDefaultDoorOpenCommand(ESwatCommand DefaultDoorOpenCommand, int32 DefaultDoorOpenCommandIndex);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
		static bool LoadDefaultDoorOpenCommand(ESwatCommand& DefaultDoorOpenCommand, int32& DefaultDoorOpenCommandIndex);

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
		static bool SaveDefaultDoorLockedCommand(ESwatCommand DefaultDoorLockedCommand, int32 DefaultDoorLockedCommandIndex);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
		static bool LoadDefaultDoorLockedCommand(ESwatCommand& DefaultDoorLockedCommand, int32& DefaultDoorLockedCommandIndex);

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
		static bool SaveDefaultDoorUnlockedCommand(ESwatCommand DefaultDoorUnlockedCommand, int32 DefaultDoorUnlockedCommandIndex);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
		static bool LoadDefaultDoorUnlockedCommand(ESwatCommand& DefaultDoorUnlockedCommand, int32& DefaultDoorUnlockedCommandIndex);

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	static bool SaveDefaultCommandAsOption(int32 DefaultCommandOption/*, int32 DefaultHumanCommandOption*/, int32 DefaultDoorUnknownCommandOption, int32 DefaultDoorOpenCommandOption, int32 DefaultDoorLockedCommandOption, int32 DefaultDoorUnlockedCommandOption);

	UFUNCTION(BlueprintPure, Category = "Gameplay")
	static bool LoadDefaultCommandsAsOption(int32& DefaultCommandOption/*, int32& DefaultHumanCommandOption*/, int32& DefaultDoorUnknownCommandOption, int32& DefaultDoorOpenCommandOption, int32& DefaultDoorLockedCommandOption, int32& DefaultDoorUnlockedCommandOption);
	
	UFUNCTION(BlueprintCallable, Category = "NVG")
	static bool SaveNVGStyle(const ENVGStyle NewNVGStyle);
	UFUNCTION(BlueprintPure, Category = "Gameplay")
	static bool LoadNVGStyle(ENVGStyle& NVGStyle);

	UFUNCTION(BlueprintCallable, Category="Low Ready")
	static bool SaveLowReadyStyle(bool bUseHighReady);

	UFUNCTION(BlueprintCallable, Category="Low Ready")
	static bool LoadLowReadyStyle(bool& bUseHighReady);
	
	/* Subtitles */
	
	UFUNCTION(BlueprintCallable, Category="Subtitles")
	static bool SaveSubtitlesEnabled(bool bEnableSubtitles);

	UFUNCTION(BlueprintCallable, Category="Subtitles")
	static bool LoadSubtitlesEnabled(bool& bEnableSubtitles);

	UFUNCTION(BlueprintCallable, Category="Subtitles")
	static bool SaveSubtitlesSize(ESubtitlesSize SubtitlesSize);

	UFUNCTION(BlueprintCallable, Category="Subtitles")
	static bool LoadSubtitlesSize(ESubtitlesSize& SubtitlesSize);

	UFUNCTION(BlueprintCallable, Category="Subtitles")
	static bool SaveSubtitlesLocale(FString SubtitlesLocale);

	UFUNCTION(BlueprintCallable, Category="Subtitles")
	static bool LoadSubtitlesLocale(FString& SubtitlesLocale);
	
	UFUNCTION(BlueprintCallable, Category="Subtitles")
	static bool SaveSubtitlesBackgroundOpacity(float SubtitlesBackgroundOpacity);

	UFUNCTION(BlueprintCallable, Category="Subtitles")
	static bool LoadSubtitlesBackgroundOpacity(float& SubtitlesBackgroundOpacity);
	
	UFUNCTION(BlueprintCallable, Category="Subtitles")
	static bool SaveSubtitlesSpeed(float SubtitlesSpeed);

	UFUNCTION(BlueprintCallable, Category="Subtitles")
	static bool LoadSubtitlesSpeed(float& SubtitlesSpeed);
	
	UFUNCTION(BlueprintCallable, Category="Subtitles")
	static bool SaveReplayEnabled(bool bReplayEnabled);
    
    UFUNCTION(BlueprintCallable, Category="Subtitles")
    static bool LoadReplayEnabled(bool& bReplayEnabled);
	
	UFUNCTION(BlueprintPure, Category = "Gameplay")
	static class ASuspectCharacter* GetClosestActiveSuspect(const FVector& Location, float Distance = 800.0f, bool bMustHaveTarget = false);
	
	UFUNCTION(BlueprintPure, Category = "Gameplay")
	static class ACivilianCharacter* GetClosestActiveCivilian(const FVector& Location, float Distance = 800.0f, bool bMustHaveTarget = false);
	
	UFUNCTION(BlueprintPure, Category = "SaveGame")
	static class UReadyOrNotSaveGame* GetLoadGameInstance(FString LoadSlotName = "");

	UFUNCTION(BlueprintPure, BlueprintPure, Category = "Profiles")
		static class UReadyOrNotProfile* GetCurrentProfile(UWorld* WorldContext = nullptr);
	
	UFUNCTION(BlueprintPure, BlueprintPure, Category = "Profiles|Multiplayer")
	static class UReadyOrNotMultiplayerProfile* GetMultiplayerProfile(FString LoadSlotName = "LevelStats");
	
	UFUNCTION(BlueprintPure, Category = "Version")
		static FString GetProjectVersion();

	UFUNCTION(BlueprintPure, Category = "Version")
	static int32 GetProjectVersionAsInt();

	UFUNCTION(BlueprintPure, Category = "Version")
	static FString GetProjectName();
	
	UFUNCTION(BlueprintPure, Category = "Version")
	static bool IsShippingBuild();

	UFUNCTION(BlueprintCallable, Category = "Settings")
		static bool SetLastConnectedServerIP(FString IP);

	UFUNCTION(BlueprintPure, Category = "Settings")
		static bool GetLastConnectedServerIP(FString& IP);

	UFUNCTION(BlueprintCallable, Category = "Settings")
	static void ToggleGrenadeDrawDebug();

	
	UFUNCTION(BlueprintCallable, Category = "Settings")
	static void ToggleFriendlyNameplates();

	UFUNCTION(BlueprintPure, Category = "Debug")
		static FString GetAdditionalBugReportInformation(APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category = "Settings")
		static bool SaveSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
		static bool ReloadSettings();

	template<typename T>
	static FString EnumToString(const FString& enumName, const T value)
	{
		UEnum* pEnum = FindObject<UEnum>(ANY_PACKAGE, *enumName);
		return *(pEnum ? pEnum->GetNameStringByValue(static_cast<uint8>(value)) : "null");
	}

	UFUNCTION(BlueprintPure, Category = "FPS")
		static bool GetShowFPS(bool& bShowFPS);

	UFUNCTION(BlueprintCallable, Category = "FPS")
		static bool SetShowFPS(bool bShowFPS);

	UFUNCTION(BlueprintPure, Category = "HUD")
		static bool GetShowControls(bool& bShowControls);

	UFUNCTION(BlueprintCallable, Category = "HUD")
		static bool SetShowControls(bool bShowControls);

	UFUNCTION(BlueprintPure, Category = "Game Instance")
		static class UReadyOrNotGameInstance* GetGameInstance(UWorld* WorldContext = nullptr);

	UFUNCTION(BlueprintPure, Category = "Map Statistics")
		static class AMapStatisticsSystem* GetMapStatistics(UWorld* WorldContext = nullptr);
	
	UFUNCTION(BlueprintPure, Category = "Map Statistics")
	static bool GetSendMapStatistics(bool& bSendMapStatistics);
	
	UFUNCTION(BlueprintCallable, Category = "Map Statistics")
	static bool SetSendMapStatistics(bool bSendMapStatistics);
	
	UFUNCTION(BlueprintPure, Category = "Compass")
		static FString ConvertDegreeIntoLetter(float Degrees);

	// Gets the global item data. Works on both sides. (client/server)
	// Note, calling this without a World Context will cause a dedicated server to crash!
	UFUNCTION(BlueprintPure, Category = Data)
		static UItemData* GetItemData(UWorld* WorldContext = nullptr);
	
	UFUNCTION(BlueprintPure, Category = Data)
		static UDataTable* GetPairedInteractionDataTable();
	
	UFUNCTION(BlueprintPure, Category = Data)
		static UDataTable* GetMoveStyleDataTable();

	// Gets the global level data. Works on both sides. (client/server)
	// Note, calling this without a World Context will cause a dedicated server to crash!
	UFUNCTION(BlueprintPure, Category = Data)
		static FLevelDataLookupTable GetLevelData(UWorld* WorldContext = nullptr);

	// Gets the global music data for the level. Works on both sides. (client/server)
	// Note, calling this without a World Context will cause a dedicated server to crash!
	UFUNCTION(BlueprintPure, Category = Data)
		static UMusicData* GetMusicData(UWorld* WorldContext = nullptr);

	
	UFUNCTION(BlueprintCallable, Category = Attachments)
		static void AttachMagazinesToWeapon(TSubclassOf<class ABaseMagazineWeapon> WeaponClass, class ABaseMagazineWeapon* Weapon);

	UFUNCTION(BlueprintCallable, Category = UI)
		static void PlayInterfaceSound(UWorld* WorldContext, EInterfaceSoundType SoundClass);

	UFUNCTION(BlueprintPure, BlueprintPure, Category = Attachments)
		static int GetAttachmentPointsRemaining(FSavedLoadout Loadout);
	
	static class APlayerCharacter* GetLocalPlayerCharacter(UWorld* World);
	static class AReadyOrNotPlayerController* GetLocalPlayerController(UWorld* World);

	UFUNCTION(BlueprintPure, Category = "Campaign Completion")
		static UWorld* GetWorldBP(APlayerController* pc);
	
	UFUNCTION(BlueprintPure, Category = "Campaign Completion")
	static bool IsObjectiveTarget(AReadyOrNotCharacter* Target, AReadyOrNotCharacter* LocalPlayer);

	UFUNCTION(BlueprintPure, Category = "Lookup Data")
		static class ULookupData* GetLookupData();

	UFUNCTION(BlueprintCallable, Category = "Customization")
		static void LoadCustomizationLevels(UWorld* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "Customization")
		static void UnloadCustomizationLevels(UWorld* WorldContext);

	UFUNCTION(BlueprintPure, Category = "Penetration")
		static UPenetrationData* GetPenetrationData();

	// NOTE(killo): the item data table has been deprecated, please use fields in weapon blueprint directly
	// there are also the ULoadoutManager:: statics that may be helpful
	// UFUNCTION(BlueprintPure, Category = "Lookup Data")
	// 	static class UDataTable* GetItemLookupDataTable();

	UFUNCTION(BlueprintPure, Category = "Lookup Data")
		static class UDataTable* GetAmmoLookupDataTable();
	
	UFUNCTION(BlueprintPure, Category = "Lookup Data")
		static class UDataTable* GetInputKeyGamepadIconLookupDataTable();

	UFUNCTION(BlueprintPure, Category = "Lookup Data")
		static class UDataTable* GetLevelLookupDataTable();
	
	UFUNCTION(BlueprintPure, Category = "Lookup Data")
		static class UDataTable* GetAILookupDataTable();
	
	UFUNCTION(BlueprintPure, Category = "Lookup Data")
		static class UDataTable* GetAnimatedIconLookupDataTable();

	UFUNCTION(BlueprintPure, Category = "Lookup Data")
		static class UDataTable* GetSpeechLookupDataTable(FString Speaker);

	UFUNCTION(BlueprintPure, Category = "Lookup Data")
		static class UDataTable* GetDoorLookupDataTable();

	UFUNCTION(BlueprintPure, Category = "Lookup Data")
		static class UDataTable* GetTrapLookupDataTable();

	UFUNCTION(BlueprintPure, Category = "Lookup Data")
	static class UDataTable* GetCharacterLookOverrideDataTable();

	UFUNCTION(BlueprintPure, Category = "Lookup Data")
	static class UDataTable* GetConversationLookupDataTable();
	
	UFUNCTION(BlueprintPure, Category = "Lookup Data")
    static class UDataTable* GetGameModeSettingsLookupDataTable();
    
	UFUNCTION(BlueprintPure, Category = "Lookup Data")
	static class UDataTable* GetSuspectArmourDataTable();

	UFUNCTION(BlueprintPure, Category = "Lookup Data|Animated Icon")
    static FAnimatedIcon GetAnimatedIconFromTable(FName RowName, bool& bSuccess);

	template <typename ObjClass>
	static FORCEINLINE ObjClass* LoadObjFromPath(const FName& Path)
	{
		if (Path == NAME_None) return NULL;
		//~

		return Cast<ObjClass>(StaticLoadObject(ObjClass::StaticClass(), NULL, *Path.ToString()));
	}

	UFUNCTION(BlueprintPure, Category = Level)
		static bool IsPvPSupported(FLevelDataLookupTable LookupTable);

	UFUNCTION(BlueprintPure, Category = Level)
		static bool IsCOOPSupported(FLevelDataLookupTable LookupTable);

	UFUNCTION(BlueprintPure, Category = "Player")
		static class AReadyOrNotPlayerState* GetLocalPlayerState(UWorld* World);

	UFUNCTION(BlueprintPure, BlueprintPure, Category = "LevelStats")
		static class ULicenseSave* LoadLicenseSave();

	UFUNCTION(BlueprintCallable, Category = LevelStats)
		static void SaveLicenseSave(class ULicenseSave* LevelStats);

	UFUNCTION(BlueprintPure, Category = Damage, Meta = (ExpandEnumAsExecs = "Branches"))
		static FPointDamageEvent CastToPointDamageEvent(FDamageEvent DamageEvent, EStructureCastPathway& Branches);

	UFUNCTION(BlueprintPure, Category = Damage, Meta = (ExpandEnumAsExecs = "Branches"))
		static FRadialDamageEvent CastToRadialDamageEvent(FDamageEvent DamageEvent, EStructureCastPathway& Branches);

	// warning: EXPENSIVE to call this
	UFUNCTION(BlueprintPure, Category = Player)
		static class APlayerCharacter* FindClosestDeadGuyInRadius(FVector Origin, AActor* Causer, float Radius, bool bIncludeUnconscious);

	UFUNCTION(BlueprintPure)
		static FString GetDMOAddress();

	UFUNCTION(BlueprintPure)
		static FString GetDMOGameMode();

	UFUNCTION(BlueprintPure)
		static ETeamType GetDMOTeamType();

	UFUNCTION(BlueprintPure, BlueprintPure, Category = Editor)
		static bool IsEditorBuild()
	{
#if WITH_EDITOR
		return true;
#else
		return false;
#endif
	}

	UFUNCTION(BlueprintPure, BlueprintPure, Category = Build)
		static FString GetBuildDate();

	UFUNCTION(BlueprintPure, BlueprintPure, Category = Build)
		static FString GetBuildTime();

	// backwards compatibility
	UFUNCTION(BlueprintPure)
	static EWeaponType ConvertItemTypeToWeaponType(EItemType ItemType);

	UFUNCTION(BlueprintPure)
	static EItemType ConvertWeaponTypeToItemType(EWeaponType WeaponType);

	static void SpawnTacticalItem(ABaseItem** Target, TSubclassOf<ABaseItem> TargetClass, AReadyOrNotCharacter* TargetOwner, bool bReplicates, int32& GrenadeCount);

	UFUNCTION(BlueprintPure, meta=(WorldContext="WorldContextObject"))
	static bool IsMultiplayer(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, meta=(WorldContext="WorldContextObject"))
	static bool IsLobby(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, meta=(WorldContext="WorldContextObject"))
	static bool IsCommanderMode(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, meta=(WorldContext="WorldContextObject"))
	static bool IsIronmanMode(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"))
    static bool IsConsole(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Interactables", meta=(WorldContext="WorldContextObject"))
	static void UpdateInteractableComponentsWorldSpaceActionPrompts(const UObject* WorldContextObject, bool bEnableWorldSpaceActionPrompts);
};



USTRUCT(BlueprintType)
struct FSavedWeaponAttachmentData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Details")
	bool bHasSavedData = false;
	
	UPROPERTY(BlueprintReadWrite, Category = "Details")
	TSubclassOf<UWeaponAttachment> ScopeAttachment;

	UPROPERTY(BlueprintReadWrite, Category = "Details")
	TSubclassOf<UWeaponAttachment> MuzzleAttachment;

	UPROPERTY(BlueprintReadWrite, Category = "Details")
	TSubclassOf<UWeaponAttachment> UnderbarrelAttachment;

	UPROPERTY(BlueprintReadWrite, Category = "Details")
	TSubclassOf<UWeaponAttachment> OverbarrelAttachment;
	
	UPROPERTY(BlueprintReadWrite, Category = "Details")
	TSubclassOf<UWeaponAttachment> StockAttachment;
	
	UPROPERTY(BlueprintReadWrite, Category = "Details")
	TSubclassOf<UWeaponAttachment> GripAttachment;
	
	UPROPERTY(BlueprintReadWrite, Category = "Details")
	TSubclassOf<UWeaponAttachment> IlluminatorAttachment;
	
	UPROPERTY(BlueprintReadWrite, Category = "Details")
	TSubclassOf<UWeaponAttachment> AmmunitionAttachment;
	
	UPROPERTY(BlueprintReadWrite, Category = "Details")
	TSubclassOf<USkinComponent> Skin;
	
	UPROPERTY(BlueprintReadWrite, Category = "Details")
	int32 AmmoCount;

	FSavedWeaponAttachmentData()
	{
		bHasSavedData = false;
		AmmoCount = 4;
	}
};

USTRUCT(BlueprintType)
struct FStoredWeaponAttachments
{
	GENERATED_USTRUCT_BODY()

public:
	FStoredWeaponAttachments() 
	{
		bIsEmpty = true;
	}

	UPROPERTY(BlueprintReadOnly, Category = "Details")
	TSubclassOf<UWeaponAttachment> ScopeAttachment;

	UPROPERTY(BlueprintReadOnly, Category = "Details")
	TSubclassOf<UWeaponAttachment> MuzzleAttachment;

	UPROPERTY(BlueprintReadOnly, Category = "Details")
	TSubclassOf<UWeaponAttachment> UnderbarrelAttachment;

	UPROPERTY(BlueprintReadOnly, Category = "Details")
	TSubclassOf<UWeaponAttachment> OverbarrelAttachment;

	UPROPERTY(BlueprintReadOnly, Category = "Details")
	TSubclassOf<UWeaponAttachment> StockAttachment;

	UPROPERTY(BlueprintReadOnly, Category = "Details")
	TSubclassOf<UWeaponAttachment> GripAttachment;

	UPROPERTY(BlueprintReadOnly, Category = "Details")
	TSubclassOf<UWeaponAttachment> IlluminatorAttachment;

	UPROPERTY(BlueprintReadOnly, Category = "Details")
	TSubclassOf<UWeaponAttachment> AmmunitionAttachment;

	UPROPERTY(BlueprintReadOnly, Category = "Details")
	bool bIsEmpty = true;
};

USTRUCT(BlueprintType)
struct FWeaponPreset
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Weapon Preset")
	uint8 bHasSavedData : 1;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon Preset")
	uint8 bSelected : 1;
	
	UPROPERTY(BlueprintReadWrite, Category = "Weapon Preset")
	FName PresetName = "Preset 01";

	UPROPERTY(BlueprintReadWrite, Category = "Weapon Preset")
	FSavedWeaponAttachmentData AttachmentData;
};

USTRUCT(BlueprintType)
struct FSavedWeaponPreset
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Weapon Preset")
	TArray<FWeaponPreset> Presets;
};

USTRUCT(BlueprintType)
struct FLoadoutPreset
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Loadout Preset")
	FName PresetName = "Loadout 01";

	UPROPERTY(BlueprintReadWrite, Category = "Loadout Preset")
	FSavedLoadout Loadout;
};

UCLASS()
class READYORNOT_API UReadyOrNotSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UReadyOrNotSaveGame();

	UPROPERTY(VisibleAnywhere, Category = Basic)
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	uint32 UserIndex;
	
	UPROPERTY(BlueprintReadWrite)
	TMap<ETeamType, TSubclassOf<class USkinComponent>> SkinSaveMap;
	
	// Default savegame
	UFUNCTION()
	static UReadyOrNotSaveGame* CreateDefaultSavegame(FString LoadSlotName);
};

UCLASS()
class READYORNOT_API UReadyOrNotSessionData : public USaveGame
{
	GENERATED_BODY()

public:
	UReadyOrNotSessionData();

	UPROPERTY(VisibleAnywhere, Category = Basic)
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	uint32 UserIndex;

	// FString is steamid64, int32 is how many tks have occured for this player
	UPROPERTY()
	TMap<FString, int32> SavedTeamKillData;

	// Store the ban reason so we can report it to the player next time they try and join
	UPROPERTY()
	TMap<FString, FString> BanReasonData;
	
	static UReadyOrNotSessionData* CreateDefaultSavegame(FString LoadSlotName);
};

UCLASS()
class READYORNOT_API UReadyOrNotModData : public USaveGame
{
	GENERATED_BODY()

public:
	UReadyOrNotModData();

	UPROPERTY(VisibleAnywhere, Category = Basic)
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	uint32 UserIndex;

	UPROPERTY()
	TArray<int64> DisabledMods;

	UPROPERTY()
	TArray<int64> ErroredMods;

	static UReadyOrNotModData* CreateDefaultSavegame(FString LoadSlotName);
	static UReadyOrNotModData* Get();
};

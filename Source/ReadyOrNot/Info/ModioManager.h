// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#if defined(WITH_MODIO)
#include "ModioSubsystem.h"
#include "Types/ModioErrorCode.h"
#endif
#include "ModioManager.generated.h"

// TODO(killo): ModioSubsystem->GetModDependenciesAsync

// Should be able to also get map builtdata from mod zips to save end user time

UENUM(BlueprintType)
enum class EModStatus : uint8
{
	Unsubscribed				UMETA(DisplayName="Unsubscribed"),	// Not installed or disabled, ready to be installed
	Installing					UMETA(DisplayName="Installing"),	// Installation in progress
	Updating					UMETA(DisplayName="Updating"),		// Update in progress
	Installed					UMETA(DisplayName="Active"),		// When the mod is installed and active
	Disabled					UMETA(DisplayName="Disabled"),		// When the mod is installed, but disabled
	Uninstalling				UMETA(DisplayName="Uninstalling"),	// Uninstallation in progress
	Error						UMETA(DisplayName="Error"),			// Unspecified mod-specific error
};

#if defined(WITH_MODIO)
DECLARE_EVENT_OneParam(UModioManager, FModioManagerInitialized, TOptional<FString> /* Optional Error String */);
DECLARE_EVENT_OneParam(UModioManager, FModioModEvent, FModioModID /* Mod */);
#endif

/**
 *  Manager class for Mod.io integration, runs background tasks and handles mod loading for mod.io mods
 */
UCLASS()
class READYORNOT_API UModioManager : public UObject, public FTickableGameObject
{
	GENERATED_BODY()
	
protected:
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual TStatId GetStatId() const override { return UObject::GetStatID(); }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const override { return true; }

public:
	static bool IsPackagedBuild();
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static UModioManager* GetInstance();
	
	void Initialize(UReadyOrNotGameInstance* GameInstance);
	void Shutdown(bool bRestart = false);
	
#if defined(WITH_MODIO)
	void MountInstalledMods();
	bool MountSingleMod(const FModioModID InModId);
	void UnmountMod(const FModioModID InModId);
	void DisableMod(const FModioModID InModId);
	void EnableMod(const FModioModID InModId);
	bool IsModEnabled(const FModioModID InModId) const;

	EModStatus GetModStatus(const FModioModID InModId);
	bool IsModPendingRestart(const FModioModID InModId);
#endif

	// Clears the mod.io user data present on this machine (sign out)
	UFUNCTION(BlueprintCallable)
	void ClearUserData();
	
	UFUNCTION(BlueprintCallable)
	bool IsRestartRequired() const; 
	void SetRestartRequired() { bRestartRequired = true; };

	UFUNCTION(BlueprintCallable)
	bool IsModUpdating();

	// Is ModIO Compiled into the game?
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool IsModIoEnabled();
	
	bool IsInitialized() const { return bShouldTick; }

	/**
	 * @brief 
	 * @return Get the base location where mod paks should be installed
	 */
	static FString GetModInstallDirectory();

	static FString GetModIODirectory();

	static FString GetPakIniPath();
	
#if defined(WITH_MODIO)
	static FModioManagerInitialized& OnInitialized() { return InitializedEvent; }
	static FModioModEvent& OnModInstalledEvent() { return ModInstalledEvent; }
	static FModioModEvent& OnModUninstalledEvent() { return ModUninstalledEvent; }
	static FModioModEvent& OnModStateUpdated() { return ModStateUpdatedEvent; }
#endif
	
private:
	bool bShouldTick = false;
	bool bInitialized = false;
	bool bRestartRequired = false;
	TArray<FString> MountedPaks;

	void LoadMountedPakInfo();
	void SaveModState();
	void LoadModState();
	void SavePakIni();

#if defined(WITH_MODIO)
	UModioSubsystem* ModioSubsystem = nullptr;
	FPakPlatformFile* PakPlatformFile = nullptr;;

	TSet<FModioModID> ModsWithErrors;
	TSet<FModioModID> ModsDisabled;
	TSet<FModioModID> ModsPendingRestart;
	
	// Used to check for pending restart
	TSet<FModioModID> ModsActiveOnStartup;

	// Used to check for ongoing events
	TSet<FModioModID> ModsCurrentlyUpdating;
	TSet<FModioModID> ModsCurrentlyInstalling;
	
	static FModioManagerInitialized InitializedEvent;
	static FModioModEvent ModInstalledEvent;
	static FModioModEvent ModUninstalledEvent;
	static FModioModEvent ModStateUpdatedEvent;

	void GetPaksForModioMod(const FModioModID InModId, TArray<FString>& OutPakFiles);
	
	void InternalMountMod(const FModioModID InModId, const TMap<FModioModID, FModioModCollectionEntry>& InstalledMods);
	
	void HandleModioInitialized(FModioErrorCode ErrorCode);
	void HandleModioShutdown(FModioErrorCode ErrorCode, bool bRestart);

	void HandleUserDataCleared(FModioErrorCode ErrorCode);
	void HandleModManagementEvent(FModioModManagementEvent Event);
#endif
};

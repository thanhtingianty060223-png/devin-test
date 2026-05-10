// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Commander/CommanderProfile.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineGameActivityInterface.h"
#include "PS5ActivitiesSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogReadyOrNotPS5Activities, Log, All);

UENUM(BlueprintType)
enum class EActivitiesObjective : uint8
{
	GAS,
	STREAMER, 
	METH,
	AGENCY,
	RIDGELINE,
	PENTHOUSE, 
	DATACENTER,
	BEACHFRONT,
	IMPORTER,
	VALLEY,
	CAMPUS,
	COYOTE,
	SINS,
	CLUB,
	DEALER,
	FARM,
	HOSPITAL,
	PORT
};
ENUM_RANGE_BY_FIRST_AND_LAST(EActivitiesObjective, EActivitiesObjective::GAS, EActivitiesObjective::PORT);

UCLASS()
class READYORNOT_API UPS5ActivitiesSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT( UConsoleMultiplayerSubsystem, STATGROUP_Tickables );
	}

	virtual bool IsTickableWhenPaused() const
	{
		return true;
	}
	virtual bool IsTickableInEditor() const
	{
		return false;
	}
	
	virtual void Tick( float DeltaTime ) override;

	void ActivityStart(const FUniqueNetId& LocalUserId, FString& Activity);
	void ActivityEnd(const FUniqueNetId& LocalUserId, FString& Activity, EOnlineActivityOutcome ActivityOutcome);
	void ActivitySetAvailable(const FUniqueNetId& LocalUserId, FString& Activity, bool bAvailable = true);
	void ResetAllActiveActivities(const FUniqueNetId& LocalUserId);
	void OnLoadGame(const FUniqueNetId& LocalUserId, UCommanderProfile* CommanderProfile);
	void OnSaveGame(const FUniqueNetId& LocalUserId, UCommanderProfile* CommanderProfile); 

private:
	bool bInitialized = false;

	uint32 LastFrameNumberWeTicked = INDEX_NONE;
	
	// received when the user has started a activity through the console UI
	FOnGameActivityActivationRequestedDelegate OnGameActivityActivationRequestedDelegate;
	FDelegateHandle OnGameActivityActivationRequestedDelegateHandle;

	// received when the activation system is initialized
	// FOnStartActivityComplete OnStartActivityCompleteDelegate;
	// FDelegateHandle OnStartActivityCompleteDelegateHandle;

	void ActivityActivationRequested(const FUniqueNetId& /* LocalUserId */, const FString& /* ActivityId */, const FOnlineSessionSearchResult* /* SessionInfo */);
	// void ActivityStartComplete(const FUniqueNetId& /* LocalUserId */, const FString& /* ActivityId */, const FOnlineError& /* Status */) const;

	IOnlineGameActivityPtr GetOnlineGameActivityPtr();

	UCommanderProfile* GetLastUsedCommanderProfile();

};

UCLASS()
class READYORNOT_API UPS5ActivitiesStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

private:
	static UPS5ActivitiesSubsystem* GetAchievementSubsystem(UObject* WorldContextObject);
	static FUniqueNetIdPtr GetLocalUserId(UObject* WorldContextObject);	
	
public:
	static void ActivityStart(UObject* WorldContextObject, FString& Activity);
	static void ActivityEnd(UObject* WorldContextObject, FString& Activity, EOnlineActivityOutcome ActivityOutcome);
	static void ActivitySetAvailable(UObject* WorldContextObject, FString& Activity, bool bAvailable = true);
	static void ResetAllActiveActivities(UObject* WorldContextObject);
	static void OnSaveGame(UObject* WorldContextObject, UCommanderProfile* CommanderProfile);
	static void OnLoadGame(UObject* WorldContextObject, UCommanderProfile* CommanderProfile);
};

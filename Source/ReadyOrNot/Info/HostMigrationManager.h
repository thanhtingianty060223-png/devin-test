// Copyright Void Interactive, 2022

#pragma once

#include "Actors/Gameplay/BombActor.h"
#include "Actors/Gameplay/TrapActor.h"
#include "FindSessionsCallbackProxy.h"
#include "Actors/BaseMagazineWeapon.h"
#include "HostMigrationManager.generated.h"

USTRUCT(BlueprintType)
struct FHm_InventoryInformation
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TSubclassOf<ABaseItem> Class;

	UPROPERTY()
	TArray<FMagazine> Magazines;

	UPROPERTY()
	int32 MagIndex;
};

USTRUCT(BlueprintType)
struct FHm_PlayerInformation
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString UniqueId;

	UPROPERTY()
	TArray<uint8> SaveData;

	UPROPERTY()
	FTransform CharacterTransform;

	UPROPERTY()
	FRotator ControlRotation;	

	UPROPERTY()
	float Health;

	UPROPERTY()
	TSubclassOf<ABaseItem> EquippedItemClass;

	UPROPERTY()
	TArray<FHm_InventoryInformation> Inventory;

	UPROPERTY()
	int32 TotalGrenades;

	UPROPERTY()
	int32 TotalDevices;

	UPROPERTY()
	bool bHasBeenReported;

	FHm_PlayerInformation()
	{
		UniqueId = "";
		CharacterTransform = FTransform();
		ControlRotation = FRotator::ZeroRotator;
		Health = 100.0f;
		EquippedItemClass = nullptr;
		Inventory = {};
		TotalGrenades = 0;
		TotalDevices = 0;
		bHasBeenReported = false;
		
	}
};

USTRUCT(BlueprintType)
struct FHm_CyberneticsInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FTransform CharacterTransform;

	UPROPERTY()
	ETeamType TeamType;
	
	UPROPERTY()
	float Health;

	UPROPERTY()
	bool bIsArrested;

	UPROPERTY()
	bool bIsSurrendered;

	UPROPERTY()
	bool bHasBeenReported;

	UPROPERTY()
	TArray<FName> Tags;

	UPROPERTY()
	TSubclassOf<ABaseItem> EquippedItemClass;

	UPROPERTY()
	FCharacterMesh CharacterMeshData;

	FHm_CyberneticsInformation()
	{
		CharacterTransform = FTransform();
		TeamType = ETeamType::TT_NONE;
		Health = 0.0f;
		bIsArrested = false;
		bIsSurrendered = false;
		bHasBeenReported = false;
		EquippedItemClass = nullptr;
		CharacterMeshData = FCharacterMesh();
	}
	
};

USTRUCT(BlueprintType)
struct FHm_DoorChunkInformation
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform Transform;
	
	UPROPERTY()
	bool bIsSimulating = false;

	FHm_DoorChunkInformation()
	{
		Transform = FTransform();
		bIsSimulating = false;
	}
};

USTRUCT(BlueprintType)
struct FHm_DoorInformation
{

	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	bool bIsBroken;

	UPROPERTY()
	float OpenCloseAmount;

	UPROPERTY()
	FTransform DoorMeshTransform;
	
	UPROPERTY()
	bool bIsSimulatingPhysics;

	UPROPERTY()
	TArray<FHm_DoorChunkInformation> DoorChunkInformations;
	
	UPROPERTY()
	FName TrapName;	

	FHm_DoorInformation()
	{
		Name = "";
		OpenCloseAmount = 0.0f;
		bIsBroken = false;
		DoorMeshTransform = FTransform();
		bIsSimulatingPhysics = false;
		DoorChunkInformations = {};
		TrapName = NAME_None;
	}
	
};

USTRUCT(BlueprintType)
struct FHm_BombInformation
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString BombName;

	UPROPERTY()
	EBombState BombState;

	UPROPERTY()
	float TimeRemaining = 0.0f;
	FHm_BombInformation()
	{
		BombName = "";
		BombState = EBombState::BS_None;
		TimeRemaining = 0.0f;
	};
};

USTRUCT(BlueprintType)
struct FHm_BaggedEvidence
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform Transform;

	FHm_BaggedEvidence()
	{
		Transform = FTransform();
	}
	
};


USTRUCT(BlueprintType)
struct FHm_DroppedEvidence
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	TSubclassOf<ABaseItem> Class;

	FHm_DroppedEvidence()
	{
		Transform = FTransform();
		Class = nullptr;
	}
	
};

USTRUCT(BlueprintType)
struct FHm_Objectives
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	EObjectiveStatus ObjectiveStatus;

	FHm_Objectives()
	{
		Name = "";
		ObjectiveStatus = EObjectiveStatus::Objective_InProgress;
	};
	
};

/**
 * 
 */
UCLASS()
class READYORNOT_API UHostMigrationManager : public UObject
{
	GENERATED_BODY()

	UHostMigrationManager();

	bool bHostMigrationInProgress = false;
	
	FTimerHandle TH_SaveState;

	UPROPERTY()
	TArray<FHm_PlayerInformation> PlayerInformations;

	UPROPERTY()
	TArray<FHm_CyberneticsInformation> CyberneticsInformations;
	
	UPROPERTY()
	TArray<FHm_DoorInformation> DoorInformations;

	UPROPERTY()
	TArray<FHm_BombInformation> BombInformations;

	UPROPERTY()
	TArray<FHm_BaggedEvidence> BaggedEvidenceInformations;
	
	UPROPERTY()
	TArray<FHm_DroppedEvidence> DroppedEvidenceInformations;
	
	UPROPERTY()
	TArray<FHm_Objectives> ObjectiveInformations;

	UPROPERTY()
	TArray<FString> ActiveEvidence;


	TArray<FArchive> SaveData;

	UPROPERTY()
	APlayerState* NextHost;

	UPROPERTY()
	FString MigrationGUID;

	int32 FindSessionAttempt = 0;



	UPROPERTY()
	FString MapName;

	UPROPERTY()
	FString ModeName;

	bool bFoundSession = false;

public:
	void Init();
	UFUNCTION()
	void SaveState();

	UPROPERTY()
	FString NextHostName;
	
	UPROPERTY()
	int32 ExpectedPlayerCount = 0;
	
	bool IsNextHost(class AReadyOrNotPlayerController* PlayerController);
	bool IsMigratingHost();
	// Don't do any updates while host migration is in progress!
	void SetHostMigrationInProgress(bool bInProgress);

	void SetNextHost(APlayerState* NewHost, FString GUID);
	void LoadState(TArray<FHm_PlayerInformation>& OutPlayerInformation,
		TArray<FHm_CyberneticsInformation>& OutCyberneticsInformation, TArray<FHm_DoorInformation>& OutDoorInformation,
		TArray<FHm_BombInformation>& OutBombInformations, TArray<FString>& OutActiveEvidence,
		TArray<FHm_BaggedEvidence>& OutBaggedEvidences, TArray<FHm_DroppedEvidence>& OutDroppedEvidence,
		TArray<FHm_Objectives>& OutObjectiveInformations);

	void StartMigration(bool bAsHost);
	
	UFUNCTION()
	void CreateMigrationSession();

	UFUNCTION()
	void OnLobbySuccess();
	UFUNCTION()
	void FindMigrationSession();
	UFUNCTION()
	void OnMigrationSessionFoundSuccess(const TArray<FBlueprintSessionResult>& Results);
	UFUNCTION()
	void OnMigrationSessionFoundFailed(const TArray<FBlueprintSessionResult>& Results);

	UFUNCTION()
	void ReturnToMainMenu();
	
	
};

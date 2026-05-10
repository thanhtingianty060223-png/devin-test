// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "ReadyOrNotGameMode.h"
#include "LobbyGM.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ALobbyGM : public AReadyOrNotGameMode
{
	GENERATED_BODY()

	ALobbyGM();
	
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	
	bool bIsServerTraveling = false;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void RespawnPlayer(APlayerController* Player, bool bForceSpectator = false) override {};
	virtual void Logout(AController* Exiting) override;

	virtual void ProcessServerTravel(const FString& URL, bool bAbsolute) override;
	
	virtual bool ShouldAlertSuspectWhenLastAlive() const override;
	virtual bool ShouldAlertCivilianWhenLastAlive() const override;
	
	UPROPERTY()
	TArray<class AReadyOrNotPlayerController*> InitalizedPlayerControllers;

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;

	virtual bool CanTakeDamage(AController* EventInstigator, AController* DamageReceiver) const override;

	void RemoveUnableToMigrateHostSetting();
	// don't load migration data in the lobby!
	virtual void LoadHostMigrationData() override {};

	UFUNCTION()
	void OnLoadingScreenCleared();
	
protected:
	bool bLastKnownGoodPlayerTransformSet = false;
	FTransform LastKnownGoodPlayerTransform;
	FRotator LastKnownGoodPlayerCameraRotation;
	
public:
	UPROPERTY()
	class UCommanderProfile* CommanderProfile;

	UPROPERTY()
	class URosterManager* RosterManager;

	void OpenLobbyWidget(FString Name);
	
	UFUNCTION(Exec)
	void OpenRosterSelection();

	UFUNCTION(Exec)
	void OpenMissionSelect();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGenericPlayerInitialization, AReadyOrNotPlayerController*, ReadyOrNotPlayerController);
	FOnGenericPlayerInitialization OnGenericPlayerInitialization;
	
	virtual void GenericPlayerInitialization(AController* C) override;
};

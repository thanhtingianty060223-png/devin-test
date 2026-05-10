// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "ReadyOrNotGameMode.h"
#include "Enums.h"
#include "TrainingGM.generated.h"

#define SEQ_NEXT_STATE(C, N) case(C): return N;

struct FSwatCommandData;
class AActivityTriggerVolume;
class ACheckpointActivityTriggerVolume;

UCLASS()
class READYORNOT_API ATrainingGM : public AReadyOrNotGameMode
{
	GENERATED_BODY()

public:
	ATrainingGM();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	virtual void StartMatch() override;
	virtual void ResetLevel() override;

	virtual void RespawnPlayer(APlayerController* Player, bool bForceSpectator = false) override;

	/** Called when the Player is killed. */
	virtual void PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;

	/** Spawns all AI controlled officers. */
	UFUNCTION(BlueprintCallable)
	void SpawnPolice(const bool bSpawnWithPlayer = false);

	/** Remove all AI controlled characters. */
	UFUNCTION(BlueprintCallable)
	void RemoveAllSpawnedAI();

	virtual void ExfiltrateMission(TArray<ASWATCharacter*> ExfilCharacters) override;

	void StartTrainingEndTimer(bool bWon);

	/** Ends the training. Transports the player back to main menu or station. */
	UFUNCTION(BlueprintCallable)
	void TrainingEnd(bool bSuccess);

	/** Get all SWAT commands that the player must issue according to current active volumes. */
	TArray<FSwatCommandData> GetCurrentCommandsToIssue();

protected:
	/** Spawns a single AI controlled officer. */
	void SpawnAIOfficer(const ESquadPosition SquadPosition, const ETeamType CommandTeam, const FString& LoadoutName, const FTransform& SpawnTransform);

	/** Setup the customization (name) of a given officer. */
	virtual void SetupOfficerCustomization(class ASWATCharacter* Character, FSavedCustomization& OutCustomization);

	/** Reset the squad leader to the first found player. */
	void ResetSquadLeader();

	/** Find the correct spawn location for the AI officers. */
	FTransform GetAIOfficerSpawnTransform(const bool bSpawnWithPlayer);

	void AdjustAIOfficerSpawnLocation();

	/** Called when the player kills a friendly AI (such as a SWAT officer). */
	UFUNCTION()
	virtual void FriendlyAIKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);

private:
	UPROPERTY()
	TArray<AReadyOrNotPlayerController*> InitalizedPlayerControllers;

	UPROPERTY()
	class UCommanderProfile* CommanderProfile = nullptr;

	FTimerHandle TrainingEndTimer;

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrainingEnded, bool, bSuccess);
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = "Transitions")
	FOnTrainingEnded OnTrainingEnded;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SWAT")
	FString SwatSpawnTag;

	UPROPERTY(EditAnywhere, Category = "SWAT")
	TSubclassOf<ACyberneticCharacter> SWATAIClass;

	UPROPERTY(EditAnywhere, Category = "SWAT")
	TSubclassOf<AAIController> FriendlyAIController;

	UPROPERTY(BlueprintReadOnly, Category = "SWAT")
	TArray<ASWATCharacter*> SpawnedSWATAI;

	UPROPERTY(BlueprintReadOnly, Category = "Transitions")
	TArray<AActivityTriggerVolume*> ActiveTriggerVolumes;

	UPROPERTY(BlueprintReadOnly, Category = "Transitions")
	ACheckpointActivityTriggerVolume* CurrentCheckpoint = nullptr;
};

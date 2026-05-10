// Void Interactive, 2020

#pragma once

#include "ReadyOrNotGameMode_PVP.h"
#include "IncriminationGM.generated.h"

/**
 * The Intel sought by both M.L.O. and SWAT teams must be found in the map.
 * There are three potential locations for it to spawn, both teams are given the same locations and the team who finds it first, becomes defence, the team who does not has to go on the offensive.
 * The team who found the Intel source must now defend the point as the other team’s new goal is to disrupt their extraction process before it can be loaded.
 */
UCLASS()
class READYORNOT_API AIncriminationGM : public AReadyOrNotGameMode_PVP
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Game Mode|Incrimination")
	bool HasVisitedAllEvidenceSpawns() const;
	
protected:
	void Tick(float DeltaSeconds) override;
	void OnRoundStarted_Implementation() override;
	void OnRoundEnded_Implementation() override;
	void CheckVictoryConditions() override;
	
	AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	APlayerStart* FindPlayerStartWithTag(const FName& Tag) const override;
	
	void RespawnPlayer(APlayerController* Player, bool bForceSpectator = false) override;

	bool ShouldSpawnAtStartSpot(AController* Player) override;

	void InitializeIncriminationRound();

	UFUNCTION(BlueprintNativeEvent, Category = "Game Mode|Incrimination")
			void OnEvidencePickedUp(AActor* PickupActor);
	virtual void OnEvidencePickedUp_Implementation(AActor* PickupActor);
	
	UFUNCTION(BlueprintNativeEvent, Category = "Game Mode|Incrimination")
			void OnEvidenceDropped(AActor* DropActor);
	virtual void OnEvidenceDropped_Implementation(AActor* DropActor);
	
	UFUNCTION(BlueprintNativeEvent, Category = "Game Mode|Incrimination")
			void OnClueFound(class AIncriminationClue* ClueActor, AActor* ClueFounder);
	virtual void OnClueFound_Implementation(class AIncriminationClue* ClueActor, AActor* ClueFounder);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setup", meta = (ClampMin = 1, UIMin = 1))
	uint8 MaxCluesToFind = 3;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
    class ASpawnGenerator* ChosenSpawnGroup_BlueTeam = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Data")
    class ASpawnGenerator* ChosenSpawnGroup_RedTeam = nullptr;
	
private:
	bool ChooseEvidenceSpawn();
	void SpawnEvidenceActor();
	void ResetAllEvidenceSpawnVisits();

	bool ChooseExtractionDevice();

	void GatherAllClueSpawnsInLevel();
	void GatherAllClueSpawnsFromChosenEvidenceBuilding();
	void SpawnChosenClues();

	void AssignRandomNonMainEvidenceSearchZones();
	
	static TArray<class AIncriminationClueSpawnPoint*> GetAllClueSpawnsOfClueNumber(const TArray<class AIncriminationClueSpawnPoint*>& InClueSpawns, int32 ClueNumber);

	class ASpawnGenerator* FindTeamSpawnGroup(const ETeamType& Team) const;
	class APlayerStart* FindPlayerStartFromSpawnGroup(ASpawnGenerator* SpawnGenerator) const;
};

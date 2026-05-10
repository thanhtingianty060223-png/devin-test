// Void Interactive, 2020

#pragma once

#include "ReadyOrNotGameState.h"
#include "IncriminationGS.generated.h"

/**
 * The Intel sought by both M.L.O. and SWAT teams must be found in the map.
 * There are three potential locations for it to spawn, both teams are given the same locations and the team who finds it first, becomes defence, the team who does not has to go on the offensive.
 * The team who found the Intel source must now defend the point as the other team’s new goal is to disrupt their extraction process before it can be loaded.
 */
UCLASS()
class READYORNOT_API AIncriminationGS : public AReadyOrNotGameState
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Game State|Incrimination")
	class AIncriminationClue* GetClue(int32 ClueNumber, bool& bSuccess, bool bMustBeFound = false);

	UFUNCTION(BlueprintPure, Category = "Game State|Incrimination")
    TArray<class AIncriminationClue*> GetAllCluesOfNumber(int32 ClueNumber);

	UFUNCTION(BlueprintPure, Category = "Game State|Incrimination")
	bool DoesPlayerPossessAnyClue(APlayerCharacter* PlayerCharacter);

	UFUNCTION(BlueprintPure, Category = "Game State|Incrimination")
	bool AnyHigherCluesFound(int32 ClueNumber);
	
	UFUNCTION(BlueprintPure, Category = "Game State|Incrimination")
	bool AnyLowerCluesFound(int32 ClueNumber);
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEvidenceActorStateChanged, class AEvidenceActor*, EvidenceActor, EEvidenceActorState, NewEvidenceState, bool, bExtracted);
	UPROPERTY(BlueprintAssignable, Category = "Game State|Incrimination|Events")
	FOnEvidenceActorStateChanged OnIntelStateChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEvidenceSearchAreaChosen, class ASplineTrigger_Incrimination*, EvidenceSearchArea);
	UPROPERTY(BlueprintAssignable, Category = "Game State|Incrimination|Events")
	FOnEvidenceSearchAreaChosen OnIntelSearchAreaChosen;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEvidenceBuildingChosen, class ABuildingTrigger_Incrimination*, EvidenceSearchArea);
	UPROPERTY(BlueprintAssignable, Category = "Game State|Incrimination|Events")
    FOnEvidenceBuildingChosen OnIntelBuildingChosen;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActiveClueChanged, class AIncriminationClue*, ActiveClue);
	UPROPERTY(BlueprintAssignable, Category = "Game State|Incrimination|Events")
	FOnActiveClueChanged OnActiveClueChanged;

	UPROPERTY(BlueprintAssignable, Category = "Game State|Incrimination|Events")
	FOnActiveClueChanged OnPreviousActiveClueChanged;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCluesChanged, TArray<class AIncriminationClue*>, Clues);
	UPROPERTY(BlueprintAssignable, Category = "Game State|Incrimination|Events")
	FOnCluesChanged OnCluesChanged;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Game State|Incrimination")
	class AEvidenceSpawnPoint* ChosenEvidenceSpawn = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Game State|Incrimination")
	class AEvidenceActor* ChosenEvidenceActor = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Game State|Incrimination")
	class AEvidenceExtractionDevice_Incrim* ChosenExtractionDevice = nullptr;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_OnCluesChanged, Category = "Game State|Incrimination")
	TArray<class AIncriminationClue*> Clues;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Game State|Incrimination")
    TArray<class AIncriminationClueSpawnPoint*> ClueSpawnPoints;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_OnActiveClueChanged, Category = "Game State|Incrimination")
	class AIncriminationClue* ActiveClue = nullptr;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_OnPreviousActiveClueChanged, Category = "Game State|Incrimination")
	class AIncriminationClue* PreviousActiveClue = nullptr;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_OnIntelSearchAreaChosen, Category = "Game State|Incrimination")
	class ASplineTrigger_Incrimination* ChosenEvidenceSearchArea = nullptr;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_OnIntelBuildingChosen, Category = "Game State|Incrimination")
	class ABuildingTrigger_Incrimination* ChosenEvidenceBuilding = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Game State|Incrimination")
	TArray<class ASplineTrigger_Incrimination*> NonMainIntelSearchZones;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Game State|Incrimination")
	class AEvidenceExtractionDevice* CurrentExtractionDevice = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Game State|Incrimination")
	ETeamType PickupTeam = ETeamType::TT_NONE;
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_OnIntelStateChanged, Category = "Game State|Incrimination")
	EEvidenceActorState IntelState = EEvidenceActorState::Unclaimed;
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_OnIntelStateChanged, Category = "Game State|Incrimination")
	bool bIntelExtracted = false;
	
	UFUNCTION()
	void OnRep_OnIntelStateChanged();
	
	UFUNCTION()
	void OnRep_OnIntelSearchAreaChosen();

	UFUNCTION()
	void OnRep_OnIntelBuildingChosen();
	
	UFUNCTION()
	void OnRep_OnActiveClueChanged();
	
	UFUNCTION()
	void OnRep_OnPreviousActiveClueChanged();
	
	UFUNCTION()
	void OnRep_OnCluesChanged();
	
protected:
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

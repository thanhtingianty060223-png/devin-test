// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "GameModes/CoopGM.h"
#include "CommanderGM.generated.h"

class UCommanderProfile;
class URosterManager;

USTRUCT(BlueprintType)
struct READYORNOT_API FExfiltrationData
{
	GENERATED_BODY()

	FExfiltrationData() {}
	FExfiltrationData(bool bExfilMission, bool bBombThreat, bool bShooter)
		: bExfiltratedMission(bExfilMission), bActiveBombThreat(bBombThreat), bActiveShooter(bShooter)
	{
	}

	// Did we exfiltrate the mission? (Exfil portal or return to station/mainmenu)
	bool bExfiltratedMission = false;
	// Did we exfil while there was an active bomb threat?
	bool bActiveBombThreat = false;
	// Did we exfil while there was an active shooter?
	bool bActiveShooter = false;	
};

UCLASS()
class READYORNOT_API ACommanderGM : public ACoopGM
{
	GENERATED_BODY()

public:
	ACommanderGM();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	virtual bool AreAllPlayersDead() override;
	
	virtual void ReturnToStation() override;

	UPROPERTY()
	UCommanderProfile* CommanderProfile;

	UPROPERTY()
	URosterManager* RosterManager;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AExfilPortal> ExfilPortalClass;
	
protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void ExfiltrateMission(TArray<ASWATCharacter*> ExfilCharacters) override;

	virtual void StartMatch() override;

	virtual void OnMissionCompleted() override;
	virtual void CheckProgression(TSet<FName>& InProgressionTags) override;
	void CheckFlawlessIronman(TSet<FName>& InProgressionTags);
	
	virtual void CreateMatchEndWidgets(AReadyOrNotPlayerController* PlayerController) override;

	virtual void PlayerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;
	virtual void FriendlyAIKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;
	virtual void AIKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;
	virtual void AIIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter) override;
	virtual void AIArrested(AReadyOrNotCharacter* ArrestedCharacter, AReadyOrNotCharacter* InstigatorCharacter) override;
	
	virtual void SuspectKilledOrIncapacitated(AReadyOrNotCharacter* KilledCharacter, AReadyOrNotCharacter* InstigatorCharacter);
	virtual void CheckPlayerGenocide(AReadyOrNotCharacter* KilledCharacter, AReadyOrNotCharacter* InstigatorCharacter);
	
	virtual void SetupOfficerCustomization(ASWATCharacter* Character, FSavedCustomization& OutCustomization) override;

	/** If there are bombs on the map that have yet to be defused */
	bool HasActiveBombThreat() const;
	/** If there is an active shooter still alive/not arrested on the map*/
	bool HasActiveShooter() const;
	
	float AudioStartTime = 0.0f;
};

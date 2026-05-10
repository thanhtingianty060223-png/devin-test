// Copyright Void Interactive, 2024

#pragma once

#include "GameFramework/Actor.h"
#include "AIFactionManager.generated.h"

UENUM(BlueprintType)
enum class EAITeamTactic : uint8
{
	None,
	Suppress,
	Cover,
	Flank,
	Push,
	Charge,
	Custom
};

USTRUCT(BlueprintType)
struct FFactionSuspectTeam
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite)
	TArray<class ASuspectCharacter*> Suspects;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite)
	TArray<EAITeamTactic> Tactics;
};

USTRUCT(BlueprintType)
struct FFactionTeamTactics
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<EAITeamTactic> Tactics;
};

USTRUCT()
struct FAIFactionTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	FString Name = "None";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	TSubclassOf<AAIFactionManager> Manager = nullptr;

	/*
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	FString UnarmedMoveStyleName = "None";
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	FString ArmedMoveStyleName = "None";
	*/
};

UCLASS(Abstract, Transient, BlueprintType, Blueprintable, NotPlaceable, HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API AAIFactionManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AAIFactionManager();

	void AddCharacter(ACyberneticCharacter* Character);

	void OnAllAISpawned();

	UFUNCTION(BlueprintPure)
	FORCEINLINE TArray<ACyberneticCharacter*> GetAllCharacters() const { return Characters; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE TArray<ACyberneticCharacter*> GetAllLeaders() const { return Leaders; }

	UFUNCTION(BlueprintPure)
	TArray<ASuspectCharacter*> GetAllSuspects() const;
	
	UFUNCTION(BlueprintPure)
	TArray<ACivilianCharacter*> GetAllCivilians() const;

	FString GetFactionDebugInfo(ACyberneticCharacter* Character) const;
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void OnAIAdded(ACyberneticCharacter* Character);

	UFUNCTION()
	virtual void OnAISpottedEnemy(ACyberneticCharacter* Spotter, AReadyOrNotCharacter* Enemy);
	UFUNCTION()
	virtual void OnAISpottedFriendly(ACyberneticCharacter* Spotter, AReadyOrNotCharacter* Friendly);
	UFUNCTION()
	virtual void OnAISpottedNeutral(ACyberneticCharacter* Spotter, AReadyOrNotCharacter* Neutral);

	void InitializeSuspectTeams();
	void TryFindTeam(TArray<class ASuspectCharacter*> InSuspects);

	UFUNCTION(BlueprintPure)
	bool IsSuspectInTeam(class ASuspectCharacter* InSuspect, FFactionSuspectTeam& OutTeam) const;
	UFUNCTION(BlueprintPure)
	bool AreTeamSpotsAvailable(int32& OutIndex) const;
	UFUNCTION(BlueprintPure)
	bool IsTeamFull(const FFactionSuspectTeam& InTeam) const;

	UFUNCTION(BlueprintPure)
	class ASuspectCharacter* FindClosestSuspect(const TArray<class ASuspectCharacter*>& OtherSuspects, class ASuspectCharacter* Suspect, float MaxDistance = 1500.0f) const;

	UFUNCTION(BlueprintPure)
	bool GetSuspectsInTeam(class ASuspectCharacter* InSuspect, TArray<class ASuspectCharacter*>& OutSuspects, bool bIncludeSelf = false);

	UFUNCTION(BlueprintPure)
	bool GetTeamTacticFor(class ASuspectCharacter* InSuspect, EAITeamTactic& OutTactic) const;
	
	UFUNCTION(BlueprintPure)
	int32 GetTeamIndex(class ASuspectCharacter* InSuspect) const;
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On AI Spotted Enemy")
	void OnAISpottedEnemy_Blueprint(ACyberneticCharacter* Spotter, AReadyOrNotCharacter* Enemy);
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On AI Spotted Friendly")
	void OnAISpottedFriendly_Blueprint(ACyberneticCharacter* Spotter, AReadyOrNotCharacter* Friendly);
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On AI Spotted Neutral")
	void OnAISpottedNeutral_Blueprint(ACyberneticCharacter* Spotter, AReadyOrNotCharacter* Neutral);
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On AI Added")
	void OnAIAdded_Blueprint(ACyberneticCharacter* Character);

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On All AI Spawned")
	void OnAllAISpawned_Blueprint();

	void AlertOtherSuspectsInTeam(class ASuspectCharacter* Suspect, class AReadyOrNotCharacter* Enemy);
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Alert Other Suspects In Team")
	void AlertOtherSuspectsInTeam_Blueprint(class ASuspectCharacter* Suspect, class AReadyOrNotCharacter* Enemy);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Suspect Teams")
	uint8 bGroupIntoTeams : 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Suspect Teams", meta = (ClampMin = 2, EditCondition = "bGroupIntoTeams"))
	int32 TeamsOf = 2;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Suspect Teams", meta = (EditCondition = "bGroupIntoTeams"))
	uint8 bAssignRandomTeamTactics : 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Suspect Teams", meta = (EditCondition = "bGroupIntoTeams && !bAssignRandomTeamTactics"))
	TArray<FFactionTeamTactics> TacticsPool;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Suspect Teams")
	TArray<FFactionSuspectTeam> SuspectTeams;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status")
	TArray<ACyberneticCharacter*> Characters;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status")
	TArray<ACyberneticCharacter*> Leaders;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TeamBehaviourStrengthReductionSpeed = 1.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float TeamBehaviourOverrideStrength = 0.0f;
	
	#if !UE_BUILD_SHIPPING
	FColor DebugColor = FColor::White;
	#endif
};

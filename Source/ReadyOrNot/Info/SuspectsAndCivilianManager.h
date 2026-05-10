// Copyright Void Interactive, 2021

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "SuspectsAndCivilianManager.generated.h"

DECLARE_STATS_GROUP(TEXT("SuspectsAndCivilianManager"), STATGROUP_SuspectsAndCiviliansManager, STATCAT_Advanced);

UCLASS()
class READYORNOT_API USuspectsAndCivilianManager final : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override;

	UPROPERTY()
	TArray<class ATrapActor*> InvestigatedTrap;
	
public:
	static USuspectsAndCivilianManager* Get(const UObject* WorldContext);

	bool CanInvestigate();
	void StartedInvestigating();

	bool CanInvestigateTrap(class ATrapActor* Trap);
	void StartedInvestigatingTrap(class ATrapActor* Trap);

	UFUNCTION(BlueprintPure)
	TArray<ACyberneticCharacter*> GetAllSuspectsAndCivilians() const;

	int32 GetNumPerformingWorldBuilding() const;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<ACyberneticCharacter*> Suspects;
	UPROPERTY(BlueprintReadOnly)
	TArray<ACyberneticCharacter*> Civilians;

	void PlayRandomIdleLine();

	UPROPERTY(BlueprintReadOnly)
	float TimeSinceLastInvestigation = FLT_MAX;

	float TimeSinceIdleVO = 0.0f;
	float TimeSinceIdleVO_MirrorGun = 0.0f;

	// provide just the speech row (ie. for [Bark]MovingToCover or [Reply]MovingToCover or [Tell]MovingToCover just provide MovingToCover)
	void PlayBarkOrStartConversation(FString SpeechRow, ACyberneticCharacter* Speaker);

	void PlayBarkOrStartConversation(FString SpeechRow, FVector Location);

	void TriggerSharedBarkOrConversation(FString& SpeechRow, ACyberneticCharacter* Speaker, float Cooldown = 3.0f);
	
protected:
	UPROPERTY()
	TMap<FString, float> SpeechCooldownMap;

	UFUNCTION(Server, Reliable)
	void Server_PlaySharedBarkOrStartConversation(const FString& SpeechRow, ACyberneticCharacter* Speaker, float Cooldown = 3.0f);
	void Server_PlaySharedBarkOrStartConversation_Implementation(const FString& SpeechRow, ACyberneticCharacter* Speaker, float Cooldown = 3.0f);
};

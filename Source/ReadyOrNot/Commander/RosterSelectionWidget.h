// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "RosterManager.h"
#include "RosterSelectionWidget.generated.h"

class URosterCharacter;
class URosterManager;
class UCommanderProfile;

USTRUCT(BlueprintType)
struct READYORNOT_API FRosterActiveTraitInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	URosterTrait* Trait;

	UPROPERTY(BlueprintReadOnly)
	int32 Count;

	UPROPERTY(BlueprintReadOnly)
	float Value;
};

/**
 * 
 */
UCLASS()
class READYORNOT_API URosterSelectionWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable)
	void RefreshRoster();
	
	UFUNCTION(BlueprintImplementableEvent)
	void RefreshRosterEvent();
	
	UFUNCTION(BlueprintCallable)
	TArray<URosterCharacter*> GetAllCharacters();

	UFUNCTION(BlueprintCallable)
	TMap<ERosterSquadPosition, URosterCharacter*> GetSquadCharacters();

	UFUNCTION(BlueprintCallable)
	TArray<URosterCharacter*> GetRecruitableCharacters();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetCurrentRosterSize() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetMaximumRosterSize() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<int32> GetUnlockableSlotMissionsRemaining() const;
	
	UFUNCTION(BlueprintCallable)
	FRosterLoadout GetCharacterLoadout(URosterCharacter* Character);
	
	UFUNCTION(BlueprintCallable)
	void SetSquadCharacter(URosterCharacter* Character, ERosterSquadPosition Position);

	UFUNCTION(BlueprintCallable)
	void RecruitCharacter(URosterCharacter* Character);
	
	UFUNCTION(BlueprintCallable)
	void SendCharacterToTherapist(URosterCharacter* Character);

	UFUNCTION(BlueprintCallable)
	void FireCharacter(URosterCharacter* Character);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool CanUseTherapist(URosterCharacter* Character);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetNumCharactersInTherapy();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetMaxCharactersInTherapy();
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool CanFireCharacter(URosterCharacter* Character);
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FRosterActiveTraitInfo> GetActiveTraits() const;

	UFUNCTION(BlueprintImplementableEvent)
	void InitializeRoster();
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void OnCharactersUpdated();

	DECLARE_MULTICAST_DELEGATE(FOnRosterSelectionOpened);
	static FOnRosterSelectionOpened OnRosterSelectionOpened;

	DECLARE_MULTICAST_DELEGATE(FOnRosterSelectionClosed);
	static FOnRosterSelectionClosed OnRosterSelectionClosed;

	static void UpdateAllRosterWidgets(UWorld* World);
	
private:
	UPROPERTY()
	UCommanderProfile* CommanderProfile;
	
	UPROPERTY()
	URosterManager* RosterManager;
};

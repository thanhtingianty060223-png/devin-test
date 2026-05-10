// Copyright Void Interactive, 2021

#pragma once

#include "Structs.h"
#include "Info/Activities/BaseActivity.h"
#include "TeamBaseActivity.generated.h"

UCLASS(Abstract)
class READYORNOT_API UTeamBaseActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UTeamBaseActivity();

protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual bool CanFinishActivity() const override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual void ResetData() override;

	virtual float GetMoveAcceptanceRadiusOverride() const override;

public:
	virtual bool OverrideAvoidanceLocation() const;
	virtual FVector GetBestAvoidanceLocation(ACyberneticCharacter* OverlappingAI) const;

	virtual bool CanShoot() const override;

	virtual void SwapSquadPositionWith(ESquadPosition SquadPosition, bool bLeadInitiated = false);
	virtual bool CanSwapSquadPositions() const;
	
	virtual bool ShouldForceNoStrafe() const override;

	bool IsAnyoneSwapping() const;

	UFUNCTION(BlueprintPure)
	bool HasTeamReachedPosition(float Tolerance = 0.0f) const;
	
	UFUNCTION(BlueprintPure)
	class AReadyOrNotCharacter* GetSquadLeader() const;
	
	UFUNCTION(BlueprintPure)
	ACyberneticCharacter* GetCharacterAtSquadPosition(ESquadPosition SquadPosition) const;
	UFUNCTION(BlueprintPure)
	ACyberneticCharacter* GetCharacterWithItem(TSubclassOf<ABaseItem> ItemClass) const;
	UFUNCTION(BlueprintPure)
	ACyberneticCharacter* GetCharacterClosestToLocation(const FVector& TestLocation) const;
	UFUNCTION(BlueprintPure)
	ACyberneticCharacter* GetCharacterClosestToCharacter(ACyberneticCharacter* InCharacter) const;

	UPROPERTY(BlueprintReadOnly)
	ESquadPosition OverrideSquadPosition = ESquadPosition::SP_NONE;
	UPROPERTY(BlueprintReadOnly)
	ESquadPosition PreviousSquadPosition = ESquadPosition::SP_NONE;

	UPROPERTY(BlueprintReadOnly)
	bool bIsSwapping = false;

	FSharedTeamData* SharedData = nullptr;

	template<typename T>
	T* GetSharedData() const;
};

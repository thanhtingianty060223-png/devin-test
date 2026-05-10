// Copyright Void Interactive, 2021

#pragma once

#include "Info/Activities/Team/TeamBaseActivity.h"
#include "Info/Activities/ActivityManagerTemplates.h" // Clang is picky about this one
#include "TeamStackUpActivity.generated.h"

UCLASS()
class READYORNOT_API UTeamStackUpActivity : public UTeamBaseActivity
{
	GENERATED_BODY()

public:
	UTeamStackUpActivity();

	virtual bool GetOverrideMovementSpeed(float& OutMovementSpeed) const override;

	virtual void ResumeActivity() override;
	virtual bool ShouldForceStrafe() const override;
	virtual bool ShouldForceNoStrafe() const override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual void GatherDebugString(FString& OutString) override;
	virtual void ResetData() override;

	virtual float GetDestinationTolerance() const override;
	
	virtual bool CanBePushed() const override;

	virtual void SwapSquadPositionTo(ESquadPosition SquadPosition, bool bSameSide = false);
	virtual void SwapSquadPositionWith(ESquadPosition SquadPosition, bool bLeadInitiated = false) override;
	virtual void SwapSquadPositionWith(ESquadPosition SquadPosition, bool bOtherSide, bool bLeadInitiated = false);
	virtual bool CanSwapSquadPositions() const override;

	virtual bool GetLeanOverride(float& LeanOverride) const override;
	virtual bool GetLowReadyOverride(bool& bLowReady) const override;

	virtual bool ShouldDisableMoveRequest() const override;

	virtual void Transfer(UTeamStackUpActivity* OtherActivity);

	void CalculateStackUpPosition();

	FORCEINLINE EStackupGenArea GetStackUpArea() const { return ChosenStackUpArea; }
	FORCEINLINE ADoor* GetStackUpDoor() const { return GetSharedData<FSharedStackUpData>() ? GetSharedData<FSharedStackUpData>()->StackUpDoor : nullptr; }
	
	UFUNCTION(BlueprintPure)
	ACyberneticCharacter* GetCharacterAtSquadPositionInStackUpArea(ESquadPosition SquadPosition, EStackupGenArea StackUpArea) const;
	
	UFUNCTION(BlueprintPure)
	ACyberneticCharacter* GetCharacterAtHighestSquadPositionInStackUpArea(EStackupGenArea StackUpArea) const;

	UPROPERTY()
	class AStackUpActor* OccupiedStackUpActor = nullptr;
	
protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	#if !UE_BUILD_SHIPPING
	virtual void PerformActivity_Debug(float DeltaTime) override;
	#endif
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void FinishedActivity_NoOwner(bool bSuccess) override;

	virtual void TryCollapse();
	virtual bool CanCollapse() const;
	
	virtual void OnAsyncPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath) override;
	
	bool AllStacked(bool bLocationCheck = false) const;
	bool AllStackUpPathsReady() const;

	void MoveToOriginalLocation();
	
	void FindStackUpPath();
	void ReSortExistingSwat();

	bool IsLeaderOccupyingSquadPosition(ESquadPosition Position, EStackupGenArea StackUpArea = EStackupGenArea::SGA_All) const;

	bool IsFurthestOccupiedStackUpInArea() const;
	
	virtual void OnSwatSorted(const TArray<ASWATCharacter*>& InSortedSwat, bool bReversed);

	virtual bool ShouldCloseDoorWhenStackingUp();

	virtual void AbortActivityOnPathNotFound() override;

	FVector GetDoorFocalPoint() const;
	
	ACyberneticCharacter* FindChecker() const;
	ESquadPosition GetSquadPositionForCharacter(const ACyberneticCharacter* InSwatCharacter) const;
	TArray<UTeamStackUpActivity*> GetTotalSwatInStackUpArea(EStackupGenArea StackUpArea) const;
	
	UFUNCTION()
	virtual void PerformStackUpStage(float DeltaTime, float Uptime);
	UFUNCTION()
	virtual void PerformCheckStage(float DeltaTime, float Uptime);
	UFUNCTION()
	virtual void EnterStackedStage();
	UFUNCTION()
	virtual void PerformStackedStage(float DeltaTime, float Uptime);
	
	UFUNCTION()
	virtual void EnterStackupStage();
	UFUNCTION()
	virtual void ExitStackupStage();
	UFUNCTION()
	virtual void EnterCheckStage();
	UFUNCTION()
	virtual void ExitCheckStage();

	UFUNCTION()
	virtual void OnDoorChecked();
	
	UFUNCTION()
	virtual bool IsCheckFinished() const;
	UFUNCTION()
	virtual bool CanPerformCheck() const;

	UFUNCTION()
	void OnDoorOpened();

	bool DoesStackUpPathGoThroughDoor() const;

	bool IsCommandFrontOfDoor() const;
	
	bool bPreviousAllPositionsSameLevelAsAlpha = false;
	bool bHigherPositionSameDepth = false;
	bool bDontCalculateStackUp = false;
	bool bFoundStackUpPath = false;
	bool bIsCollapsing = false;

	FVector StubLocation = FVector::ZeroVector;
	
	uint32 StackUpAsyncPathId;
	float StackUpPathLength = 0.0f;
	TArray<FNavPathPoint> StackUpPath;

	EStackUpStyle StackUpStyleOverride = EStackUpStyle::Auto;

	EStackupGenArea PreviousStackUpArea = EStackupGenArea::SGA_None;
	EStackupGenArea ChosenStackUpArea = EStackupGenArea::SGA_None;
};

// Copyright Void Interactive, 2021

#pragma once

#include "TeamStackUpActivity.h"
#include "TeamBreachAndClearActivity.generated.h"

UCLASS()
class READYORNOT_API UTeamBreachAndClearActivity final : public UTeamStackUpActivity
{
	GENERATED_BODY()

public:
	UTeamBreachAndClearActivity();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBreachAndClearComplete, UTeamBreachAndClearActivity*, Activity, bool, bAuto);
	UPROPERTY(BlueprintAssignable)
	FBreachAndClearComplete OnCleared;

	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	#if !UE_BUILD_SHIPPING
	virtual void PerformActivity_Debug(float DeltaTime) override;
	#endif
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void ActivityOverriden(UBaseActivity* OverridingActivity) override;

	virtual bool CanTick() const override;

	virtual void ResumeActivity() override;

	virtual bool CanBePushed() const override;
	
	virtual void ResetData() override;

	virtual float GetDestinationTolerance() const override;

	virtual TSubclassOf<UNavigationQueryFilter> GetNavigationQueryOverride() override;
	
	virtual bool GetOverrideMovementSpeed(float& OutMovementSpeed) const override;
	virtual bool CanSwapSquadPositions() const override;

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual bool ShouldForceStrafe() const override;
	virtual bool ShouldForceNoStrafe() const override;
	
	virtual bool GetLeanOverride(float& LeanOverride) const override;
	virtual bool GetLowReadyOverride(bool& bLowReady) const override;

	bool IsBreaching() const;
	
	UFUNCTION()
	void OnDoorBreachActivityFinished(UBaseActivity* InActivity, ACyberneticController* InController);
	UFUNCTION()
	void OnDoorBreachFinished(UBaseActivity* InActivity, ACyberneticController* InController);

	FORCEINLINE bool HasPassedThreshold() const { return bHasEverPassedThreshold; }
	FORCEINLINE bool HasBreached() const { return GetSharedData<FSharedBreachData>()->bHasBreacherBreached; }
	FORCEINLINE float GetBreachTime() const { return GetSharedData<FSharedBreachData>()->ClearingTime; }

	void SortSwatForClearing();

	UFUNCTION()
	bool IsFinishedClearing() const;

	virtual void GatherDebugString(FString& OutString) override;
	
protected:
	virtual bool CanCollapse() const override;

	virtual bool ShouldCloseDoorWhenStackingUp() override;
	
	void TryMoveToCurrentStage();

	bool GiveDoorBreachActivity(UDoorBreachActivity* InDoorBreachActivity, ABaseItem* InBreachItem);

	bool CanUseDoor() const;

	virtual void PerformStackedStage(float DeltaTime, float Uptime) override;

	UFUNCTION()
	void OnLeaderItemPrimaryUse(AReadyOrNotCharacter* ItemOwner, ABaseItem* Item);
	
	UFUNCTION()
	void OnLeaderGrenadeDetonated(ABaseGrenade* InGrenade);
	
	UFUNCTION()
	void OnLeaderGrenadeProjectileDetonated(class AGrenadeProjectile* InGrenadeProjectile);

	UFUNCTION()
	void OnDoorBreacherReady();
	
	UFUNCTION()
	void OnDoorBreacherBreaching();
	
	UFUNCTION()
	bool CanPerformBreach() const;

	UFUNCTION()
	bool CanPerformClear() const;

	UFUNCTION()
	bool ShouldScan() const;

	virtual bool IsCheckFinished() const override;

	// Finds a character across the doorway from the door user to be the new breacher. Can return nullptr if there are no characters across the door from the door user.
	ACyberneticCharacter* FindBreacher(ACyberneticCharacter* InDoorUser) const;
	ACyberneticCharacter* FindUser() const;

	bool ShouldCheckDoorBeforeBreach() const;
	
	bool IsDoorBreacher() const;
	bool IsDoorUser() const;
	bool IsDoorScanner() const;

	bool HasLeaderPassedThreshold() const;

	bool AnySwatOnOtherSide(uint8* Num = nullptr) const;

	virtual void RequestMoveFromPath(const FVector& InLocation, FNavPathSharedPtr NavPath) override;
	
private:
	void SetupDoorUsers();

	void TryActivateDoorBlocker(FRoom* Room);
	void TryDeactivateDoorBlocker(FRoom* Room);
	void TryActivateBreachBlockers();
	
	virtual bool CanPerformCheck() const override;

	virtual void PerformCheckStage(float DeltaTime, float Uptime) override;
	
	virtual void EnterStackupStage() override;
	virtual void EnterStackedStage() override;
	
	virtual void OnSwatSorted(const TArray<ASWATCharacter*>& InSortedSwat, bool bReversed) override;
	
	UFUNCTION()
	void PerformBreachStage(float DeltaTime, float Uptime);
	UFUNCTION()
	void PerformClearStage(float DeltaTime, float Uptime);
	UFUNCTION()
	void PerformScanStage(float DeltaTime, float Uptime);

	UFUNCTION()
	void EnterBreachStage();
	UFUNCTION()
	void EnterClearStage();
	UFUNCTION()
	void EnterClearedStage();
	
	UFUNCTION()
	void EnterScanStage();

	void GiveScanActivity(EThresholdAssessment Assessment);
	
	UFUNCTION()
	bool IsScanFinished() const;

	UFUNCTION()
	void OnDoorScanFinished(UBaseActivity* Activity, ACyberneticController* Controller);

	void OnDoorScanFinished_Internal();

	void TryDropChemlight();

	bool AnyUnclearedLandmarksForClearPoint(const FClearPoint* ClearPoint) const;

	float GetPathDistanceToNextPoint() const;

	void LeaderCallBreachDone();
	
	void TryCalloutLeftOpening(ADoor* TargetDoor, FVector Point);
	void TryCalloutRightOpening(ADoor* TargetDoor, FVector Point);

	FVector CornerFocalPoint = FVector::ZeroVector;

	float LocalClearingTime = 0.0f;
	float TimeSinceLastClearingTick = 0.0f;
	
	bool bHasEverPassedThreshold = false;
	bool bHasLeaderEverPassedThreshold = false;
	bool bDroppedChemlightHalfway = false;
	bool bHasLOSToClearPoint = false;
	bool bHasLetGoOfStackUp = false;
	bool bHasEverEquippedShieldWhileClearing = false;
	bool bHasTrackedAnyoneWhileClearing = false;
	bool bNextStageIsOccupied = false;
	bool bCalledOutAreaClear = false;
	
	uint8 CurrentClearStage = 0;
	uint8 StageLimit = 0;
	
	TArray<FClearPoint>* ChosenClearPoints = nullptr;
	FClearPoint* CurrentClearPoint = nullptr;

	UPROPERTY()
	ACyberneticCharacter* AIBlockingClearingPath = nullptr;

	UPROPERTY()
	ACyberneticCharacter* ClearingLeader = nullptr;
	
	UPROPERTY()
	class AThreatAwarenessActor* NearestThreat = nullptr;

	FCollisionQueryParams QueryParameters;
};

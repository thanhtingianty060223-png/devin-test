// Void Interactive, 2020

#pragma once

#include "BaseCombatMoveActivity.h"
#include "CoverData.h"
#include "Info/Activities/TakeCoverActivity.h"
#include "Info/Activities/Tasks/FindCoverTask.h"
#include "HardCoverCombatMove.generated.h"

UCLASS()
class READYORNOT_API UHardCoverCombatMove : public UBaseCombatMoveActivity
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCoverFoundDelegate);
	FCoverFoundDelegate NewCoverFound;
	FCoverFoundDelegate NoCoverFound;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCoverEventDelegate);
	FCoverEventDelegate OnCoverExit;
	FCoverEventDelegate OnRequestCover;
	FCoverEventDelegate OnRequestCoverLandmark;
	FCoverEventDelegate OnCoverLandmarkExit;

	bool TryMoveIntoCover(const FCoverInstigatorStimulus& InstigatorStimulus, bool bRequireLOS = true);
	
	bool TryMoveIntoCoverLandmark(const FVector& ThreatLocation, const FVector& ThreatDirection, float MinDistanceFromInstigator = 0.0f, AReadyOrNotCharacter* InstigatorCharacter = nullptr);
	bool TryMoveIntoCoverLandmark(float MinDistanceFromInstigator = 0.0f, AReadyOrNotCharacter* InstigatorCharacter = nullptr);

	bool RequestCover(const FCoverInstigatorStimulus& InstigatorStimulus);

	EAbortCoverReason GetLastAbortCoverReason() const;
	
	virtual bool ShouldForceStrafe() const override;
	virtual bool ShouldForceNoStrafe() const override;
	virtual bool CanShoot() const override;
	
	virtual void StartActivity(AAIController* Owner) override;

	virtual void GiveTakeCoverActivity(const FCoverData& InCoverData);
	virtual void GiveTakeCoverAtLandmarkActivity(class ACoverLandmark* InCoverLandmark);

	bool IsRequestingCover() const;

	bool IsMovingToCover() const;
	void AbortCoverNow();

	virtual void OnMoveInterrupted(UBaseActivity* Activity) override;
	
	#if !UE_BUILD_SHIPPING
	virtual void GatherDebugString(FString& OutString) override;
	#endif

	virtual FName GetMoveStyleOverride_Implementation() const override;

protected:
	virtual void RequestCombatMove(float DeltaTime) override;
	virtual void FinishedActivity(bool bSuccess) override;
	
	virtual void OnKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;

	TArray<class ACoverLandmark*> FindCoverLandmarksInArea(const FVector& InLocation, const FVector& InExtent) const;

	UFUNCTION()
	void TrackNewEnemy(AReadyOrNotCharacter* NewTrackedEnemy);
	
	void ResetCoverData();
	
	void OnNewCoverFoundAsync(uint32 NumCoverFound, float TimeMs);
	void OnNoCoverFoundAsync(uint32 NumCoverFound, float TimeMs);
	
	virtual void OnCoverFound(const FCoverData& InCoverData);

	UFUNCTION()
	void OnTakeCoverActivityFinished(UBaseActivity* Activity, ACyberneticController* Controller);
	UFUNCTION()
	void OnTakeCoverAtLandmarkActivityFinished(UBaseActivity* Activity, ACyberneticController* Controller);
	
	FAutoDeleteAsyncTask<FFindCoverTask>* CurrentFindCoverTask = nullptr;
	TSharedPtr<FCoverData> CurrentFindCoverData;
	
	UPROPERTY()
	class UTakeCoverActivity* TakeCoverActivity = nullptr;
	
	UPROPERTY()
	class UTakeCoverAtLandmarkActivity* TakeCoverAtLandmarkActivity = nullptr;

	FCoverData LastBestCover;
	FFindCoverQuery LastFindCoverQuery;

	const FCoverData* PendingNewCover = nullptr;

	UPROPERTY()
	AReadyOrNotCharacter* LastTrackedEnemy = nullptr;
	
	UPROPERTY()
	FCoverInstigatorStimulus LastCoverInstigatorStimulus;
	UPROPERTY()
	FCoverInstigatorStimulus PendingCoverInstigatorStimulus;
};

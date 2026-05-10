// Copyright Void Interactive, 2023

#pragma once

#include "Info/Activities/Team/TeamBaseActivity.h"
#include "SearchAndSecureActivity.generated.h"

UCLASS()
class READYORNOT_API USearchAndSecureActivity final : public UBaseActivity
{
	GENERATED_BODY()
	
public:
	USearchAndSecureActivity();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSearchComplete, USearchAndSecureActivity*, Activity, ADoor*, BreachedDoor);
	UPROPERTY(BlueprintAssignable)
	FSearchComplete OnSearchComplete;

	FVector CommandLocation = FVector::ZeroVector;
	ETeamType CommandTeam = ETeamType::TT_NONE;

	bool bAuto = false;

	UPROPERTY()
	ADoor* BreachDoor = nullptr;
	
	const FRoom* SearchingRoom = nullptr;

	bool bBreachedFromFront = false;
	bool bReturnToOriginalLocation = false;
	
	virtual void ResetData() override;

protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual bool CanFinishActivity() const override;
	virtual TSubclassOf<UNavigationQueryFilter> GetNavigationQueryOverride() override;

	void TrySecure();
	
	FVector OriginalLocation = FVector::ZeroVector;

	float SecureCooldown = 0.0f;
	float TimeSinceLastSecure = 0.0f;

	UPROPERTY()
	TArray<AActor*> AllSecurables;
	
	UPROPERTY()
	AActor* ClosestSecurable = nullptr;
};

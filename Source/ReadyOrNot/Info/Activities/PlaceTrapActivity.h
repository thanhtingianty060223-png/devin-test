// Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "Info/Activities/WorldBuildingActivity.h"
#include "PlaceTrapActivity.generated.h"

/**
 *	Place trap activity, handles trap placement for suspects
 */
UCLASS()
class READYORNOT_API UPlaceTrapActivity : public UWorldBuildingActivity
{
	GENERATED_BODY()

public:
	UPlaceTrapActivity();

	UPROPERTY()
	ADoor* Door = nullptr;

	FName TrapType = NAME_None;

protected:
	virtual void StartActivity(AAIController* Controller) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void FinishedActivity_NoOwner(bool bSuccess) override;
	virtual void ActivityOverriden(UBaseActivity* OverridingActivity) override;
	
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

	virtual float GetDestinationTolerance() const override;

	virtual bool CanShoot() const override;
	
	virtual bool ShouldForceStrafe() const override;
	virtual bool ShouldForceNoStrafe() const override;

	UFUNCTION()
	void OnTrapPlacementInterrupted();

	// Bind/unbind helpers for door events
	void BindEvents();
	void UnbindEvents();
};

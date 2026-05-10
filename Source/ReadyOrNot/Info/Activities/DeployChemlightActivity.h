// Void Interactive, 2020

#pragma once

#include "Team/DeployItemAtLocationActivity.h"
#include "DeployChemlightActivity.generated.h"

UCLASS()
class READYORNOT_API UDeployChemlightActivity : public UDeployItemAtLocationActivity
{
	GENERATED_BODY()

public:
	UDeployChemlightActivity();
	
protected:
	virtual void EnterDeployStage() override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void ActivityOverriden(UBaseActivity* OverridingActivity) override;

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

	virtual FString GetDeployAnimation() const override;

	UFUNCTION()
	void OnChemlightThrown(ABaseItem* Item);
};

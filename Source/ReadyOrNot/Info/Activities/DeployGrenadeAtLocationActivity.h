// Void Interactive, 2020

#pragma once

#include "Team/DeployItemAtLocationActivity.h"
#include "DeployGrenadeAtLocationActivity.generated.h"

UCLASS()
class READYORNOT_API UDeployGrenadeAtLocationActivity : public UDeployItemAtLocationActivity
{
	GENERATED_BODY()

public:
	UDeployGrenadeAtLocationActivity();
	
protected:
	virtual void EnterMoveToStage() override;
	virtual void TickMoveToStage(float DeltaTime, float Uptime) override;
	virtual void EnterDeployStage() override;
	virtual void TickDeployStage(float DeltaTime, float Uptime) override;

	virtual bool CanDeploy() const override;
	virtual FString GetDeployAnimation() const override;
	
private:
	void TryThrowGrenade(ABaseGrenade* Grenade, float DeltaTime, float Uptime);
};

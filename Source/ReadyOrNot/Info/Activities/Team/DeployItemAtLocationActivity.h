// Void Interactive, 2020

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "DeployItemAtLocationActivity.generated.h"

UCLASS()
class READYORNOT_API UDeployItemAtLocationActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UDeployItemAtLocationActivity();

	virtual bool CanFinishActivity() const override;
	virtual bool CanOverrideActivity() const override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	
	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<ABaseItem> DeployItemClass = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FVector DeployLocation = FVector::ZeroVector;

protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void FinishedActivity(bool bSuccess) override;
	
	UFUNCTION()
	virtual void EnterMoveToStage();
	UFUNCTION()
	virtual void TickMoveToStage(float DeltaTime, float Uptime);
	UFUNCTION()
	virtual void ExitMoveToStage();
	
	UFUNCTION()
	virtual bool CanDeploy() const;
	
	UFUNCTION()
	virtual void EnterDeployStage();
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Enter Deploy Stage")
	void Blueprint_EnterDeployStage();

	UFUNCTION()
	virtual void TickDeployStage(float DeltaTime, float Uptime);
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Tick Deploy Stage")
	void Blueprint_TickDeployStage(float DeltaTime, float Uptime);

	UFUNCTION()
	virtual void ExitDeployStage();
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Exit Deploy Stage")
	void Blueprint_ExitDeployStage();

	virtual bool CheckEdgeCases();

	virtual FString GetDeployAnimation() const;
	
	void PlayDeployAnim();
	void StopDeployAnim(float BlendOutTime = 0.25f);
};

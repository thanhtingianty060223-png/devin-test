// Void Interactive, 2020

#pragma once

#include "DoorInteractionActivity.h"
#include "DeployWedgeActivity.generated.h"

UCLASS()
class READYORNOT_API UDeployWedgeActivity : public UDoorInteractionActivity
{
	GENERATED_BODY()

public:
	UDeployWedgeActivity();

	uint8 bRemoveWedge : 1;
	
protected:
	virtual void OnInteractionBegin() override;
	virtual void OnInteractionEnd() override;
	
	virtual bool CanInteract() const override;
	virtual void EnterGetInPositionStage() override;
	virtual float GetInteractionDistance() const override;
	virtual FString GetInteractionAnimation() const override;
	virtual FVector GetInteractionLocation() const override;
	
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
	
	virtual bool CheckEdgeCases() override;
};

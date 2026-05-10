// Void Interactive, 2020

#pragma once

#include "Info/Activities/Team/DoorInteractionActivity.h"
#include "LockPickDoorActivity.generated.h"

UCLASS()
class READYORNOT_API ULockPickDoorActivity : public UDoorInteractionActivity
{
	GENERATED_BODY()
	
public:
	ULockPickDoorActivity();

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	
protected:
	virtual void EnterGetInPositionStage() override;

	virtual bool CheckEdgeCases() override;

	virtual void OnInteractionBegin() override;
	virtual void OnInteractionEnd() override;
	
	virtual bool CanInteract() const override;
	
	virtual float GetInteractionDistance() const override;
	virtual FString GetInteractionAnimation() const override;

	virtual void BindEvents() override;
	virtual void UnbindEvents() override;
};

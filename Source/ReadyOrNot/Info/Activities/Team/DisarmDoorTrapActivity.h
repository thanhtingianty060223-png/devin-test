// Copyright Void Interactive, 2021

#pragma once

#include "DoorInteractionActivity.h"
#include "DisarmDoorTrapActivity.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UDisarmDoorTrapActivity : public UDoorInteractionActivity
{
	GENERATED_BODY()

public:
	UDisarmDoorTrapActivity();

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

protected:
	virtual void FinishedActivity(bool bSuccess) override;

	virtual void OnInteractionBegin() override;
	virtual void OnInteractionEnd() override;

	virtual bool CanInteract() const override;

	virtual bool CanOverrideActivity() const override;
	
	virtual float GetInteractionDistance() const override;
	virtual FString GetInteractionAnimation() const override;
	virtual FVector GetInteractionLocation() const override;
	
	virtual void BindEvents() override;
	virtual void UnbindEvents() override;

	virtual bool CheckEdgeCases() override;
	
	virtual void EnterGetInPositionStage() override;
	virtual void EnterInteractStage() override;

	virtual void OnDoorOpened() override;

	bool IsDoorTrapLive() const;

	UFUNCTION()
	void OnTrapTriggered(class ATrapActor* Trap, AReadyOrNotCharacter* TriggeredBy);
};

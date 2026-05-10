// Void Interactive, 2020

#pragma once

#include "Team/DoorInteractionActivity.h"
#include "ToggleDoorActivity.generated.h"

UCLASS()
class READYORNOT_API UToggleDoorActivity final : public UDoorInteractionActivity
{
	GENERATED_BODY()

public:
	UToggleDoorActivity();

	uint8 bOpenDoor : 1;

protected:
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

	virtual bool CheckEdgeCases() override;
	
	virtual void EnterGetInPositionStage() override;
	virtual void PerformInteractStage(float DeltaTime, float Uptime) override;
	virtual void PerformGetInPositionStage(float DeltaTime, float Uptime) override;
	virtual void EnterInteractStage() override;

	virtual void OnDoorOpened() override;
	virtual void OnDoorClosed() override;
	virtual void OnDoorMovementBlocked() override;
	
	virtual float GetInteractionDistance() const override;
	virtual FVector GetInteractionLocation() const override;

	virtual bool CanInteract() const override;

private:
	void ToggleDoor();
};

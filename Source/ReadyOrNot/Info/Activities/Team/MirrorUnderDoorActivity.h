// Copyright Void Interactive, 2021

#pragma once

#include "DoorInteractionActivity.h"
#include "MirrorUnderDoorActivity.generated.h"

UENUM()
enum class EMirrorContactType : uint8
{
	AI,
	Trap,
	Custom,
	Both
};

UCLASS()
class READYORNOT_API UMirrorUnderDoorActivity : public UDoorInteractionActivity
{
	GENERATED_BODY()

public:
	UMirrorUnderDoorActivity();

	virtual void PerformActivity(float DeltaTime) override;

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

	UPROPERTY(BlueprintReadOnly)
	EMirrorContactType MirrorContactType = EMirrorContactType::AI;
	
protected:
	UPROPERTY()
	TArray<ACyberneticCharacter*> SpottedCharacters;
	
	virtual void EnterGetInPositionStage() override;
	virtual void EnterInteractStage() override;

	virtual void PerformInteractStage(float DeltaTime, float Uptime) override;
	
	virtual void OnInteractionBegin() override;
	virtual void OnInteractionEnd() override;

	virtual bool CanInteract() const override;
	
	virtual float GetInteractionDistance() const override;
	virtual FString GetInteractionAnimation() const override;

	virtual bool CheckEdgeCases() override;

	virtual void ResetData() override;

	void MirrorForAI();
	void MirrorForTrap();
	void MirrorForRooms();

	UFUNCTION(BlueprintImplementableEvent)
	void MirrorForCustom();

	virtual void BindEvents() override;
	virtual void UnbindEvents() override;

	bool bAttemptedRoomCallout = false;
	
	bool bCalledOutLeftOpening = false;
	bool bCalledOutRightOpening = false;
	bool bCalledOutFrontOpening = false;

	FRoom* MirroringRoom = nullptr;
};

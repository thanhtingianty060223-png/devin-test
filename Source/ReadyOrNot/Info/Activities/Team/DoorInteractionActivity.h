// Copyright Void Interactive, 2021

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "DoorInteractionActivity.generated.h"

/**
 * Base class for all interactions relating to a door
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class READYORNOT_API UDoorInteractionActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UDoorInteractionActivity();
	
	UPROPERTY(BlueprintReadOnly)
	class ADoor* Door = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FVector CommandLocation = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly)
	FVector OriginalLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	uint8 bReturnToPositionAfterInteraction : 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	uint8 bDisablePlayerDoorInteraction : 1;
	
	UPROPERTY(BlueprintReadOnly)
	FString InteractionAnimation = "";

	virtual bool GetOverrideMovementSpeed(float& OutMovementSpeed) const override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual bool ShouldForceStrafe() const override;
	virtual bool CanFinishActivity() const override;
	virtual bool CanOverrideActivity() const override;
	virtual bool CanBeOverridenBy(UBaseActivity* InOverridingActivity) override;

	virtual float GetDestinationTolerance() const override;

	virtual bool CanBePushed() const override;

protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void ResumeActivity() override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void FinishedActivity_NoOwner(bool bSuccess) override;
	virtual void ActivityOverriden(UBaseActivity* OverridingActivity) override;

	virtual float GetInteractionDistance() const;
	virtual FVector GetInteractionLocation() const;
	virtual FString GetInteractionAnimation() const;

	virtual bool IsLeftSideInteraction() const;
	
	virtual bool CheckEdgeCases();

	void EnableDoorInteractable();
	void DisableDoorInteractable();
	
	virtual void BindEvents();
	virtual void UnbindEvents();

	UFUNCTION()
	virtual void OnDoorOpened();
	UFUNCTION()
	virtual void OnDoorClosed();
	UFUNCTION()
	virtual void OnDoorBroken();
	UFUNCTION()
	virtual void OnDoorMovementBlocked();
	
	UFUNCTION()
	virtual void EnterGetInPositionStage();
	UFUNCTION()
	virtual void PerformGetInPositionStage(float DeltaTime, float Uptime);
	UFUNCTION()
	virtual void ExitGetInPositionStage();

	UFUNCTION()
	virtual bool ShouldGetInPosition() const;
	UFUNCTION()
	virtual bool CanInteract() const;

	UFUNCTION()
	virtual void EnterInteractStage();
	UFUNCTION()
	virtual void PerformInteractStage(float DeltaTime, float Uptime);
	UFUNCTION()
	virtual void ExitInteractStage();
	
	UFUNCTION()
	virtual void OnInteractionBegin();
	UFUNCTION()
	virtual void OnInteractionEnd();
	
	UFUNCTION()
	virtual void EnterReturnStage();
	UFUNCTION()
	virtual void ExitReturnStage();
	UFUNCTION()
	virtual void TickReturnStage(float DeltaTime, float Uptime);
	UFUNCTION()
	virtual bool CanReturn() const;

	virtual void OnInteractionAnimFinished();

	void PlayInteractionAnim(const FVector& InFocalPoint = FVector::ZeroVector);
	void StopInteractionAnim(float BlendOutTime = 0.25f);

	void RequestMoveToInteractLocation();

	UPROPERTY()
	UAnimMontage* InteractionAnimMontage = nullptr;
	
	uint8 bInteractionFinished : 1;
	uint8 bCanInteractWithDoorway : 1;
	
private:
	void BindEvents_Internal();
	void UnbindEvents_Internal();
	
	FTimerHandle TH_InteractionAnim;
};

// Void Interactive, 2020

#pragma once

#include "DoorInteractionActivity.h"
#include "DoorBreachActivity.generated.h"

UCLASS(Abstract, BlueprintType, NotBlueprintable)
class READYORNOT_API UDoorBreachActivity : public UDoorInteractionActivity
{
	GENERATED_BODY()

public:
	UDoorBreachActivity();

	// Optional breaching tool
	UPROPERTY(BlueprintReadOnly)
	ABaseItem* BreachItem = nullptr;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBreachFinished, UBaseActivity*, Activity, ACyberneticController*, Controller);
	UPROPERTY(BlueprintAssignable)
	FOnBreachFinished OnBreachFinished;
	
	UFUNCTION()
	virtual void FinishDoorBreach();
	
protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual bool CanFinishActivity() const override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual bool ShouldForceStrafe() const override;
	virtual void ResetData() override;
	
	virtual void OnDoorOpened() override;
	virtual void OnDoorBroken() override;
	virtual void OnDoorClosed() override;

	UFUNCTION()
	virtual void EnterBreachedStage();
	UFUNCTION()
	virtual void ExitBreachedStage();
	UFUNCTION()
	virtual void TickBreachedStage(float DeltaTime, float Uptime);

	UFUNCTION()
	virtual bool IsBreachFinished() const;
	virtual bool CanReturn() const override;
	
	UFUNCTION()
	virtual void OnBreacherKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);

private:
	uint8 bBreachFinished : 1;
};

UCLASS(BlueprintType, NotBlueprintable)
class READYORNOT_API UKickDoorActivity : public UDoorBreachActivity
{
	GENERATED_BODY()

public:
	UKickDoorActivity();

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

protected:
	virtual void EnterInteractStage() override;
	virtual void PerformInteractStage(float DeltaTime, float Uptime) override;

	virtual void FinishedActivity(bool bSuccess) override;
	
	virtual void PerformGetInPositionStage(float DeltaTime, float Uptime) override;
	virtual bool CanInteract() const override;
	
	virtual void ResetData() override;

	virtual bool CanFinishActivity() const override;
	virtual float GetDestinationTolerance() const override;

	virtual FString GetInteractionAnimation() const override;
	virtual float GetInteractionDistance() const override;
	virtual FVector GetInteractionLocation() const override;

	virtual bool CheckEdgeCases() override;

	virtual void OnDoorOpened() override;

	UFUNCTION()
	void OnDoorKicked();

	uint8 bHasPlayedKickResponseLine : 1;
};

UCLASS(BlueprintType, NotBlueprintable)
class READYORNOT_API UC2DoorActivity : public UDoorBreachActivity
{
	GENERATED_BODY()

public:
	UC2DoorActivity();

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	
protected:
	virtual bool ShouldGetInPosition() const override;
	
	virtual bool CanInteract() const override;
	
	virtual void ResetData() override;
	
	virtual void FinishedActivity(bool bSuccess) override;

	virtual bool CanFinishActivity() const override;

	virtual void EnterInteractStage() override;
	virtual void PerformInteractStage(float DeltaTime, float Uptime) override;
	virtual void ExitInteractStage() override;

	virtual FString GetInteractionAnimation() const override;
	virtual float GetInteractionDistance() const override;
	virtual FVector GetInteractionLocation() const override;

	virtual bool IsBreachFinished() const override;
	virtual bool CheckEdgeCases() override;
	
	UFUNCTION()
	void EnterDetonateC2Stage();
	UFUNCTION()
	void PerformDetonateC2Stage(float DeltaTime, float Uptime);
	
	UFUNCTION()
	bool CanDetonateC2() const;

	UFUNCTION()
	void OnC2Placed();
	
	UFUNCTION()
	void OnC2Detonate();

	void PlayC2BreachResponse();

	uint8 bHasEverDetonatedC2 : 1;

	FTimerHandle DelayFinishDoorUseTimer;
};

UCLASS(BlueprintType, NotBlueprintable)
class READYORNOT_API UShotgunDoorActivity : public UDoorBreachActivity
{
	GENERATED_BODY()

public:
	UShotgunDoorActivity();

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	
protected:
	virtual void FinishedActivity(bool bSuccess) override;

	void DestroyChunk(int32 ChunkIndex, float ImpulseForce, bool bCheckSupports);

	bool CanShotgunHinges() const;
	
	virtual void ResetData() override;

	virtual void EnterInteractStage() override;
	virtual void PerformInteractStage(float DeltaTime, float Uptime) override;

	virtual FString GetInteractionAnimation() const override;
	virtual float GetInteractionDistance() const override;
	virtual FVector GetInteractionLocation() const override;

	virtual void OnDoorOpened() override;
	
	UFUNCTION()
	void OnDoorShotgunned();
	UFUNCTION()
	void OnDoorKicked();

	uint8 bBreachedDoor : 1;
	uint8 bKickedDoor : 1;
};

UCLASS(BlueprintType, NotBlueprintable)
class READYORNOT_API URamDoorActivity : public UDoorBreachActivity
{
	GENERATED_BODY()

public:
	URamDoorActivity();

protected:
	virtual void FinishedActivity(bool bSuccess) override;

	virtual bool CheckEdgeCases() override;
	virtual void EnterInteractStage() override;
	virtual void PerformInteractStage(float DeltaTime, float Uptime) override;

	virtual void ResetData() override;

	virtual FString GetInteractionAnimation() const override;
	virtual float GetInteractionDistance() const override;
	virtual FVector GetInteractionLocation() const override;

	UFUNCTION()
	void OnDoorRammed();

	uint8 bHasPlayedRamResponseLine : 1;
};

UCLASS(BlueprintType, NotBlueprintable)
class READYORNOT_API ULaunchGrenadeThroughDoorActivity : public UDoorBreachActivity
{
	GENERATED_BODY()
	
public:
	ULaunchGrenadeThroughDoorActivity();
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLauncherReady);
	UPROPERTY(BlueprintAssignable)
	FOnLauncherReady OnLauncherReady;

protected:
	virtual void EnterGetInPositionStage() override;
	virtual FString GetInteractionAnimation() const override { return ""; }
	virtual FVector GetInteractionLocation() const override;
	virtual float GetInteractionDistance() const override;

	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;

	virtual void FinishedActivity(bool bSuccess) override;
	
	virtual void EnterInteractStage() override;
	virtual void PerformInteractStage(float DeltaTime, float Uptime) override;

	virtual bool GetLeanOverride(float& LeanOverride) const override;
	virtual bool GetLowReadyOverride(bool& bLowReady) const override;

	virtual void ResetData() override;

	float TimeDoorOpen = 0.0f;
	bool bFired = false;
};

UCLASS(Abstract, BlueprintType, NotBlueprintable)
class READYORNOT_API UThrowItemThroughDoorActivity : public UDoorBreachActivity
{
	GENERATED_BODY()

public:
	UThrowItemThroughDoorActivity();

	UPROPERTY(BlueprintReadOnly)
	TSubclassOf<ABaseItem> ThrowItemClass = nullptr;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnThrowReady);
	UPROPERTY(BlueprintAssignable)
	FOnThrowReady OnThrowReady;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPerformingThrow);
	UPROPERTY(BlueprintAssignable)
	FOnPerformingThrow OnThrowingItem;

	uint8 bWaitBeforeThrow : 1;

	void CalculateThrowTransform(const FTransform& InSocketTransform, FVector& ThrowLocation, FVector& ThrowDirection);

protected:
	virtual void FinishedActivity(bool bSuccess) override;
	virtual bool ShouldGetInPosition() const override;
	
	virtual void TickBreachedStage(float DeltaTime, float Uptime) override;

	virtual void TryThrowItem();

	bool CanThrowNow() const;

	virtual void ResetData() override;

	// Ready state
	virtual void PerformGetInPositionStage(float DeltaTime, float Uptime) override;

	virtual void OnReady();
	
	// Throw state
	virtual void EnterInteractStage() override;
	virtual void PerformInteractStage(float DeltaTime, float Uptime) override;
	
	virtual bool CanInteract() const override;

	UPROPERTY()
	ABaseItem* ThrownItem = nullptr;

	uint8 bItemThrown : 1;
};

UCLASS(BlueprintType, NotBlueprintable)
class READYORNOT_API UThrowGrenadeThroughDoorActivity : public UThrowItemThroughDoorActivity
{
	GENERATED_BODY()

public:
	UThrowGrenadeThroughDoorActivity();

protected:
	virtual void PerformInteractStage(float DeltaTime, float Uptime) override;
	virtual void FinishedActivity(bool bSuccess) override;

	virtual FString GetInteractionAnimation() const override;
};

UCLASS(Abstract, BlueprintType, Blueprintable)
class READYORNOT_API UCustomDoorBreachActivity : public UDoorBreachActivity
{
	GENERATED_BODY()

public:
	UCustomDoorBreachActivity();

protected:
	virtual void PerformActivity(float DeltaTime) override;
	
	UFUNCTION(BlueprintImplementableEvent)
	void TickBreachDoor(float DeltaTime);
};
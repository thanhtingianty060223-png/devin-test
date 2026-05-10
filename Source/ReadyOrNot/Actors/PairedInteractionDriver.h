// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "PairedInteractionDriver.generated.h"

UCLASS()
class READYORNOT_API APairedInteractionDriver : public AInfo
{
	GENERATED_BODY()

public:
	APairedInteractionDriver();

protected:
	UPROPERTY()
	UInteractionsData* InteractionData = nullptr;
	UPROPERTY()
	AActor* Driver = nullptr;
	UPROPERTY()
	AActor* Slave = nullptr;
	UPROPERTY()
	ABaseItem* OptionalItem = nullptr;
	
	FVector SlaveOriginalWorldPos;
	FRotator SlaveOriginalWorldRot;
	FVector DriverWorldPos;
	FRotator DriverWorldRot;
	FVector DriverForward;
	FVector DriverRight;
	FVector DriverUp;

	bool bHasPlayedMontages = false;

	FVector RelativeOffset;
	
	FVector SlaveFinalPos;
	FRotator SlaveFinalRot;

	float TotalTimeUnmatched = 0.0f;
	
	uint8 bEverRanDriverInteractionFinished : 1;
	uint8 bEverRanSlaveInteractionFinished : 1;
	
public:
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	uint8 bInteractionRunning : 1;
	
	void BeginInteraction();
	void TickInteraction(float DeltaTime);
	bool IsPlayingAnimationForCharacter(AReadyOrNotCharacter* TestCharacter, UAnimMontage* Montage);
	bool IsDriver(AActor* TestActor);
	bool IsSlave(AActor* TestActor);
	FVector GetDriverWorldPos();

	void EndInteraction();
	void EndSlaveInteraction();

	FORCEINLINE bool IsDriverFinished() const { return bEverRanDriverInteractionFinished; }
	FORCEINLINE bool IsSlaveFinished() const { return bEverRanSlaveInteractionFinished; }

	bool IsInPositionToPlayInteraction() const;

	bool ShouldHolsterItem() const;

	UFUNCTION()
	void OnEquippedItemHolstered(ABaseItem* Item);

	bool bCanPlayMontagesNow = false;
	
	UInteractionsData* GetInteractionData() const { return InteractionData; }

	// Returns whether or not the associated montages have begun playing
	bool HasPlayedMontages() { return bHasPlayedMontages; }

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPairedInteractionStarted);
	UPROPERTY(BlueprintAssignable)
	FOnPairedInteractionStarted Event_OnPairedInteractionStarted;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorInteractionFinished, AActor*, InActor);
	UPROPERTY(BlueprintAssignable)
	FOnActorInteractionFinished Event_OnDriverInteractionFinished;
	UPROPERTY(BlueprintAssignable)
	FOnActorInteractionFinished Event_OnSlaveInteractionFinished;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPairedInteractionFinished, AActor*, InDriver, AActor*, InSlave);
	UPROPERTY(BlueprintAssignable)
	FOnPairedInteractionFinished Event_OnPairedInteractionFinished;
	
	FTimerHandle OnAnimationFinished_Handle;
	UFUNCTION()
	void OnInteractionFinished();
	
	FTimerHandle OnDriverAnimationFinished_Handle;
	UFUNCTION()
	void OnDriverInteractionFinished();
	
	FTimerHandle OnSlaveAnimationFinished_Handle;
	UFUNCTION()
	void OnSlaveInteractionFinished();
	
	static APairedInteractionDriver* CreateAndPlayInteraction(UWorld* World, UInteractionsData* InInteractionsData, AActor* Driver, AActor* Slave, ABaseItem* InOptionalItem);
};

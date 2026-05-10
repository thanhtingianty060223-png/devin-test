// Copyright Void Interactive, 2022

#pragma once

#include "UObject/Object.h"
#include "AIActionData.h"
#include "Enums.h"
#include "AIAction.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class READYORNOT_API UAIAction : public UObject
{
	GENERATED_BODY()

public:
	void OnCreate(ACyberneticController* InController);
	
	void InitAction(ACyberneticController* InController, FAIActionData* InActionData);

	UFUNCTION(BlueprintCallable)
	void AbortAction();

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool WantsAbort() const { return bAbortAction; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE bool HasTag(const FName Tag) const { return Tags.Contains(Tag); }
	
	virtual void BeginAction();
	virtual void EndAction();
	virtual void Tick(float DeltaTime);

	virtual void OnTakeDamage(float Damage, AReadyOrNotCharacter* Instigator);
	
	virtual void OnSucceededToConsider();
	virtual void OnFailedToConsider();

	UFUNCTION(BlueprintNativeEvent)
			bool ShouldPerformAction() const;
	virtual bool ShouldPerformAction_Implementation() const;

	UFUNCTION(BlueprintNativeEvent)
			FString GatherDebugInfo() const;
	virtual FString GatherDebugInfo_Implementation() const;
	
	UFUNCTION(BlueprintNativeEvent)
			FName GetMoveStyleOverride() const;
	virtual FName GetMoveStyleOverride_Implementation() const;

	virtual UWorld* GetWorld() const override final;
	
protected:
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Create")
	void OnCreate_Blueprint(ACyberneticController* Controller);
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Init Action")
	void InitAction_Blueprint(ACyberneticController* Controller);
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Begin Action")
	void BeginAction_Blueprint();
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "End Action")
	void EndAction_Blueprint();
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Tick")
	void Tick_Blueprint(float DeltaTime);
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Succeeded To Consider")
	void OnSucceededToConsider_Blueprint();
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Failed To Consider")
	void OnFailedToConsider_Blueprint();
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Take Damage")
	void OnTakenDamage_Blueprint(float Damage, AReadyOrNotCharacter* Instigator);

	UFUNCTION(BlueprintCallable)
	void RequestMove(FVector Location, float AcceptanceRadius = 10.0f);

	UFUNCTION()
	void OnPathFound(int32 PathId, ERonNavigationQueryResult Result);
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Path Found")
	void OnPathFound_Blueprint(int32 PathId, ERonNavigationQueryResult Result);

	UFUNCTION()
	void OnMoveComplete(AAIController* Controller, int32 RequestID);

	UFUNCTION(BlueprintImplementableEvent, DisplayName = "On Move Complete")
	void OnMoveComplete_Blueprint(AAIController* Controller, int32 RequestID);

	UFUNCTION(BlueprintPure)
	ACyberneticCharacter* GetCharacter() const;
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE FAIActionData GetActionData() const { return *ActionData; }
	UFUNCTION(BlueprintPure)
	int32 GetActionRunCount() const;
	
	FAIActionData* ActionData = nullptr;

	UPROPERTY(EditAnywhere)
	TArray<FName> Tags;

	UPROPERTY(BlueprintReadOnly)
	ACyberneticController* OwningController = nullptr;

	uint8 bAbortAction : 1;
	uint8 bSearchingPath : 1;

	UPROPERTY(BlueprintReadOnly)
	int32 LastMoveRequestPathID = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 LastMoveRequestMoveID = 0;
};

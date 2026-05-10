// Copyright Void Interactive, 2022

#pragma once

#include "Components/ActorComponent.h"
#include "Animation/MoveStyle/RoNMoveStyleTable.h"
#include "RoNMoveStyleComponent.generated.h"

#define NEW_MOVESTYLE_LAYOUT 1

UCLASS(ClassGroup = (RoNMoveStyle), meta = (BlueprintSpawnableComponent, DisplayName = "RoNMoveStyleComponent"))
class URoNMoveStyleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URoNMoveStyleComponent();

	virtual void InitializeComponent() override;

	/* the movement style database to use for this character, active entry gets sent to graph */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoreData)
	UDataTable* MoveStyleDatabase;

	/* The default move style set on init of this character */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoreData)
	FName DefaultMoveStyleName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoreData)
	FName DefaulGaitName;

	UPROPERTY(VisibleAnywhere, Category = CoreData)
	FName MoveStyleCharacterName;
	
	UPROPERTY(VisibleAnywhere, Category = CoreData)
	FName PreviousMoveStyleName;
	UPROPERTY(VisibleAnywhere, Category = CoreData)
	uint8 bIsOverriding;
	
	UPROPERTY(VisibleAnywhere, Category = CoreData)
	FRoNMovementStyle ActiveMoveStyle;

	UFUNCTION()
	void OnRep_MoveStyle();

	UPROPERTY(ReplicatedUsing=OnRep_MoveStyle)
	FName Rep_MoveStyleName;

	/* gait */
	UPROPERTY(ReplicatedUsing=OnRep_MoveStyle, VisibleAnywhere, Category = CoreData)
	int ActiveGaitIndex;

	UPROPERTY(ReplicatedUsing=OnRep_MoveStyle, VisibleAnywhere, Category = CoreData)
	FName ActiveGaitName;

	const FRoNMovementStyle* GetMovementStyleByName(FName Name) const;

	UFUNCTION(BlueprintCallable)
	void SetOverrideMoveStyleByName(FName Name);
	UFUNCTION(BlueprintCallable)
	void ClearOverrideMoveStyle();

	UFUNCTION(BlueprintCallable, Category = RoNMoveStyle)
	virtual void SetMovementStyleByName(FName Name);

	UPROPERTY(VisibleInstanceOnly)
	float GaitTimeOut = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	FName PendingGaitName = NAME_None;
	
	UFUNCTION(BlueprintCallable, Category = RoNMoveStyle)
	bool SetMovementGaitByName(FName Name, bool bForce = false);
	
	UFUNCTION(BlueprintCallable, Category = RoNMoveStyle)
	virtual void SetCharacterSpeed(float Speed);

	UFUNCTION(BlueprintCallable, Category = RoNMoveStyle)
	void SetCharacterSpeedMultiplier(float Multiplier);

	UFUNCTION(BlueprintCallable, Category = RoNMoveStyle)
	virtual void SetCharacterAcceleration(float Acceleration);

	UFUNCTION(BlueprintCallable, Category = RoNMoveStyle)
	void SetCharacterAccelerationMultiplier(float Multiplier);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_IsStrafing, Category = CoreData)
	bool bIsStrafing;

	void SetIsStrafing(bool bNewStrafing);

	UFUNCTION()
	void OnRep_IsStrafing();

	bool bServerIsStrafing = false;

	float TargetInterpSpeed = 0.5f;
private:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float TargetSpeed = 160.0f;
	
	float LastSetSpeed;
	float LastSetAcceleration;

	float CurrentMoveSpeedMultiplier = 1.0f;
	float CurrentAccelerationMultiplier = 1.0f;
};
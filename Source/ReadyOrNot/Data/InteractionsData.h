// Copyright Void Interactive, 2018

#pragma once

#include "Engine/DataAsset.h"
#include "InteractionsData.generated.h"

USTRUCT(BlueprintType)
struct FPairedInteractionTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FName, class UInteractionsData*> Interactions;
};

UCLASS(BlueprintType, Blueprintable)
class UInteractionsData : public UDataAsset
{
	GENERATED_BODY()

public:
	UInteractionsData();
	
	// Called from the server.
	UFUNCTION(BlueprintCallable, Category = "Interaction Data")
    class APairedInteractionDriver* PlayPairedInteraction(AActor*  Driver, AActor*  Slave, class ABaseItem* OptionalItem = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Interaction Data")
    static APairedInteractionDriver* IsPairedInteractionPlayingOn(AActor* Target);
    static APairedInteractionDriver* IsPairedInteractionPlayingOn(const AActor* Target);
	
    static bool IsPairedInteractionPlayingOn(UInteractionsData* InteractionData, AActor* Target);
    static bool IsPairedInteractionPlayingOn(UInteractionsData* InteractionData, const AActor* Target);

	static UInteractionsData* GetInteraction(const FName& InteractionName);
	static TMap<FName, UInteractionsData*> GetInteractionCollection(const FName& CollectionName);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	FName InteractionName = "Default";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	uint8 bLooping : 1;
	
	// Allow this interaction to play even when the driver is dead
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	uint8 bAllowDeadDriverInteraction : 1;
	
	// Allow this interaction to play even when the slave is dead
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	uint8 bAllowDeadSlaveInteraction : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	uint8 bAllowAimOffsetDuringInteraction : 1;
	
	// In some cases we dont want to modify the Slave at all and start from where it was from
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	uint8 bApplyRelativeOffsetBeforePlaying : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	uint8 bUseSyncBone : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	uint8 bSweepEnvironment : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	uint8 bDoNotApplyRelativeOffset : 1;
	
	// Relative position offset starting from driver
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main", meta = (EditCondition = "!bDoNotApplyRelativeOffset || bApplyRelativeOffsetBeforePlaying"))
	FVector RelativePosOffsetToDriver;
	
	// Relative rotation offset starting from driver
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main", meta = (EditCondition = "!bDoNotApplyRelativeOffset || bApplyRelativeOffsetBeforePlaying"))
	FRotator RelativeRotOffsetToDriver;
	
	// Duration in which this interaction can be cancelled, order is from start to end in timing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	float CancelDurationLength = 0.0f;

	// Do we need to force a holster before playing this Interaction?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	uint8 bHolsterItemBeforePlaying : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main", meta = (EditCondition = "bHolsterItemBeforePlaying", EditConditionHides))
	uint8 bInstantHolster : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	uint8 bEquipLastItemAfterPlaying : 1;
	
	// Use when the rotation has to match the interaction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	uint8 bUpdateSlaveTransform : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	uint8 bUpdateTransformsInstantly : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	uint8 bIndependentFinishes : 1;

	// Should this interaction play a fp motion from driver perspective?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driver")
	uint8 bUseDriverFPMotion : 1;
	
	// The Driver Motion to play
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driver")
	UAnimMontage* DriverMontage = nullptr;

	// The Driver Motion to play in First-Person
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driver", meta = (EditCondition = "bUseDriverFPMotion"))
	UAnimMontage* DriverMontage_FP = nullptr;

	// The Tolerance allowed to start this interaction based on distance to slave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driver")
	float TriggerTolerance = 0.0f;

	// Time it takes to correct Driver into correct relative offset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driver")
	float DriverBlendDuration = 0.0f;
	
	// Should this interaction play a fp motion from slave perspective?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slave")
	uint8 bUseSlaveFPMotion : 1;

	// The Slave Motion to play, leave empty if slave recieves no motion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slave")
	UAnimMontage* SlaveMontage = nullptr;

	// The Slave Motion to play in First-Person
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slave", meta = (EditCondition = "bUseSlaveFPMotion"))
	UAnimMontage* SlaveMontage_FP = nullptr;
			
	// Use when the rotation has to match the interaction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slave")
	uint8 bMatchDriverYaw : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slave")
	float MatchedYawOffset = 0.0f;
	
	// Use shared item motion across driver/slave
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Other")
	uint8 bHasSharedItemAnim : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Other", meta = (EditCondition = "bHasSharedItemAnim"))
	UAnimMontage* SharedItemMontage = nullptr;
};

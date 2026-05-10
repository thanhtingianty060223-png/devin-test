// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Actors/Door.h"
#include "GameFramework/Actor.h"
#include "InteractWithDoorObjective.generated.h"

UENUM()
enum EObjectiveDoorInteractions
{
	ODI_Open,
	ODI_Close,
	ODI_Kick
};

UCLASS()
class READYORNOT_API AInteractWithDoorObjective : public AObjective
{
	GENERATED_BODY()
	
	AInteractWithDoorObjective();
	
	virtual void BeginPlay() override;

	virtual void TickObjective() override;

	UFUNCTION()
	void OnDoorKicked(ADoor* Door, AReadyOrNotCharacter* InstigatorCharacter, bool bSuccess);

	virtual void Multicast_UnlockObjective_Implementation() override;

	void SetupDoorBindings();

	UPROPERTY()
	TArray<ADoor*> Doors;

public:
	UPROPERTY(EditAnywhere, Category = "Objective Properties")
	FName DoorTag;

	UPROPERTY(EditAnywhere, Category = "Objective Properties")
	TEnumAsByte<EObjectiveDoorInteractions> DoorInteractionType;	
};

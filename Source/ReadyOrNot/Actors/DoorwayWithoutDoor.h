// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "Components/DoorwayComponent.h"
#include "DoorwayWithoutDoor.generated.h"

UCLASS(Blueprintable, HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API ADoorwayWithoutDoor : public AActor
{
	GENERATED_BODY()
	
public:
	ADoorwayWithoutDoor();

	UFUNCTION(BlueprintPure)
	FORCEINLINE UDoorwayComponent* GetDoorway() const { return Doorway; }

	UFUNCTION(BlueprintPure)
	FVector GetDoorSize() const;

	UFUNCTION(BlueprintPure)
	bool IsPointInFrontOfDoorway(FVector Point) const;
	
protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Door)
	UDoorwayComponent* Doorway = nullptr;
};

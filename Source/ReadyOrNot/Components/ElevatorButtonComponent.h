// © Void Interactive, 2017

#pragma once

#include "Components/StaticMeshComponent.h"
#include "Interfaces/CanUse.h"
#include "Interfaces/RespondToPlayerGaze.h"
#include "ElevatorButtonComponent.generated.h"

// Elevator Button Components go inside the elevator. Each one corresponds to either a floor or the door closing/opening.
// There's event bindings for when the button is pressed.
UCLASS(Blueprintable, BlueprintType)
class UElevatorButtonComponent : public UStaticMeshComponent, public ICanUse, public IRespondToPlayerGaze
{
	GENERATED_BODY()

public:
	//
	//	ICanUse implementation
	//

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Use)
		bool bOverrideButtonPromptText = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Use)
		FText ButtonPromptText;

	UPROPERTY(BlueprintReadWrite, Category = Use)
		class AElevator* OwningElevator;

	// Whether this is a button that controls the doors
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Use)
		bool bDoorButton = false;

	// If this is a door button, true = closes the doors, false = opens the doors
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Use)
		bool bDoorClose;

	// If this is not a door button, this indicates which floor to move the elevator to.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Use)
		int32 Floor;

	// Returns true if we can use this thing now
	virtual bool CanUse_Implementation(class APlayerCharacter* User) override { return true; }

	// If this returns false, this item should not be considered for trace
	virtual bool IsAvailableForUse_Implementation() override { return true; }

	// Returns true if we can use this thing now
	virtual bool StartUse_Implementation(class APlayerCharacter* User) override;

	// Stopped holding use
	virtual void EndUse_Implementation(class APlayerCharacter* User) override;

	// If true, override the button prompt text (it will use "TO INTERACT" instead)
	virtual bool OverridesUseButtonPromptText_Implementation() override { return bOverrideButtonPromptText; }

	// If true, this uses "HOLD" instead of "PRESS" in the button prompt
	virtual bool UsesHoldButtonPrompt_Implementation() override { return false; }

	// Get the text used for the button prompt
	virtual FText GetUseButtonPromptText_Implementation() override { return ButtonPromptText; }

	// If true, plays the icon complete animation when used
	virtual bool PlaysUseIconComplete_Implementation() { return false; }

	// Returns the component to bolt the icon to
	virtual USceneComponent* GetUseIconBoltComponent() { return this; }

	// Returns the components we have to be looking at in order to use this object
	virtual TArray<USceneComponent*> GetUseViewComponents_Implementation() override;

	//
	//	IRespondToPlayerGaze implementation
	//
};

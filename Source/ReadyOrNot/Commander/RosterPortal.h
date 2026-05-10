// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "RosterPortal.generated.h"

UENUM()
enum class ERosterPortalType : uint8
{
	Roster,
	Therapist,
	Loadout,
	Training
};

UCLASS()
class READYORNOT_API ARosterPortal : public AActor, public IUseabilityInterface
{
	GENERATED_BODY()

public:
	ARosterPortal();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;

	// Determines the default page the roster should open once interacted with
	UPROPERTY(EditInstanceOnly)
	ERosterPortalType PortalType;
	
protected:
	UPROPERTY(VisibleDefaultsOnly)
	UInteractableComponent* InteractableComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* BillboardComponent;
#endif
	
	bool IsRosterAvailable() const;
};

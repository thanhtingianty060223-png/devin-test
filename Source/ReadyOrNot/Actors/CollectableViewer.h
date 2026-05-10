// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CollectableViewer.generated.h"

class ACollectable;
class ACollectableViewController;
class AReadyOrNotCharacter;
class UInteractableComponent;

UCLASS(Blueprintable)
class READYORNOT_API ACollectableViewer : public AActor, public IUseabilityInterface
{
	GENERATED_BODY()

public:
	ACollectableViewer();
	
	virtual void BeginPlay() override;
	
	void CheckUnlocked();
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<ACollectable> CollectableClass;

private:
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	
	UPROPERTY(EditDefaultsOnly)
	UInteractableComponent* InteractableComponent;

	UPROPERTY(Transient)
	ACollectableViewController* ViewController;
};
// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MemorialViewer.generated.h"

UCLASS(Blueprintable, Abstract)
class READYORNOT_API AMemorialViewer : public AActor, public IUseabilityInterface
{
	GENERATED_BODY()

public:
	AMemorialViewer();
	
	virtual void BeginPlay() override;

private:
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;

	UPROPERTY(EditDefaultsOnly)
	USceneComponent* RootSceneComponent;
	
	UPROPERTY(EditDefaultsOnly)
	UInteractableComponent* InteractableComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* BillboardComponent;
#endif
	
	UPROPERTY(Transient)
	UUserWidget* Widget;
};

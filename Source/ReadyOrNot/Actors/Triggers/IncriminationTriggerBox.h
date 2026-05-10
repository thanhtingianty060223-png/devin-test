// Void Interactive, 2020

#pragma once

#include "Actors/Triggers/PVPTriggerBox.h"
#include "IncriminationTriggerBox.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API AIncriminationTriggerBox : public APVPTriggerBox
{
	GENERATED_BODY()
	
public:
	AIncriminationTriggerBox();

protected:
	void BeginPlay() override;

	void OnBeginOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor) override;
	void OnEndOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor) override;
};


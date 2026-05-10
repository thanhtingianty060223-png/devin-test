// Void Interactive, 2020

#pragma once

#include "Engine/TriggerBox.h"
#include "CTFTriggerBox.generated.h"

/**
 * A trigger box class that is used for Capture The Flag (CTF) game mode
 */
UCLASS()
class READYORNOT_API ACTFTriggerBox : public ATriggerBox
{
	GENERATED_BODY()
	
public:
	ACTFTriggerBox();

protected:
	void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent)
			bool FulfillsRequirements();
	virtual bool FulfillsRequirements_Implementation();

	UFUNCTION(BlueprintNativeEvent, DisplayName = "Trigger Enter")
			void OnBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
	virtual void OnBeginOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor);

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "VIP Trigger Box")
	UTextRenderComponent* TextRender = nullptr;
};

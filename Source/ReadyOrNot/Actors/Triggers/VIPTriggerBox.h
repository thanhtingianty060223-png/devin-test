// Copyright Void Interactive, 2021

#pragma once

#include "PVPTriggerBox.h"
#include "VIPTriggerBox.generated.h"

/**
 * A trigger box base class used for the VIP game mode
 */
UCLASS(BlueprintType, Blueprintable)
class READYORNOT_API AVIPTriggerBox : public APVPTriggerBox
{
	GENERATED_BODY()

public:
	AVIPTriggerBox();

	UFUNCTION(BlueprintPure, Category = "VIP Trigger Box")
	bool IsVIPInTriggerBox(APlayerCharacter*& OutVIPCharacter) const;

	void ShowObjectiveMarker() override;

protected:
	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;

	void OnBeginOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor) override;
    void OnEndOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor) override;
};

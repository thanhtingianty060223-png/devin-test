// Copyright Void Interactive, 2022
#pragma once

#include "CoreMinimal.h"
#include "FootstepFoleyVolume.generated.h"

/*
 *	Footstep foley volume, enables the specified foley sounds when a character walks through
 */
UCLASS(BlueprintType, Blueprintable)
class READYORNOT_API AFootstepFoleyVolume : public AVolume
{
	GENERATED_BODY()

	AFootstepFoleyVolume();

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor);

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* BillboardComponent;
#endif

public:
	// Footstep foley event to play when walking through this volume
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep Foley")
	UFMODEvent* FootstepFoleyEvent;

	// First-person only footstep foley event to play when walking through this volume. Defaults to the above event if not set
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep Foley")
	UFMODEvent* FootstepFoleyEventFirstPerson;

	// Whether or not NPCs should trigger the footstep foley
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep Foley")
	bool bNPCsTriggerFootstepFoley = true;
};

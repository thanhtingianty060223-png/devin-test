// � Void Interactive, 2017

#pragma once

#include "Components/BoxComponent.h"
#include "FMODEvent.h"
#include "FootstepFoleyComponent.generated.h"

/**
 *	When FootstepFoleyComponents are waded into, they turn on the FootstepFoley FMOD component on the PlayerCharacter.
 *	These should be placed near tall grass, snow piles, etc.
 *	@author	eezstreet
 */
UCLASS(meta = (BlueprintSpawnableComponent), Blueprintable, BlueprintType)
class READYORNOT_API UFootstepFoleyComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UFootstepFoleyComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Foley)
	UFMODEvent* SetEventTo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Foley)
	UFMODEvent* SetEventToRemote;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Foley)
	bool bPlayOnPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Foley)
	bool bPlayEveryStep = true;


	UFUNCTION()
	void StartedOverlapping(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void StoppedOverlapping(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void BeginPlay() override;
};
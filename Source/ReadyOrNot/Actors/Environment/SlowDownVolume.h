// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "SlowDownVolume.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ASlowDownVolume : public ATriggerBox
{
	GENERATED_BODY()

		ASlowDownVolume();

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(class AActor* OverlappedActor, class AActor* OtherActor);
	UFUNCTION()
	void OnOverlapEnd(class AActor* OverlappedActor, class AActor* OtherActor);

	UFUNCTION()
		void OnOverlapBeginComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnOverlapEndComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex);

	// set to less to slow down the player more
	// set higher than 1.0 to speed up the player
	UPROPERTY(EditAnywhere, Category = "Speed")
	float SlowDownMultiplier = 1.0f;
	
};

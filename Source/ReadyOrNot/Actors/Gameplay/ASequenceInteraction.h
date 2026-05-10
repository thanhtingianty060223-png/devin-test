// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FMODAudioComponent.h"
#include "FMODEvent.h"
#include "Components/BoxComponent.h"
#include "LevelSequenceActor.h"
#include "Interfaces/CanUse.h"
#include "Interfaces/RespondToPlayerGaze.h"
#include "ASequenceInteraction.generated.h"

UCLASS()
class READYORNOT_API AASequenceInteraction : public ALevelSequenceActor, public ICanUse, public IRespondToPlayerGaze
{
	GENERATED_BODY()
	
public:
	AASequenceInteraction(const FObjectInitializer& Init);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	//activate in range?
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bAutoActivateInRange;

	// Please set this so the character that is used by this actor can be looked at :)
	UPROPERTY(EditAnywhere)
	AActor* ReferencedCharacterViewTarget;

	UFUNCTION()
		void OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void OnBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	void PlaySequence(class APlayerCharacter* OverlappedCharacter);

	UPROPERTY(VisibleAnywhere)
		UBoxComponent* RadiusComp;

	UPROPERTY()
	APlayerCharacter* LastPlayedSequencerCharacter;

	UFUNCTION()
	void OnSequencerFinished();


	//
	//	ICanUse implementation
	//

// 	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Use)
// 		bool bOverrideButtonPromptText = false;
// 
// 	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Use)
// 		FText ButtonPromptText;
// 
// 	// Returns true if we can use this thing now
// 	virtual bool CanUse_Implementation(class APlayerCharacter* User) override { return true; }
// 
// 	// If this returns false, this item should not be considered for trace
// 	virtual bool IsAvailableForUse_Implementation() override { return true; }
// 
// 	// Returns true if we can use this thing now
// 	virtual bool StartUse_Implementation(class APlayerCharacter* User) override;



};

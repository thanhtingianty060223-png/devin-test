// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "CharacterReactionVolume.generated.h"

UCLASS(NotPlaceable)
class AReactionInterestPoint : public AActor
{
	GENERATED_BODY()

public:
	AReactionInterestPoint();

	virtual void Destroyed() override;
	
private:
	UPROPERTY()
	UBillboardComponent* BillboardComponent;
};

/*
 * Character reaction volume, specifies an area with interest points that characters can look at and react to
 */
UCLASS(HideCategories=("Collision", "Navigation", "Tags", "Cooking"))
class READYORNOT_API ACharacterReactionVolume : public AVolume
{
	GENERATED_BODY()

public:
	ACharacterReactionVolume();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Destroyed() override;
	
	virtual void Tick(float DeltaSeconds) override;

	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	
	// The list of possible voice lines that this volume can trigger
	UPROPERTY(EditAnywhere, Category="Reaction Volume")
	TArray<FString> PossibleVoiceLines;

	// Should stealth voice lines be enabled for this volume
	UPROPERTY(EditAnywhere, Category="Reaction Volume")
	bool bUseStealthVoiceLines = false;

	// Voice lines allowed whenever there is no alert present
	UPROPERTY(EditAnywhere, Category="Reaction Volume", meta=(EditCondition=bUseStealthVoiceLines))
	TArray<FString> PossibleStealthVoiceLines;

	// Should we only consider specific voices to react to this volume
	UPROPERTY(EditAnywhere, Category="Reaction Volume")
	bool bUseEligibleSpeakersOnly = false;

	// The specific speakers we want to consider eligible to react
	UPROPERTY(EditAnywhere, Category="Reaction Volume", meta=(EditCondition=bUseEligibleSpeakersOnly))
	TArray<FString> EligibleSpeakers;

	// Whether or not specific voice lines should be used (i.e. [CALL]EnvValley_1)
	UPROPERTY(EditAnywhere, Category="Reaction Volume")
	bool bUseSpecificVoiceLines = false;

	// Time the character will look at an interest point before speaking, with zero being no time
	UPROPERTY(EditAnywhere, Category="Reaction Volume")
	float InspectTimeBeforeReaction = 2.0f;

	// The time between attempts to have characters inside the volume react
	UPROPERTY(EditAnywhere, Category="Reaction Volume")
	float TimeBetweenReactionAttempts = 5.0f;

	// Time required without combat in order for a reaction to play
	UPROPERTY(EditAnywhere, Category="Reaction Volume")
	float TimeWithoutCombat = 15.0f;

	// The maximum number of reactions that can occur in a row
	UPROPERTY(EditAnywhere, Category="Reaction Volume")
	int32 MaxReactions = 1;

	// Volumes marked with the same tag will be disabled once any one of them has been triggered
	UPROPERTY(EditAnywhere, Category="Reaction Volume")
	FName VolumeTag;
	
	// Should this volume consider all characters or just SWAT
	UPROPERTY(EditAnywhere, Category="Reaction Volume")
	bool bSwatOnly = true;

	UPROPERTY(VisibleInstanceOnly, AdvancedDisplay, Category="Reaction Volume")
	TArray<AReactionInterestPoint*> InterestPoints;

	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnVolumeTriggered, ACharacterReactionVolume*, FName /** Tag */)
	static FOnVolumeTriggered OnVolumeTriggered;
	
private:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* BillboardComponent;
#endif

	UPROPERTY()
	TArray<AReadyOrNotCharacter*> OverlappingCharacters;
	
	TArray<FString> ReactedVoices;

	bool bHasEverTriggered = false;
	int32 CurrentReactions = 0;
	
	FTimerHandle TH_AttemptReaction;
	FTimerHandle TH_PlayReaction;
	
	UFUNCTION()
	void AttemptReaction();
	
	UFUNCTION()
	void PlayReaction(AReadyOrNotCharacter* Character);

	UFUNCTION()
	void ReactionLengthReady(float Length);
	
	UFUNCTION()
	void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void OnTaggedVolumeTriggered(ACharacterReactionVolume* Volume, FName Tag);
	
	void TryLookRandomPoint(AReadyOrNotCharacter* Character);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
	
	UFUNCTION(CallInEditor, Category="Reaction Volume")
	void AddInterestPoint();

	UFUNCTION(CallInEditor, Category="Reaction Volume")
	void RemoveInterestPoint();
#endif
};

// Copyright Void Interactive, 2021

#pragma once

#include "Conversation.generated.h"

/*
* Try and find AI to do the conversation with that meets these requirements, all requirements are optional, if left blank then they will not be required
*/
USTRUCT(BlueprintType)
struct FConversationData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	// if this is not blank then the speaker for this will be set for all speakers of this Id
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speaker")
	FName SpeakerId = NAME_None;

	// if specified will look at this speaker when talking
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speaker")
	FName LookAtSpeakerId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Line")
	bool bUseVoiceLineFromSpeechTable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Line", meta = (EditCondition = "bUseVoiceLineFromSpeechTable"))
	FString VoiceLineRowName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice Line", meta = (EditCondition = "!bUseVoiceLineFromSpeechTable"))
	USoundWave* VoiceLineWav = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Voice Line", meta = (EditCondition = "!bUseVoiceLineFromSpeechTable"))
	class ACyberneticCharacter* Speaker = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category =  "Voice Line")
	float AdditionalDelayAfterVoiceLineBeforeNextSpeaker = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optional Requirements")
	FName RequiredTag = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optional Requirements")
	ETeamType TeamType = ETeamType::TT_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optional Requirements")
	float MaxDistance = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optional Requirements")
	bool bRequireLineOfSight = false;
	// if the speaker is marked as optional it will just be skipped over if not set and is not otherwise required
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optional Requirements")
	bool bOptionalSpeaker = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Optional Requirements")
	int32 SkipXAfterOptionalSpeakerNotFound = 1;

	// if a tag is specified will try look after a world building activity by tag after converation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activities")
	FName GiveWorldBuildingActivityByTagOnStartConversation = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activities")
	FName GiveWorldBuildingActivityByTagAfterConversation = NAME_None;
};

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UConversation : public UObject
{
	GENERATED_BODY()

protected:
	FName ConversationId;

	// what part of the conversation are we up to
	int32 ConversationIdx = 0;

	FTimerHandle ContinueConversation_Handle;

public:

	// Conversation Data setup in the blueprint
	UPROPERTY(BlueprintReadWrite)
	TArray<FConversationData> ConversationData;

	// Optional blueprint overrideable function for building the conversation data
	UFUNCTION(BlueprintNativeEvent)
	void BuildConversation(FName Id);

	// trys to get all of the available speakers based on the paramaters defined
	UFUNCTION(BlueprintNativeEvent)
	void TryGetSpeakers();

	// Are all the requirements met (overrideable)
	UFUNCTION(BlueprintNativeEvent)
	bool RequirementsMet();

	// begins the conversation (overrideable)
	UFUNCTION(BlueprintNativeEvent)
	void BeginConversation(ACyberneticCharacter* ConversationStarter, FName Id = NAME_None);
	
	void EndConversation();

	bool CanConversationContinue();

	// continue the conversation
	UFUNCTION(BlueprintNativeEvent)
	void ContinueConversation();
	
	UFUNCTION()
	void ReplyToConversation(AReadyOrNotCharacter* Speaker);
	
	bool bConversationRunning = false;
	bool IsConversationPlaying();

	UFUNCTION(BlueprintCallable)
	void GoToSpecificConversationIdAndContinueConversation(int32 Idx) { ConversationIdx = Idx; ContinueConversation(); }

	bool IsSpeakerUnique(class ACyberneticCharacter* TestSpeaker);

	void SetSpeakerForAllSpeakerIds(class ACyberneticCharacter* Speaker, FName SpeakerId);

	UFUNCTION(BlueprintCallable)
	class ACyberneticCharacter* GetSpeakerForId(FName Id);

	UFUNCTION(BlueprintCallable)
	class ACyberneticCharacter* GetSpeakerForConversationIdx(int32 Idx);

	UFUNCTION(BlueprintCallable)
	FConversationData GetConversationData();

	FTimerHandle GiveWorldBuildingActivityAfterConversation_Handle;
	UFUNCTION()
	void GiveWorldBuildingActivityByTag(FName SpeakerId, FName Tag);

	FName GetConversationId() { return ConversationId; }

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConversationContinuing, int32, Idx);
	UPROPERTY(BlueprintAssignable)
	FOnConversationContinuing OnConversationContinuing;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndConversation);
	UPROPERTY(BlueprintAssignable)
	FOnEndConversation OnEndConversation;
};

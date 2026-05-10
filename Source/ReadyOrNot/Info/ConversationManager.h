// Copyright Void Interactive, 2021

#pragma once

#include "GameFramework/Info.h"
#include "ConversationManager.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API AConversationManager : public AInfo
{
	GENERATED_BODY()

public:
	AConversationManager();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	class UConversation* TryPlayConversation(FName ConversationId, class ACyberneticCharacter* ConversationStarter);

	UPROPERTY()
	TArray<UConversation*> RunningConversations;

	UFUNCTION(BlueprintCallable)
	class UConversation* PlayPrebuiltConversation(TSubclassOf<UConversation> Conversation, class ACyberneticCharacter* ConversationStarter, FName ConversationId = NAME_None);

	UFUNCTION(BlueprintCallable)
	void PlayConversationInst(UConversation* Conversation, ACyberneticCharacter* ConversationStarter);

	void StopConversationForSpeaker(ACyberneticCharacter* Speaker);

	bool IsSpeakerInRunningConversation(ACyberneticCharacter* Speaker);
};
  
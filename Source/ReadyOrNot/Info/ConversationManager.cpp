// Copyright Void Interactive, 2021

#include "ConversationManager.h"
#include "Conversation.h"

AConversationManager::AConversationManager()
{
	PrimaryActorTick.TickInterval = 1.0f;
}

void AConversationManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	for (int32 i = 0; i < RunningConversations.Num(); i++)
	{
		if (RunningConversations[i] && !RunningConversations[i]->IsConversationPlaying())
		{
			RunningConversations.RemoveAt(i);
		}
	}
}

UConversation* AConversationManager::TryPlayConversation(FName ConversationId, ACyberneticCharacter* ConversationStarter)
{
	for (TObjectIterator<UConversation> It; It; ++It)
	{
		UConversation* cnv = *It;
		if (cnv->IsConversationPlaying() && cnv->GetConversationId() == ConversationId)
		{
			return nullptr;
		}
	}
	UConversation* Conversation = NewObject<UConversation>(this);
	Conversation->BeginConversation(ConversationStarter, ConversationId);
	RunningConversations.AddUnique(Conversation);
	return Conversation;
}

UConversation* AConversationManager::PlayPrebuiltConversation(TSubclassOf<UConversation> Conversation, class ACyberneticCharacter* ConversationStarter, FName ConversationId)
{ 
	if (!Conversation)
		return nullptr;

	UConversation* ConversationInst = NewObject<UConversation>(this, Conversation);
	ConversationInst->BeginConversation(ConversationStarter, ConversationId);
	RunningConversations.AddUnique(ConversationInst);
	return ConversationInst;
}

void AConversationManager::PlayConversationInst(UConversation* Conversation, ACyberneticCharacter* ConversationStarter)
{
	if (!Conversation)
		return;
	
	RunningConversations.AddUnique(Conversation);
	Conversation->BeginConversation(ConversationStarter);
}

void AConversationManager::StopConversationForSpeaker(ACyberneticCharacter* Speaker)
{
	int32 i = 0;
	for (UConversation* cnv : RunningConversations)
	{
		if (cnv && cnv->bConversationRunning)
		{
			bool bStopped = false;
			for (const FConversationData& cnvData : cnv->ConversationData)
			{
				if (cnvData.Speaker == Speaker)
				{
					if (Speaker->VoiceSoundSource)
					{
						Speaker->VoiceSoundSource->Stop();
						Speaker->VoiceSoundSource = nullptr;

						bStopped = true;
					}
					
					break;
				}
			}

			if (bStopped)
			{
				break;
			}
		}
		
		i++;
	}

	if (RunningConversations.IsValidIndex(i))
		RunningConversations.RemoveAt(i);
}

bool AConversationManager::IsSpeakerInRunningConversation(ACyberneticCharacter* Speaker)
{
	for (UConversation* cnv : RunningConversations)
	{
		if (cnv && cnv->bConversationRunning)
		{
			for (FConversationData cnvData : cnv->ConversationData)
			{
				if (cnvData.Speaker == Speaker)
				{
					return true;
				}
			}
		}
	}
	return false;
}

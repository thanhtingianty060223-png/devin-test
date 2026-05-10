// Copyright Void Interactive, 2021

#include "Conversation.h"

#include "ReadyOrNotAISystem.h"
#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"
#include "lib/ScriptedLevelEvents.h"

void UConversation::BuildConversation_Implementation(FName Id)
{
	if (Id == NAME_None)
		return;

	ConversationData.Empty();
	UDataTable* dt = UBpGameplayHelperLib::GetConversationLookupDataTable();
	if (dt)
	{
		for (FName row : dt->GetRowNames())
		{
			if (row.ToString().Contains(Id.ToString()))
			{
				FConversationData* cnv = dt->FindRow<FConversationData>(row, "ConversationManagerLookup");
				if (cnv)
				{
					ConversationData.Add(*cnv);
				}
			}
		}
	}
}

void UConversation::TryGetSpeakers_Implementation()
{
	float ClosestDist = FLOAT_NON_FRACTIONAL;
	for (int32 i = 1; i < ConversationData.Num(); i++)
	{
		FConversationData cnv = ConversationData[i];
		{
			if (!cnv.Speaker)
			{
				for (ACyberneticCharacter* c : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
				{
					if (!c)
						continue;
					
					if (c->IsDeadOrUnconscious() || c->IsIncapacitated() || c->IsInRagdoll())
						continue;
					
					if (cnv.TeamType != ETeamType::TT_NONE && cnv.TeamType != c->GetTeam())
						continue;
						
					// Tag requirement
					if (cnv.RequiredTag != NAME_None)
					{
						if (!c->Tags.Contains(cnv.RequiredTag))
							continue;
					}

					ACyberneticCharacter* ConversationStarter = ConversationData[0].Speaker;
					if (ConversationStarter)
					{
						if (cnv.MaxDistance > 0.0f)
						{
							float Dist = (c->GetActorLocation() - ConversationStarter->GetActorLocation()).Size();
							if (Dist > cnv.MaxDistance)
								continue;

							if (Dist < ClosestDist)
							{
								ClosestDist = Dist;
							}
							else
							{
								continue;
							}
						}
						if (cnv.bRequireLineOfSight)
						{
							if (!UBpGameplayHelperLib::HasLineOfSight(c, ConversationStarter))
								continue;
						}
					}

					// if (UReadyOrNotStatics::GetConversationManager()->IsSpeakerInRunningConversation(c))
					// 	continue;
					
					// check they are not already speaker
					if (!IsSpeakerUnique(c))
						continue;

					SetSpeakerForAllSpeakerIds(c, cnv.SpeakerId);
				}
			}
		}
	}
}

bool UConversation::RequirementsMet_Implementation()
{
	for (int32 i = 0; i < ConversationData.Num(); i++)
	{
		FConversationData cnv = ConversationData[i];
		if (!cnv.Speaker && !cnv.bOptionalSpeaker)
		{
			return false;
		}
	}
	return true;
}

void UConversation::BeginConversation_Implementation(ACyberneticCharacter* ConversationStarter, FName Id)
{
	ConversationId = Id;
	if (ConversationData.IsValidIndex(0))
	{
		ConversationData[0].Speaker = ConversationStarter;
		SetSpeakerForAllSpeakerIds(ConversationStarter, ConversationData[0].SpeakerId);
	}

	TryGetSpeakers();
	
	if (RequirementsMet())
	{
		bConversationRunning = true;
		GetWorld()->GetTimerManager().SetTimer(ContinueConversation_Handle, this, &UConversation::ContinueConversation, 1.0f);
	}
}

void UConversation::EndConversation()
{
	OnEndConversation.Broadcast();
	bConversationRunning = false;
}

bool UConversation::CanConversationContinue()
{
	if (UReadyOrNotAISystem::WasRecentlyInCombat(5.0f))
		return false;
	
	for (int32 i = 0; i < ConversationData.Num(); i++)
	{
		FConversationData cnv = ConversationData[i];
		if (cnv.Speaker)
		{
			if ((cnv.Speaker->IsDeadOrUnconscious() || cnv.Speaker->bHasEverShot) && !cnv.bOptionalSpeaker)
			{
				return false;
			}
		}
	}
	
	return true;
}

void UConversation::ContinueConversation_Implementation()
{
	if (!ConversationData.IsValidIndex(ConversationIdx) || !CanConversationContinue())
	{
		OnEndConversation.Broadcast();
		bConversationRunning = false;
		return;
	}

	bConversationRunning = true;
	FConversationData cnv = ConversationData[ConversationIdx];

	// Skip if speaker is not found or something happens during conversation
	if (cnv.Speaker == nullptr || cnv.Speaker->IsDeadOrUnconscious())
	{
		ConversationData.RemoveAt(ConversationIdx);
		ContinueConversation();
		return;
	}
	
	cnv.Speaker->OnVoiceAudioStoppedDelegate.RemoveAll(this);
	cnv.Speaker->OnVoiceAudioStoppedDelegate.AddDynamic(this, &UConversation::ReplyToConversation);
	cnv.Speaker->PlayRawVO(cnv.VoiceLineRowName);
}

void UConversation::ReplyToConversation(AReadyOrNotCharacter* Speaker)
{
	FConversationData cnv = ConversationData[ConversationIdx];
	if (!cnv.Speaker)
		return;
	
	cnv.Speaker->OnVoiceAudioStoppedDelegate.RemoveDynamic(this, &UConversation::ReplyToConversation);
	// handle LookAt
	AAIController* AiController = Cast<AAIController>(cnv.Speaker->GetController());
	if (AiController)
	{
		AiController->SetFocus(GetSpeakerForId(cnv.LookAtSpeakerId), EAIFocusPriority::Default);
		ACyberneticCharacter* OtherCharacter = Cast<ACyberneticCharacter>(GetSpeakerForId(cnv.LookAtSpeakerId));
		if (OtherCharacter)
		{
			AAIController* OtherController = Cast<AAIController>(OtherCharacter->GetController());
			if (OtherController)
			{
				OtherController->SetFocus(cnv.Speaker, EAIFocusPriority::Default);
			}
		}
	}
	
	GetWorld()->GetTimerManager().SetTimer(ContinueConversation_Handle, this, &UConversation::ContinueConversation, 1.0f);

	// Not used
	/*
	if (cnv.GiveWorldBuildingActivityByTagAfterConversation != NAME_None)
	{
		GetWorld()->GetTimerManager().SetTimer(GiveWorldBuildingActivityAfterConversation_Handle,
		                                       FTimerDelegate::CreateUObject(
			                                       this, &UConversation::GiveWorldBuildingActivityByTag, cnv.SpeakerId,
			                                       cnv.GiveWorldBuildingActivityByTagAfterConversation),
		                                       1.0f, false);
	}
	
	if (cnv.GiveWorldBuildingActivityByTagOnStartConversation != NAME_None)
	{
		GiveWorldBuildingActivityByTag(cnv.SpeakerId, cnv.GiveWorldBuildingActivityByTagOnStartConversation);
	}
	*/

	OnConversationContinuing.Broadcast(ConversationIdx);
	ConversationIdx++;
}

bool UConversation::IsConversationPlaying()
{
	return bConversationRunning;
}

bool UConversation::IsSpeakerUnique(class ACyberneticCharacter* TestSpeaker)
{
	for (int32 i = 0; i < ConversationData.Num(); i++)
	{
		if (ConversationData[i].Speaker && ConversationData[i].Speaker == TestSpeaker)
			return false;
	}
	return true;
}

void UConversation::SetSpeakerForAllSpeakerIds(class ACyberneticCharacter* Speaker, FName SpeakerId)
{
	if (SpeakerId == NAME_None)
		return;


	for (int32 i = 0; i < ConversationData.Num(); i++)
	{
		if (ConversationData[i].SpeakerId == SpeakerId)
		{
			ConversationData[i].Speaker = Speaker;
		}
	}
}

ACyberneticCharacter* UConversation::GetSpeakerForId(FName Id)
{
	if (Id == NAME_None)
		return nullptr;

	for (int32 i = 0; i < ConversationData.Num(); i++)
	{
		if (ConversationData[i].SpeakerId == Id)
		{
			return ConversationData[i].Speaker;
		}
	}

	// if not found try by actor tag
	for (ACyberneticCharacter* ai : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
	{
		if (ai && ai->Tags.Contains(Id))
		{
			return ai;
		}
	}
	
	return nullptr;
}

ACyberneticCharacter* UConversation::GetSpeakerForConversationIdx(int32 Idx)
{
	if (ConversationData.IsValidIndex(Idx))
	{
		return ConversationData[Idx].Speaker;
	}
	return nullptr;
}

FConversationData UConversation::GetConversationData()
{
	if (ConversationData.IsValidIndex(ConversationIdx))
		return ConversationData[ConversationIdx];

	return FConversationData();
}

void UConversation::GiveWorldBuildingActivityByTag(FName SpeakerId, FName Tag)
{
	if (GetSpeakerForId(SpeakerId))
	{
		UScriptedLevelEvents::GiveWorldBuildingActivityByTag(Cast<ACyberneticController>(GetSpeakerForId(SpeakerId)->GetController()), Tag);
	}
}

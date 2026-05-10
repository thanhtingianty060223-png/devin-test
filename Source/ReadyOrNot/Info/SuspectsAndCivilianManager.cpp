// Copyright Void Interactive, 2022

#include "SuspectsAndCivilianManager.h"

#include "Conversation.h"
#include "ReadyOrNotAIConfig.h"
#include "Activities/InvestigateStimulusActivity.h"
#include "Activities/WorldBuildingActivity.h"
#include "Characters/CyberneticController.h"
#include "Characters/AI/SuspectCharacter.h"
#include "Info/ConversationManager.h"

DECLARE_CYCLE_STAT(TEXT("RoN ~ Suspects And Civilian Manager ~ Try Get Speakers"), STAT_RONTryGetSpeakers, STATGROUP_SuspectsAndCiviliansManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Suspects And Civilian Manager ~ Play Bark Or Start Conversation"), STAT_RONPlayBark, STATGROUP_SuspectsAndCiviliansManager);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Suspects And Civilian Manager ~ Play Random Idle Or Combat Line"), STAT_RONPlayRandomIdleOrCombatLine, STATGROUP_SuspectsAndCiviliansManager);

USuspectsAndCivilianManager* USuspectsAndCivilianManager::Get(const UObject* WorldContext)
{
	return WorldContext->GetWorld()->GetSubsystem<USuspectsAndCivilianManager>();
}

void USuspectsAndCivilianManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TimeSinceIdleVO_MirrorGun -= DeltaSeconds;
	
	TimeSinceIdleVO -= DeltaSeconds;
	if (TimeSinceIdleVO <= 0.0f)
	{
		TimeSinceIdleVO = FMath::FRandRange(7.0f, 20.0f);
		PlayRandomIdleLine();
	}
	
	TimeSinceLastInvestigation += DeltaSeconds;

	for (TMap<FString, float>::TIterator It = SpeechCooldownMap.CreateIterator(); It; ++It)
	{
		It.Value() -= DeltaSeconds;
		if (It.Value() <= 0.0f)
		{
			It.RemoveCurrent();
		}
	}
}

TStatId USuspectsAndCivilianManager::GetStatId() const
{
	return GetStatID();
}

bool USuspectsAndCivilianManager::CanInvestigate()
{
	return !UActivityManager::Get(this)->IsActivityClassOnCooldown(UInvestigateStimulusActivity::StaticClass());
}

void USuspectsAndCivilianManager::StartedInvestigating()
{
	TimeSinceLastInvestigation = 0.0f;
}

bool USuspectsAndCivilianManager::CanInvestigateTrap(ATrapActor* Trap)
{
	return !InvestigatedTrap.Contains(Trap);
}

void USuspectsAndCivilianManager::StartedInvestigatingTrap(ATrapActor* Trap)
{
	InvestigatedTrap.AddUnique(Trap);
}

TArray<ACyberneticCharacter*> USuspectsAndCivilianManager::GetAllSuspectsAndCivilians() const
{
	TArray<ACyberneticCharacter*> All;
	All.Reserve(50);
	
	All += Suspects;
	All += Civilians;

	All.RemoveAll([](ACyberneticCharacter* Element)
	{
		return !Element || !IsValid(Element);
	});

	return All;
}

int32 USuspectsAndCivilianManager::GetNumPerformingWorldBuilding() const
{
	int32 Num = 0;
	
	for (const ACyberneticCharacter* AI : Suspects)
	{
		const ACyberneticController* CyberneticController = IsValid(AI) ? AI->GetCyberneticsController() : nullptr; 
		if (IsValid(CyberneticController) && CyberneticController->GetCurrentActivity<UWorldBuildingActivity>())
		{
			Num++;
		}
	}
	
	for (const ACyberneticCharacter* AI : Civilians)
	{
		const ACyberneticController* CyberneticController = IsValid(AI) ? AI->GetCyberneticsController() : nullptr;
		if (IsValid(CyberneticController) && CyberneticController->GetCurrentActivity<UWorldBuildingActivity>())
		{
			Num++;
		}
	}

	return Num;
}

void USuspectsAndCivilianManager::PlayRandomIdleLine()
{
	SCOPE_CYCLE_COUNTER(STAT_RONPlayRandomIdleOrCombatLine)
	
	TArray<ACyberneticCharacter*> AliveSuspects = Suspects;
	AliveSuspects.RemoveAll([](ACyberneticCharacter* Element)
	{
		return !Element || !Element->GetController() || Element->IsDeadOrUnconscious() || Element->IsPlayingDead() || Element->IsIncapacitated() || Element->IsInRagdoll() || Element->GetCyberneticsController()->GetAwarenessState() == EAIAwarenessState::Alerted;
	});
	
	TArray<ACyberneticCharacter*> AliveCivilians = Civilians;
	AliveCivilians.RemoveAll([](ACyberneticCharacter* Element)
	{
		return !Element || !Element->GetController() || Element->IsDeadOrUnconscious() || Element->IsPlayingDead() || Element->IsIncapacitated() || Element->IsInRagdoll() || Element->GetCyberneticsController()->GetAwarenessState() == EAIAwarenessState::Alerted;
	});

	if (AliveSuspects.Num() > 0 && (FMath::RandBool() || AliveCivilians.Num() == 0))
	{
		ACyberneticCharacter* RandomSuspect = AliveSuspects[FMath::RandRange(0, AliveSuspects.Num() - 1)];

		if (RandomSuspect->IsStunned() || RandomSuspect->IsPlayingStunAnimation())
			return;
		
		if (RandomSuspect->IsWearingExplosiveVest())
		{
			RandomSuspect->PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::IDLE_BOMB_VEST);
		}
		else if (RandomSuspect->IsArrested())
		{
			RandomSuspect->PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::IDLE_ARRESTED);
		}
		else
		{
			PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::IDLE, RandomSuspect);
		} 
	}
	else if (AliveCivilians.Num() > 0)
	{
		ACyberneticCharacter* RandomCivilian = AliveCivilians[FMath::RandRange(0, AliveCivilians.Num() - 1)];

		if (RandomCivilian->IsStunned() || RandomCivilian->IsPlayingStunAnimation())
			return;
		
		if (RandomCivilian->IsWearingExplosiveVest())
		{
			RandomCivilian->PlayRawVOWithCooldown(VO_SUSPECTS_AND_CIVILIAN::IDLE_BOMB_VEST, 45.0f);
		}
		else if (RandomCivilian->IsArrested())
		{
			RandomCivilian->PlayRawVO(VO_SUSPECTS_AND_CIVILIAN::IDLE_ARRESTED);
		}
		else
		{
			PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::IDLE, RandomCivilian);
		}
	}
}

void USuspectsAndCivilianManager::PlayBarkOrStartConversation(FString SpeechRow, ACyberneticCharacter* Speaker)
{
	SCOPE_CYCLE_COUNTER(STAT_RONPlayBark)
	
	if (!Speaker)
		return;

	if (SpeechRow.IsEmpty())
		return;

	if (Speaker->IsDeadOrUnconscious() || Speaker->IsIncapacitated() || Speaker->IsPlayingDead() || Speaker->IsInRagdoll())
		return;
	
	if (AReadyOrNotLevelScript* ls = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
	{
		if (ls->GetConversationManager()->IsSpeakerInRunningConversation(Speaker))
		{
			return;
		}
	}

	// Remove these prefixs before operating on SpeechRow
	SpeechRow.ReplaceInline(*VO_PREFIXES::BARK, TEXT(""));
	SpeechRow.ReplaceInline(*VO_PREFIXES::TELL, TEXT(""));
	SpeechRow.ReplaceInline(*VO_PREFIXES::REPLY, TEXT(""));
	
	// try and see if a conversation can be played first
	UConversation* ConvInst = NewObject<UConversation>(this, UConversation::StaticClass());

	if (SpeechRow == "Idle")
	{
		FConversationData Tell;
		Tell.SpeakerId = FName(VO_PREFIXES::TELL);
		Tell.Speaker = Speaker;
		Tell.bUseVoiceLineFromSpeechTable = true;
		Tell.VoiceLineRowName = VO_PREFIXES::TELL + SpeechRow;
		
		FConversationData Reply;
		Reply.SpeakerId = FName(VO_PREFIXES::REPLY);
		Reply.bRequireLineOfSight = false;			
		Reply.MaxDistance = 1000.0f;
		Reply.TeamType = ETeamType::TT_NONE; // Use no team so we can talk to civs as well
		Reply.bUseVoiceLineFromSpeechTable = true;
		Reply.VoiceLineRowName = VO_PREFIXES::REPLY + SpeechRow;
		
		ConvInst->ConversationData.Add(Tell);
		ConvInst->ConversationData.Add(Reply);
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_RONTryGetSpeakers)
		ConvInst->TryGetSpeakers();
	}
	
	FRoom* SpeakerRoom = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(Speaker->GetActorLocation());

	if (!SpeakerRoom)
		return;

	ACyberneticCharacter* Responder = nullptr;//ConvInst->GetSpeakerForId(FName(VO_PREFIXES::REPLY));
	
	for (ACyberneticCharacter* c : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
	{
		if (!c)
			continue;

		if (c == Speaker)
			continue;
		
		if (c->IsDeadOrUnconscious() || c->IsIncapacitated() || c->IsInRagdoll())
			continue;

		if (!c->IsActive())
			continue;

		// same room as speaker?
		if (UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(c->GetActorLocation()) != SpeakerRoom)
			continue;
		
		ACyberneticCharacter* ConversationStarter = Speaker;
		float ClosestDist = 1000.0f;
		if (ConversationStarter)
		{
			float Dist = FVector::Distance(c->GetActorLocation(), ConversationStarter->GetActorLocation());
			if (Dist > 1000.0f)
				continue;

			if (Dist < ClosestDist)
			{
				ClosestDist = Dist;
				Responder = c;
			}

			//if (!ConversationStarter->HasLineOfSightToCharacter(c))
				//continue;
		}
	}
	
	if (Responder)
	{
		// Special idle conversation logic
		if (SpeechRow == "Idle")
		{
			// Play ally or enemy lines depending on if the speaker and responder are on the same team
			bool bSameTeam = Responder->GetTeam() == Speaker->GetTeam();
			FString VoiceLine = VO_PREFIXES::BARK + SpeechRow + (bSameTeam ? "Ally" : "Enemy");

			for (int32 i = 0; i < ConvInst->ConversationData.Num(); i++)
			{
				ConvInst->ConversationData[i].VoiceLineRowName = VoiceLine;
			}
		}

		if (AReadyOrNotLevelScript* ls = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
		{
			ls->GetConversationManager()->PlayConversationInst(ConvInst, Speaker);
		}
	}
	else
	{
		if (SpeechRow == "Idle")
		{
			Speaker->PlayRawVO(VO_PREFIXES::BARK + SpeechRow + "Self");
		}
		else
		{
			if (Speaker->HasSpecificSpeech(VO_PREFIXES::BARK + SpeechRow))
				Speaker->PlayRawVO(VO_PREFIXES::BARK + SpeechRow);
			else
				Speaker->PlayRawVO(VO_PREFIXES::TELL + SpeechRow);
		}
	}
}

void USuspectsAndCivilianManager::PlayBarkOrStartConversation(FString SpeechRow, FVector Location)
{
	float closestDist = 1000.0f;
	ACyberneticCharacter* closestSpeaker = nullptr;
	for (ACyberneticCharacter* a : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
	{
		const float tstDst = FVector::Distance(a->GetActorLocation(), Location);
		if (tstDst < closestDist && (a->GetTeam() != ETeamType::TT_SERT_RED || a->GetTeam() != ETeamType::TT_SERT_BLUE))
		{
			closestDist = tstDst;
			closestSpeaker = a;
		}
	}
	
	if (closestSpeaker)
	{
		PlayBarkOrStartConversation(SpeechRow, closestSpeaker);
	}
}

void USuspectsAndCivilianManager::TriggerSharedBarkOrConversation(FString& SpeechRow, ACyberneticCharacter* Speaker, float Cooldown /*= 3.0f*/)
{
	Server_PlaySharedBarkOrStartConversation_Implementation(SpeechRow, Speaker, Cooldown);
}

void USuspectsAndCivilianManager::Server_PlaySharedBarkOrStartConversation_Implementation(const FString& SpeechRow, ACyberneticCharacter* Speaker, float Cooldown /*= 3.0f*/)
{
	if (Speaker->IsDeadOrUnconscious() || Speaker->IsIncapacitated())
		return;
	
	if (SpeechCooldownMap.Num() > 0 && SpeechCooldownMap.Contains(SpeechRow))
	{
		return;
	}

	if (SpeechRow.Contains(VO_PREFIXES::BARK) && Speaker)
	{
		if (Speaker->PlayRawVO(SpeechRow))
		{
			SpeechCooldownMap.Add(SpeechRow, Cooldown);
		}
	}
	else
	{
		PlayBarkOrStartConversation(SpeechRow, Speaker);	
		SpeechCooldownMap.Add(SpeechRow, Cooldown);
	}
}


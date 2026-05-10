// Void Interactive, 2020

#include "MusicManager.h"

#include "ReadyOrNotDebugSubsystem.h"
#include "Activities/Team/TeamBreachAndClearActivity.h"
#include "Actors/Door.h"
#include "Characters/CyberneticController.h"
#include "Characters/AI/SWATCharacter.h"
#include "GameModes/CoopGS.h"

UMusicManager::UMusicManager()
{
}

UMusicManager* UMusicManager::Get(UObject* Context)
{
	return Context->GetWorld()->GetSubsystem<UMusicManager>();
}

void UMusicManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	if (AReadyOrNotLevelScript* LevelScript = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
	{
		if (LevelScript->MusicData)
		{
			MusicEvent = LevelScript->MusicData->FMODTracks.LevelEvent;
			
			if (MusicEvent)
			{
				GetWorld()->OnWorldBeginPlay.AddUObject(this, &UMusicManager::StartMusicParametersUpdate);
			}
			else
			{
				UE_LOG(LogReadyOrNot, Warning, TEXT("Failed to start playing MusicEventInst. Music will not play."));
			}
		}
		else
		{
			UE_LOG(LogReadyOrNot, Warning, TEXT("MusicData is not set in the level script actor. Music will not play."));
		}
	}
}

void UMusicManager::Deinitialize()
{
	Super::Deinitialize();

	UFMODBlueprintStatics::EventInstanceRelease(MusicEventInst);

	//GEngine->ForceGarbageCollection();
}

bool UMusicManager::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UMusicManager::UpdateMusicParameters()
{
	if (!GetWorld() || GetWorld()->TimeSeconds < 1.0f)
		return;
	
	AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (!GameState)
		return;

	if (!MusicEvent)
	{
		if (const AReadyOrNotLevelScript* LevelScript = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
		{
			if (LevelScript->MusicData)
			{
				MusicEvent = LevelScript->MusicData->FMODTracks.LevelEvent;
			}
		}
	}

	#if !UE_BUILD_SHIPPING
	if (CHECK_DEBUG_SUBSYSTEM)
	{
		if (DEBUG_SUBSYSTEM->bDisableMusic)
		{
			StopTheMusic(true);
			
			return;
		}
	}
	#endif

	if (!MusicEventInst.Instance)
	{
		MusicEventInst = UFMODBlueprintStatics::PlayEvent2D(GetWorld(), MusicEvent, true);
	}
	
	if (MusicEventInst.Instance)
	{
		MusicEventInst.Instance->setVolume(GameState->MatchState == EMatchState::MS_Playing ? 1.0f : 0.0f);
		MusicEventInst.Instance->setParameterByName("Entry", 1.0f);
		
		MusicEventInst.Instance->setParameterByName("Entry_DistanceFromDoor", GetClosestDistanceToDoor() < 1000.0f ? 1.0f : 0.0f);

		float CombatMusic = GetCombatMusicIntensity();
		float AmbientTension = 0.0f;
		if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
		{
			for (const ACyberneticCharacter* AI : GS->AllAICharacters)
			{
				if (IsValid(AI) && AI->GetCyberneticsController() && AI->IsOnSWATTeam())
				{
					if (const UTeamBreachAndClearActivity* TeamBreachAndClearActivity = AI->GetCyberneticsController()->GetCurrentActivity<UTeamBreachAndClearActivity>())
					{
						if (TeamBreachAndClearActivity->HasBreached() && TeamBreachAndClearActivity->GetBreachTime() < 10.0f)
						{
							AmbientTension = 1.0f;
							break;
						}
					}
				}
			}
		}
		
		MusicEventInst.Instance->setParameterByName("Combat", CombatMusic);
		MusicEventInst.Instance->setParameterByName("Combat Tension", CombatMusic);

		#if !UE_BUILD_SHIPPING
		GEngine->AddOnScreenDebugMessage(999123, 2.0f, FColor::White, "CombatMusic: " + FString::SanitizeFloat(CombatMusic));
		#endif
		
		if (const APlayerCharacter* LocalPlayer = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld()))
		{
			MusicEventInst.Instance->setParameterByName("Ambient Tension", CombatMusic == 1.0f ? 0.0f : AmbientTension);
			//MusicEventInst.Instance->setParameterByName("Upper Rooms", LocalPlayer->GetActorLocation().Z > 1000.0f ? 1.2f : 0.0f);
			MusicEventInst.Instance->setParameterByName("Health", LocalPlayer->GetCurrentHealth());
		}

		if (ACoopGS* CoopGS = Cast<ACoopGS>(GameState))
		{
			if (CoopGS->MatchState == EMatchState::MS_MatchEnded)
			{
				if (CoopGS->bMissionSucceded)
				{
					MusicEventInst.Instance->setParameterByName("Win", 1.0f);
					MusicEventInst.Instance->setParameterByName("Lose", 0.0f);
				}
				else
				{
					MusicEventInst.Instance->setParameterByName("Win", 0.0f);
					MusicEventInst.Instance->setParameterByName("Lose", 1.0f);
				}
			}
		}
	}
}

void UMusicManager::ResumeMusicParametersUpdate()
{
	GetWorld()->GetTimerManager().UnPauseTimer(TH_UpdateMusicParameters);
}

void UMusicManager::StartMusicParametersUpdate()
{
	if (!GetWorld()->GetTimerManager().IsTimerActive(TH_UpdateMusicParameters))
		GetWorld()->GetTimerManager().SetTimer(TH_UpdateMusicParameters, this, &UMusicManager::UpdateMusicParameters, 1.0f, true);
}

void UMusicManager::PauseMusicParametersUpdate()
{
	GetWorld()->GetTimerManager().PauseTimer(TH_UpdateMusicParameters);
}

void UMusicManager::StopMusicParametersUpdate()
{
	GetWorld()->GetTimerManager().ClearTimer(TH_UpdateMusicParameters);
}

void UMusicManager::SetMusicParameterValue(const FString ParamName, const float ParamValue)
{
	if (ParamName.IsEmpty())
		return;
	
	if (MusicEventInst.Instance)
		MusicEventInst.Instance->setParameterByName(TCHAR_TO_ANSI(*ParamName), ParamValue);
}

FVector UMusicManager::GetLocalPlayerLocation() const
{
	if (APlayerCharacter* LocalPlayer = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld()))
	{
		return LocalPlayer->GetActorLocation();
	}
	
	return FVector::ZeroVector;
}

float UMusicManager::GetClosestDistanceToDoor() const
{
	float ClosestDist = BIG_DIST;

	const FVector LocalPlayerLocation = GetLocalPlayerLocation();
	if (LocalPlayerLocation != FVector::ZeroVector)
	{
		if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
		{
			for (const ADoor* Door : GS->AllDoors)
			{
				const float Dist = FVector::Distance(LocalPlayerLocation, Door->GetDoorMidLocation());
				if (Dist < ClosestDist)
				{
					ClosestDist = Dist;
				}
			}
		}
	}
	
	return ClosestDist;
}

float UMusicManager::GetCombatMusicIntensity()
{
	if (AReadyOrNotGameState* GS = UBpGameplayHelperLib::GetWorldStatic()->GetGameState<AReadyOrNotGameState>())
	{
		for (const ACyberneticCharacter* AI : GS->AllAICharacters)
		{
			const float TimeSinceFiredGun = AI->TimeSinceLastShot;
			
			if (AI->IsDeadOrUnconscious() && AI->TimeDeadOrUnconcious > 5.0f)
				continue;
			
			if (AI->IsIncapacitated() && AI->TimeIncapacitated > 5.0f)
				continue;
			
			if (TimeSinceFiredGun < 15 && AI->bHasEverShot)
			{
				return 1.0f;
			}
		}
	}
	
	return 0.0f;
}

TArray<FString> UMusicManager::GetMusicParameters() const
{
	TArray<FString> ParamNames;
	UFMODEvent* FMODEvent = MusicEvent;

	if (!MusicEvent)
	{
		if (AReadyOrNotLevelScript* LevelScript = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
		{
			if (LevelScript->MusicData)
			{
				FMODEvent = LevelScript->MusicData->FMODTracks.LevelEvent;
			}
		}
	}

	if (!FMODEvent)
	{
		return ParamNames;
	}
	
	TArray<FMOD_STUDIO_PARAMETER_DESCRIPTION> Params;
	FMODEvent->GetParameterDescriptions(Params);
	for (FMOD_STUDIO_PARAMETER_DESCRIPTION& Param : Params)
	{
		ParamNames.Add(Param.name);
	}

	return ParamNames;
}

void UMusicManager::StopTheMusic(const bool bGoHome)
{
	if (MusicEventInst.Instance)
	{
		MusicEventInst.Instance->stop(bGoHome ? FMOD_STUDIO_STOP_IMMEDIATE : FMOD_STUDIO_STOP_ALLOWFADEOUT);
		MusicEventInst.Instance->release();
	}
}

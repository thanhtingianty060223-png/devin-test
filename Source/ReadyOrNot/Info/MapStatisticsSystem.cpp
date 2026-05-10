// Copyright Void Interactive, 2022


#include "MapStatisticsSystem.h"

#include "Characters/CyberneticController.h"
#include "Components/MoraleComponent.h"

DEFINE_LOG_CATEGORY(LogInternalAnalytics);

TArray<AReadyOrNotCharacter*> AMapStatisticsSystem::GetActors() const
{
	TArray<AReadyOrNotCharacter*> Actors;
	for (TActorIterator<AReadyOrNotCharacter>It(GetWorld()); It; ++It)
	{
		Actors.Push(*It);
	}
	return Actors;
}

void AMapStatisticsSystem::NotifyNewActor(const FGuid InGameId, int8 InActorId, AActor* InActor)
{
	const UReadyOrNotBackend* BackEnd = UReadyOrNotStatics::GetReadyOrNotGameInstance()->ReadyOrNotBackend;
	if(BackEnd)
	{
		TMap<FString, FString> Properties;
		if(Cast<ACyberneticCharacter>(InActor))
		{
			const UAIArchetypeData* ArchType = Cast<ACyberneticCharacter>(InActor)->Archetype;
			if(ArchType)
				Properties.Add("archType", ArchType->GetName());
		}
		BackEnd->OnMapAnalyticsActorAdded(InGameId, InActorId, InActor, Properties);
	}
	else
	{
		ensure(false);
		UE_LOG_ONLINE(Fatal, TEXT("Cannot find backend"));
	}
}

void AMapStatisticsSystem::NotifyGameStart(const FGuid InGameId, FString InLevelName, FString InMode)
{
	const UReadyOrNotBackend* BackEnd = UReadyOrNotStatics::GetReadyOrNotGameInstance()->ReadyOrNotBackend;
	if(BackEnd)
	{
		BackEnd->OnMapAnalyticsGameStarted(InGameId, MAP_STATISTICS_PROTOCOL_VERSION, InLevelName, InMode);
	}
	else
	{
		ensure(false);
		UE_LOG_ONLINE(Fatal, TEXT("Cannot find backend"));
	}
}

void AMapStatisticsSystem::NotifyGameData(const FGuid InGameId, uint32 PacketIndex, TArray<FAnalyticsStatus>& InStatuses, bool HasGameEnded)
{
	if(InStatuses.Num() == 0)
		return;
		
	const UReadyOrNotBackend* BackEnd = UReadyOrNotStatics::GetReadyOrNotGameInstance()->ReadyOrNotBackend;
	if(!BackEnd)
	{
		ensure(false);
		UE_LOG_ONLINE(Fatal, TEXT("Cannot find backend"));
	}
	
	FBufferArchive BufferArchive;

	// Write the number first
	int32 ItemCount = InStatuses.Num();
	BufferArchive << ItemCount;
	
	// Now we're done with the headers, write all the status values
	for(auto Status : InStatuses)
	{
		BufferArchive << Status.ActorId;
		BufferArchive << Status.TickOffset;
		BufferArchive << Status.Position.X;
		BufferArchive << Status.Position.Y;
		BufferArchive << Status.Position.Z;
		BufferArchive << Status.Heading;
		BufferArchive << Status.Team;
		BufferArchive << Status.State;
		BufferArchive << Status.bHasSuspectState;
		if(Status.bHasSuspectState)
		{
			BufferArchive << Status.SuspectState.State;
			BufferArchive << Status.SuspectState.Morale;
			BufferArchive << Status.SuspectState.AwarenessState;
			
			if((Status.SuspectState.State & ESuspectStateData::SSD_IS_TRACKING) == ESuspectStateData::SSD_IS_TRACKING)
				BufferArchive << Status.SuspectState.TrackingActorId;
					

			if((Status.SuspectState.State & ESuspectStateData::SSD_HAS_BEST_ACTION) == ESuspectStateData::SSD_HAS_BEST_ACTION)
			{
				BufferArchive << Status.SuspectState.BestAction;
				BufferArchive << Status.SuspectState.BestActionScore;	
			}
			
			if((Status.SuspectState.State & ESuspectStateData::SSD_HAS_BEST_CONTINUOUS_ACTION) == ESuspectStateData::SSD_HAS_BEST_CONTINUOUS_ACTION)
			{
				BufferArchive << Status.SuspectState.BestContinuousAction;
				BufferArchive << Status.SuspectState.BestContinuousActionScore;
			}
			
			if((Status.SuspectState.State & ESuspectStateData::SSD_HAS_BEST_COMBAT_MOVE_ACTION) == ESuspectStateData::SSD_HAS_BEST_COMBAT_MOVE_ACTION)
			{
				BufferArchive << Status.SuspectState.BestCombatMoveAction;
				BufferArchive << Status.SuspectState.BestCombatMoveActionScore;
			}
			
		}
	}
	
	FBufferArchive TempCompressedMemory;
	int32 UncompressedSize = BufferArchive.Num() * BufferArchive.GetTypeSize();
	TempCompressedMemory.Empty(UncompressedSize * 4 / 3);
	TempCompressedMemory.AddUninitialized(UncompressedSize * 4 / 3);
	int32 CompressedSize = TempCompressedMemory.Num() * TempCompressedMemory.GetTypeSize();
	if(!FCompression::CompressMemory(NAME_Zlib, TempCompressedMemory.GetData(), CompressedSize, BufferArchive.GetData(), BufferArchive.Num(), COMPRESS_BiasSpeed))
	{
		UE_LOG_ONLINE(Fatal, TEXT("Failed to compress data"));
	}
	else
	{
		BackEnd->OnMapAnalyticsGameData(InGameId, PacketIndex, HasGameEnded, TempCompressedMemory);
		
		// const FString FilePath = FPaths::CreateTempFilename(*FPaths::ProjectUserDir(), TEXT("metrics"), TEXT(".dat"));
		//
		// IFileHandle* Handle = IPlatformFile::GetPlatformPhysical().OpenWrite(*FilePath, false, false);
		// if (!Handle)
		// {
		// 	UE_LOG_ONLINE(Fatal, TEXT("Failed to create file: %s"), *FilePath);
		// }
		// else
		// {
		// 	if(bCompressData)
		// 		Handle->Write(TempCompressedMemory.GetData(), CompressedSize);
		// 	else
		// 		Handle->Write(BufferArchive.GetData(), BufferArchive.Num());
		// 		
		// 	
		// 	delete Handle;
		// 	Handle = nullptr;
		// }
	}
	
	GEngine->AddOnScreenDebugMessage(154,
		10.0f,
		FColor::Red,
		FString::Printf(TEXT("Map Statistics Sent")));

}

AMapStatisticsSystem::AMapStatisticsSystem()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = TICK_RATE;
}

void AMapStatisticsSystem::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AMapStatisticsSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if(!bIsRecording)
		return;

	// We have time to trigger the data save
	if(Statuses.Num() >= PACKET_STATUS_SIZE)
	{
		// Send some data, and clear the array - game is still going though
		NotifyGameData(GameId, PacketIndex++, Statuses, false);
		Statuses.Empty(PACKET_STATUS_SIZE);
	}
	
	TArray<AReadyOrNotCharacter*> Actors = GetActors();
	const uint32 TickOffset = FDateTime::Now().GetTicks() - StartTick;

	for(const auto Actor : Actors)
	{
		const FString ActorName = UKismetSystemLibrary::GetObjectName(Actor);
		if(!ActorIdMap.Contains(ActorName))
		{
			const int8 ActorId = CurrentActorId++;
			ActorIdMap.Add(ActorName, ActorId);
			NotifyNewActor(GameId, ActorId, Actor);
		}
		
		FAnalyticsStatus Status;
		Status.ActorId = ActorIdMap[ActorName];
		Status.TickOffset = TickOffset;
		Status.Position = Actor->GetActorLocation();
		Status.Heading = Actor->GetActorRotation().Yaw;
		Status.Team = Actor->GetTeam();
		
		if(Actor->IsDeadNotUnconscious())
		{
			Status.State = EActorAnalyticsState::AAS_Dead;
		}
		else if(Actor->IsArrested())
		{
			Status.State = EActorAnalyticsState::AAS_Arrested;
		}
		else if(!Actor->IsFullHealth())
		{
			Status.State = EActorAnalyticsState::AAS_Wounded;
		}
		else
		{
			Status.State = EActorAnalyticsState::AAS_None;
		}

		Status.bHasSuspectState = false;
		Status.SuspectState.State = ESuspectStateData::SSD_NONE;
		if(Cast<ACyberneticCharacter>(Actor) && IsValid(Actor))
		{
			ACyberneticController* const Controller = Cast<ACyberneticController>(Actor->GetController());
			if(Controller)
			{
			
				Status.bHasSuspectState = true;
				Status.SuspectState.AwarenessState = Controller->GetAwarenessState();
				
				const UMoraleComponent* MoraleComponent = Controller->GetMoraleComp();
				if(MoraleComponent)
				{
					Status.SuspectState.Morale = MoraleComponent->GetMorale();
				}
				else
				{
					Status.SuspectState.Morale = 0.0f;
				}
				const AReadyOrNotCharacter* TrackingTarget = Controller->GetTrackedTarget();
				if(TrackingTarget)
				{
					Status.SuspectState.State |= ESuspectStateData::SSD_IS_TRACKING;
					const FString TrackingName = UKismetSystemLibrary::GetObjectName(TrackingTarget);
					if(!ActorIdMap.Contains(TrackingName))
					{
						const int8 ActorId = CurrentActorId++;
						ActorIdMap.Add(TrackingName, ActorId);
						NotifyNewActor(GameId, ActorId, Actor);
					}
					Status.SuspectState.TrackingActorId = ActorIdMap[TrackingName];
				}
			
				if(Controller->BestAction)
				{
					if (const float* ScorePtr = Controller->BestAction->Scores.Find(Controller))
					{
						Status.SuspectState.State |= ESuspectStateData::SSD_HAS_BEST_ACTION;
						Status.SuspectState.BestAction = *Controller->BestAction->Name.ToString();
						Status.SuspectState.BestActionScore = *ScorePtr;
					}
				}
				
				if(Controller->BestContinuousAction)
				{
					if (const float* ScorePtr = Controller->BestContinuousAction->Scores.Find(Controller))
					{
						Status.SuspectState.State |= ESuspectStateData::SSD_HAS_BEST_CONTINUOUS_ACTION;
						Status.SuspectState.BestContinuousAction = *Controller->BestContinuousAction->Name.ToString();
						Status.SuspectState.BestContinuousActionScore = *ScorePtr;
					}
				}
				
				if(Controller->BestCombatMoveAction)
				{
					if (const float* ScorePtr = Controller->BestCombatMoveAction->Scores.Find(Controller))
					{
						Status.SuspectState.State |= ESuspectStateData::SSD_HAS_BEST_COMBAT_MOVE_ACTION;
						Status.SuspectState.BestCombatMoveAction = *Controller->BestCombatMoveAction->Name.ToString();
						Status.SuspectState.BestCombatMoveActionScore = *ScorePtr;
					}
				}
				
			}

			
		}
				
		GEngine->AddOnScreenDebugMessage(155 + Status.ActorId + 1,
					1.0f,
					FColor::Emerald,
					FString::Printf(TEXT("Actor: %s [%.2f,%.2f,%.2f] H: %.2f. Team: %s State: %s"),
						*ActorName,
						Status.Position.X,
						Status.Position.Y,
						Status.Position.Z,
						Status.Heading,
						*TeamTypeEnumToString(Status.Team),
						*UEnum::GetDisplayValueAsText(Status.State).ToString()));

		Statuses.Push(Status);
	}

	if(Statuses.Num())
	{
		constexpr auto Size = sizeof(Statuses[0]);
		const auto TotalSize = Size * Statuses.Num();

		const FTimespan Span(TickOffset);
		
		GEngine->AddOnScreenDebugMessage(155,
			1.0f,
			FColor::Emerald,
			FString::Printf(TEXT("Map Statistics Recording, %i Items. %.2f kb. Total time: %s (%.2f kb per min per actor)"), Statuses.Num(), TotalSize/1024.0, *Span.ToString(), BytesPerMinutePerActor / 1024.0f));
	}
}

FString AMapStatisticsSystem::GetRecordingStatus() const
{
#if defined(ENABLE_MAP_STATISTICS)
	if(bIsDisabled)
	{
		return "Map Analytics disabled in options";
	}
	else if(!bIsRecording)
	{
		return "Map Analytics not recording";
	}
	else
	{
		return FString::Format(TEXT("Map Analytics recording (ID: {0})"), { *GameId.ToString() });
	}
#else
	return "Map Analytics disabled at compile time.";
#endif
}

void AMapStatisticsSystem::StartRecording(const FString& InLevelName, const FString& InGameMode)
{
#if defined(ENABLE_MAP_STATISTICS)
	// Check we are able to send statistics
	const UReadyOrNotGameUserSettings* Settings = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (!Settings || !Settings->bSendMapStatistics)
	{
		bIsDisabled = true;
		UE_LOG(LogReadyOrNot, Log, TEXT("Not allowed to send Map Statistics"))
		return ;
	}
	bIsDisabled = false;
	
	ensure(!bIsRecording);
	if(!InLevelName.IsEmpty())
	{
		Statuses.Empty(PACKET_STATUS_SIZE);
		ActorIdMap.Empty();
		PacketIndex = 0;
		CurrentActorId = 0;
		GameId = FGuid::NewGuid();
		bIsRecording = true;

		StartTick = FDateTime::Now().GetTicks();
		NotifyGameStart(GameId, InLevelName, InGameMode);
	}
#else
	bIsDisabled = true;
	UE_LOG(LogReadyOrNot, Log, TEXT("Not allowed to send Map Statistics - ENABLE_MAP_STATISTICS not defined"))
#endif
}

void AMapStatisticsSystem::EndLevel()
{
	if(bIsRecording)
	{
		// Send the last of the game data, and let the server know the game has ended
		NotifyGameData(GameId, PacketIndex++, Statuses, true);
		bIsRecording = false;
		Statuses.Empty(PACKET_STATUS_SIZE);
	}
}


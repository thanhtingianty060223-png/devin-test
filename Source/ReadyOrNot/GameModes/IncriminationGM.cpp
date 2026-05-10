// Void Interactive, 2020

#include "IncriminationGM.h"
#include "IncriminationGS.h"

#include "Actors/EvidenceSpawnPoint.h"
#include "Actors/EvidenceExtractionDevice_Incrim.h"
#include "Actors/IncriminationClueSpawnPoint.h"
#include "Actors/SpawnGenerator.h"

#include "Actors/Gameplay/EvidenceActor.h"
#include "Actors/Gameplay/IncriminationClue.h"

#include "Actors/Splines/SplineTrigger_Incrimination.h"
#include "Actors/Triggers/BuildingTrigger_Incrimination.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

void AIncriminationGM::OnRoundStarted_Implementation()
{
	Super::OnRoundStarted_Implementation();

	InitializeIncriminationRound();
}

void AIncriminationGM::OnRoundEnded_Implementation()
{
	ChosenSpawnGroup_BlueTeam = nullptr;
	ChosenSpawnGroup_RedTeam = nullptr;

	Super::OnRoundEnded_Implementation();
}

void AIncriminationGM::InitializeIncriminationRound()
{
	ChooseEvidenceSpawn();
	SpawnEvidenceActor();

	GatherAllClueSpawnsFromChosenEvidenceBuilding();
	SpawnChosenClues();

	AssignRandomNonMainEvidenceSearchZones();

	ChooseExtractionDevice();
}

bool AIncriminationGM::ChooseEvidenceSpawn()
{
	if (AIncriminationGS* IncriminationGS = GetGameState<AIncriminationGS>())
	{
		TArray<AEvidenceSpawnPoint*> EvidenceSpawns = UReadyOrNotFunctionLibrary::GetActorsOfClass<AEvidenceSpawnPoint>(GetWorld());

		AEvidenceSpawnPoint* ChosenEvidenceSpawn = nullptr;
		
		if (EvidenceSpawns.Num() > 0)
		{
			if (HasVisitedAllEvidenceSpawns())
			{
				ResetAllEvidenceSpawnVisits();
			}

			// Choose a random unvisited spawn
			do
			{
				ChosenEvidenceSpawn = EvidenceSpawns[FMath::RandRange(0, EvidenceSpawns.Num() - 1)];
			}
			while (ChosenEvidenceSpawn->bHasVisited);

			ChosenEvidenceSpawn->bHasVisited = true;
		}
		
		IncriminationGS->ChosenEvidenceSpawn = ChosenEvidenceSpawn;

		return ChosenEvidenceSpawn != nullptr;
	}

	return false;
}

void AIncriminationGM::SpawnEvidenceActor()
{
	if (AIncriminationGS* IncriminationGS = GetGameState<AIncriminationGS>())
	{
		if (IncriminationGS->ChosenEvidenceSpawn)
		{
			if (IncriminationGS->ChosenEvidenceActor)
			{
				IncriminationGS->ChosenEvidenceActor->Destroy();
				IncriminationGS->ChosenEvidenceActor = nullptr;
			}
			
			IncriminationGS->ChosenEvidenceActor = GetWorld()->SpawnActor<AEvidenceActor>(IncriminationGS->ChosenEvidenceSpawn->EvidenceActorClass, IncriminationGS->ChosenEvidenceSpawn->GetActorTransform());
			if (IncriminationGS->ChosenEvidenceActor)
			{
				IncriminationGS->ChosenEvidenceSearchArea = IncriminationGS->ChosenEvidenceSpawn->EvidenceSearchArea;
				IncriminationGS->ChosenEvidenceBuilding = IncriminationGS->ChosenEvidenceSpawn->EvidenceBuilding;
				IncriminationGS->CurrentExtractionDevice = nullptr;
				
				IncriminationGS->PickupTeam = ETeamType::TT_NONE;
				IncriminationGS->IntelState = EEvidenceActorState::Unclaimed;
				IncriminationGS->bIntelExtracted = false;
			
				IncriminationGS->ChosenEvidenceActor->HideObjectiveMarker();

				IncriminationGS->ChosenEvidenceActor->OnActorPickedUp.Remove(this, "OnEvidencePickedUp");
				IncriminationGS->ChosenEvidenceActor->OnActorPickedUp.AddDynamic(this, &AIncriminationGM::OnEvidencePickedUp);
				
				IncriminationGS->ChosenEvidenceActor->OnActorDropped.Remove(this, "OnEvidenceDropped");
				IncriminationGS->ChosenEvidenceActor->OnActorDropped.AddDynamic(this, &AIncriminationGM::OnEvidenceDropped);

				IncriminationGS->ChosenEvidenceActor->SetActorLocation(IncriminationGS->ChosenEvidenceSpawn->GetActorLocation());
			}
		}
	}
}

void AIncriminationGM::SpawnChosenClues()
{
	if (AIncriminationGS* IncriminationGS = GetGameState<AIncriminationGS>())
	{
		for (AIncriminationClue* Clue : IncriminationGS->Clues)
		{
			Clue->Destroy();
			Clue = nullptr;
		}

		IncriminationGS->Clues.Empty(MaxCluesToFind);
		
		if (IncriminationGS->ClueSpawnPoints.Num() > 0)
		{
			TArray<AIncriminationClue*> SpawnedClues;
			for (AIncriminationClueSpawnPoint* ClueSpawnPoint : IncriminationGS->ClueSpawnPoints)
			{
				if (ClueSpawnPoint)
				{
					AIncriminationClue* Clue = GetWorld()->SpawnActor<AIncriminationClue>(ClueSpawnPoint->IncriminationClueClass, ClueSpawnPoint->GetActorTransform());
					if (Clue)
					{
						Clue->Init(ClueSpawnPoint, ClueSpawnPoint->ClueNumber, ClueSpawnPoint->ClueName, ClueSpawnPoint->ClueFoundMessage, ClueSpawnPoint->ShowObjectiveMarkerIn);
						//Clue->HideClue();

						SpawnedClues.Add(Clue);
					}
				}
			}

			if (SpawnedClues.Num() > 0)
			{
				// Assign each clue to their next
				for (int32 i = 0; i < SpawnedClues.Num(); i++)
				{
					if (SpawnedClues.IsValidIndex(i+1))
						SpawnedClues[i]->SetNextClue(SpawnedClues[i+1]);
				}
				
				IncriminationGS->Clues = SpawnedClues;

				// Show the first clue
				IncriminationGS->ActiveClue = SpawnedClues[0];
				
				IncriminationGS->ActiveClue->Delegate_OnClueFound.Remove(this, "OnClueFound");
				IncriminationGS->ActiveClue->Delegate_OnClueFound.AddDynamic(this, &AIncriminationGM::OnClueFound);
				
				IncriminationGS->ActiveClue->ShowClue();
			}
		}
	}
}

void AIncriminationGM::AssignRandomNonMainEvidenceSearchZones()
{
	if (AIncriminationGS* IncriminationGS = GetGameState<AIncriminationGS>())
	{
		// Get all clues in world
		TArray<ASplineTrigger_Incrimination*> AllSplineTriggers;
		
		for (TActorIterator<ASplineTrigger_Incrimination> It(GetWorld()); It; ++It)
		{
			ASplineTrigger_Incrimination* SplineTrigger = *It;
			if (SplineTrigger)
			{
				AllSplineTriggers.Add(SplineTrigger);
			}
		}

		AllSplineTriggers.Remove(IncriminationGS->ChosenEvidenceSearchArea);

		TArray<ASplineTrigger_Incrimination*> RandomSplineTriggers;
		RandomSplineTriggers.Reserve(AllSplineTriggers.Num());

		for (int32 i = 0; i < 2; i++) // Make variable, if game design needs it?
		{
			const int32 RandIndex = FMath::RandRange(0, AllSplineTriggers.Num() - 1);
			if (AllSplineTriggers.IsValidIndex(RandIndex))
			{
				ASplineTrigger_Incrimination* ChosenRandomSpline = AllSplineTriggers[RandIndex];
				RandomSplineTriggers.Add(ChosenRandomSpline);
				AllSplineTriggers.Remove(ChosenRandomSpline);
			}
		}
		
		IncriminationGS->NonMainIntelSearchZones = RandomSplineTriggers;
	}
}

void AIncriminationGM::GatherAllClueSpawnsInLevel()
{
	if (AIncriminationGS* IncriminationGS = GetGameState<AIncriminationGS>())
	{
		// Get all clues in world
		TArray<AIncriminationClueSpawnPoint*> AllClues;
		AllClues.Reserve(MaxCluesToFind);
		
		for (TActorIterator<AIncriminationClueSpawnPoint> It(GetWorld()); It; ++It)
		{
			AIncriminationClueSpawnPoint* Clue = *It;
			if (Clue)
			{
				AllClues.Add(Clue);
			}
		}

		// Choose a random clue 1, 2 and 3 clue spawn
		TArray<TArray<AIncriminationClueSpawnPoint*>> SortedClues;
		SortedClues.Reserve(MaxCluesToFind);

		for (int32 i = 0; i < MaxCluesToFind; i++)
		{
			SortedClues.Add(GetAllClueSpawnsOfClueNumber(AllClues, i + 1));
		}

		TArray<AIncriminationClueSpawnPoint*> ChosenClues;
		ChosenClues.Reserve(MaxCluesToFind);
		
		for (int32 i = 0; i < SortedClues.Num(); i++)
		{
			if (SortedClues[i].Num() > 0)
				ChosenClues.Add(SortedClues[i][FMath::RandRange(0, SortedClues[i].Num() - 1)]); // Choose a random clue (i) clue spawn
		}

		IncriminationGS->ClueSpawnPoints = ChosenClues;
	}
}

void AIncriminationGM::GatherAllClueSpawnsFromChosenEvidenceBuilding()
{
	if (AIncriminationGS* IncriminationGS = GetGameState<AIncriminationGS>())
	{
		// Get all clues from chosen evidence building
		TArray<AIncriminationClueSpawnPoint*> AllClues;
		AllClues.Reserve(MaxCluesToFind);

		if (IncriminationGS->ChosenEvidenceBuilding)
		{
			AllClues = IncriminationGS->ChosenEvidenceBuilding->ClueSpawnPoints;
		}

		IncriminationGS->ClueSpawnPoints = AllClues;

		// Choose a random clue 1, 2 and 3 clue spawn
		//TArray<TArray<AIncriminationClueSpawnPoint*>> SortedClues;
		//SortedClues.Reserve(MaxCluesToFind);
		//
		//for (int32 i = 0; i < MaxCluesToFind; i++)
		//{
		//	SortedClues.Add(GetAllClueSpawnsOfClueNumber(AllClues, i + 1));
		//}
		//
		//TArray<AIncriminationClueSpawnPoint*> ChosenClues;
		//ChosenClues.Reserve(MaxCluesToFind);
		//
		//for (int32 i = 0; i < SortedClues.Num(); i++)
		//{
		//	if (SortedClues[i].Num() > 0)
		//		ChosenClues.Add(SortedClues[i][FMath::RandRange(0, SortedClues[i].Num() - 1)]); // Choose a random clue (i) clue spawn
		//}
		//
		//#if WITH_EDITOR
		//ensureAlways(ChosenClues.Num() == MaxCluesToFind);
		//if (ChosenClues.Num() < MaxCluesToFind)
		//	ULog::Error("Only " + FString::FromInt(ChosenClues.Num()) + "/" + FString::FromInt(MaxCluesToFind) + " were found. " + FString::FromInt(MaxCluesToFind - ChosenClues.Num()) + " clues are missing. Incrimination will fail to function properly!");
		//#endif
		//
		//IncriminationGS->ClueSpawnPoints = ChosenClues;
	}
}

void AIncriminationGM::CheckVictoryConditions()
{
	if (AreAllPlayersOnTeamDead(ETeamType::TT_SERT_RED))
	{
		if (GetGameState<AIncriminationGS>()->PickupTeam == ETeamType::TT_NONE)
		{
			RoundWonTeam(ETeamType::TT_NONE);
			return;
		}
		
		// all players on red dead = blue victory
		RoundWonTeam(GetGameState<AIncriminationGS>()->PickupTeam == ETeamType::TT_SERT_RED ? ETeamType::TT_SERT_RED : ETeamType::TT_SERT_BLUE);

		return;
	}

	if (AreAllPlayersOnTeamDead(ETeamType::TT_SERT_BLUE))
	{
		if (GetGameState<AIncriminationGS>()->PickupTeam == ETeamType::TT_NONE)
		{
			RoundWonTeam(ETeamType::TT_NONE);
			return;
		}
		
		// all players on blue dead = red victory
		RoundWonTeam(GetGameState<AIncriminationGS>()->PickupTeam == ETeamType::TT_SERT_BLUE ? ETeamType::TT_SERT_BLUE : ETeamType::TT_SERT_RED);

		return;
	}
		
	if (AreAllPlayersOnTeamArrested(ETeamType::TT_SERT_RED) || AreAllPlayersOnTeamDowned(ETeamType::TT_SERT_RED))
	{
		// all players on red arrested = blue victory
		RoundWonTeam(ETeamType::TT_SERT_BLUE);

		return;
	}

	if (AreAllPlayersOnTeamArrested(ETeamType::TT_SERT_BLUE) || AreAllPlayersOnTeamDowned(ETeamType::TT_SERT_BLUE))
	{
		// all players on blue dead = red victory
		RoundWonTeam(ETeamType::TT_SERT_RED);

		return;
	}
}

void AIncriminationGM::ResetAllEvidenceSpawnVisits()
{
	TArray<AEvidenceSpawnPoint*> EvidenceSpawns = UReadyOrNotFunctionLibrary::GetActorsOfClass<AEvidenceSpawnPoint>(GetWorld());

	for (auto* EvidenceSpawn : EvidenceSpawns)
	{
		if (EvidenceSpawn)
		{
			EvidenceSpawn->bHasVisited = false;
		}
	}
}

bool AIncriminationGM::ChooseExtractionDevice()
{
	if (AIncriminationGS* IncrimGS = GetGameState<AIncriminationGS>())
	{
		TArray<AEvidenceExtractionDevice_Incrim*> ExtractionDevices = UReadyOrNotFunctionLibrary::GetActorsOfClass<AEvidenceExtractionDevice_Incrim>(GetWorld());

		if (ExtractionDevices.Num() > 0)
		{
			AEvidenceExtractionDevice_Incrim* ChosenExtractionDevice = ExtractionDevices[FMath::RandRange(0, ExtractionDevices.Num() - 1)];

			IncrimGS->ChosenExtractionDevice = ChosenExtractionDevice;

			return IncrimGS->ChosenExtractionDevice != nullptr;
		}

		return false;
	}
	
	return false;
}

TArray<AIncriminationClueSpawnPoint*> AIncriminationGM::GetAllClueSpawnsOfClueNumber(const TArray<AIncriminationClueSpawnPoint*>& InClueSpawns, const int32 ClueNumber)
{
	TArray<AIncriminationClueSpawnPoint*> FoundClueSpawns;
	for (AIncriminationClueSpawnPoint* Clue : InClueSpawns)
	{
		if (Clue)
		{
			if (Clue->ClueNumber == ClueNumber)
			{
				FoundClueSpawns.Add(Clue);
			}
		}
	}

	return FoundClueSpawns;
}

ASpawnGenerator* AIncriminationGM::FindTeamSpawnGroup(const ETeamType& Team) const
{
	if (Team == ETeamType::TT_NONE)
		return nullptr;
	
	// Get all spawn generators in the level
	TArray<AActor*> FoundSpawnGenerators;
	UGameplayStatics::GetAllActorsOfClass(this, ASpawnGenerator::StaticClass(), FoundSpawnGenerators);

	if (FoundSpawnGenerators.Num() == 0)
		return nullptr;

	TArray<ASpawnGenerator*> ChosenSpawnGroups;

	for (AActor* Actor : FoundSpawnGenerators)
	{
		ASpawnGenerator* SpawnGenerator = Cast<ASpawnGenerator>(Actor);
		if (SpawnGenerator && SpawnGenerator->GetSpawnTeam() == Team)
			ChosenSpawnGroups.Add(SpawnGenerator);
	}

	return ChosenSpawnGroups[FMath::RandRange(0, ChosenSpawnGroups.Num() - 1)];
}

APlayerStart* AIncriminationGM::FindPlayerStartFromSpawnGroup(ASpawnGenerator* SpawnGenerator) const
{
	// Choose a random player start from the chosen spawn generator
	if (SpawnGenerator)
	{
		TArray<APlayerStart*> PlayerStarts = SpawnGenerator->GetAllPlayerStarts();

		return PlayerStarts[FMath::RandRange(0, PlayerStarts.Num() - 1)];
	}

	return nullptr;
}

bool AIncriminationGM::HasVisitedAllEvidenceSpawns() const
{
	TArray<AEvidenceSpawnPoint*> EvidenceSpawns = UReadyOrNotFunctionLibrary::GetActorsOfClass<AEvidenceSpawnPoint>(GetWorld());

	bool bHasVisitedAllSpawns = true;
	for (auto* EvidenceSpawn : EvidenceSpawns)
	{
		if (EvidenceSpawn && !EvidenceSpawn->bHasVisited)
		{
			bHasVisitedAllSpawns = false;
			break;
		}
	}

	return bHasVisitedAllSpawns;
}

void AIncriminationGM::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (AIncriminationGS* IncriminationGS = GetGameState<AIncriminationGS>())
	{
		AEvidenceActor* ChosenEvidenceActor = IncriminationGS->ChosenEvidenceActor;
		AEvidenceSpawnPoint* ChosenEvidenceSpawn = IncriminationGS->ChosenEvidenceSpawn;
		
		if (ChosenEvidenceActor && ChosenEvidenceSpawn)
		{
			// Perform fail-safe 1
			if (UReadyOrNotFunctionLibrary::IsActorOutsideSplineEnclosure(ChosenEvidenceSpawn->EvidenceSearchArea, ChosenEvidenceActor) && IncriminationGS->IntelState == EEvidenceActorState::Unclaimed)
			{
				ChosenEvidenceActor->SetActorLocation(ChosenEvidenceSpawn->GetActorLocation());
			}

			// Perform fail-safe 2
			if (ChosenEvidenceActor->GetActorLocation().Size() > 100000.0f)
			{
				ChosenEvidenceActor->SetActorLocation(ChosenEvidenceSpawn->GetActorLocation());
			}

			// Perform fail-safe 3
			//if (!ChosenEvidenceActor->GetActorLocation().Equals(ChosenEvidenceSpawn->GetActorLocation(), 100.0f) && IncriminationGS->IntelState == EEvidenceActorState::Unclaimed)
			//{
			//	ChosenEvidenceActor->SetActorLocation(ChosenEvidenceSpawn->GetActorLocation());
			//}
		}
	}
}

AActor* AIncriminationGM::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!Player)
	{
		return nullptr;
	}

	if (AReadyOrNotPlayerState* PS = Cast<AReadyOrNotPlayerState>(Player->PlayerState))
	{
		switch (PS->GetTeam())
		{
			case ETeamType::TT_SERT_RED:
				return FindPlayerStartFromSpawnGroup(ChosenSpawnGroup_RedTeam);

			case ETeamType::TT_SERT_BLUE:
				return FindPlayerStartFromSpawnGroup(ChosenSpawnGroup_BlueTeam);

			default:
				return FindPlayerStartFromSpawnGroup(ChosenSpawnGroup_BlueTeam);
		}
	}

	return FindPlayerStartWithTag(SWATBlueStartTag);
}

APlayerStart* AIncriminationGM::FindPlayerStartWithTag(const FName& Tag) const
{
	if (Tag.IsNone())
		return nullptr;

	// Get all spawn generators in the level
	TArray<AActor*> FoundSpawnGenerators;
	UGameplayStatics::GetAllActorsOfClass(this, ASpawnGenerator::StaticClass(), FoundSpawnGenerators);

	if (FoundSpawnGenerators.Num() == 0)
		return Super::FindPlayerStartWithTag(Tag);

	// Separate them into 2 arrays
	TArray<ASpawnGenerator*> BlueTeamSpawnGroups;
	TArray<ASpawnGenerator*> RedTeamSpawnGroups;

	ETeamType TagToTeamType = ETeamType::TT_NONE;

	if (Tag == SWATBlueStartTag)
	{
		TagToTeamType = ETeamType::TT_SERT_BLUE;

		for (AActor* Actor : FoundSpawnGenerators)
		{
			ASpawnGenerator* SpawnGenerator = Cast<ASpawnGenerator>(Actor);
			if (SpawnGenerator && SpawnGenerator->GetSpawnTeam() == ETeamType::TT_SERT_BLUE)
				BlueTeamSpawnGroups.Add(SpawnGenerator);
		}
	}
	else if (Tag == SWATRedStartTag)
	{
		TagToTeamType = ETeamType::TT_SERT_RED;
		
		for (AActor* Actor : FoundSpawnGenerators)
		{
			ASpawnGenerator* SpawnGenerator = Cast<ASpawnGenerator>(Actor);
			if (SpawnGenerator && SpawnGenerator->GetSpawnTeam() == ETeamType::TT_SERT_RED)
				RedTeamSpawnGroups.Add(SpawnGenerator);
		}
	}

	if (TagToTeamType == ETeamType::TT_NONE)
		return Super::FindPlayerStartWithTag(Tag);

	TArray<ASpawnGenerator*> ChosenSpawnGroups = (TagToTeamType == ETeamType::TT_SERT_BLUE ? BlueTeamSpawnGroups : RedTeamSpawnGroups);

	// Choose a random spawn generator of the same team as the tag
	ASpawnGenerator* SpawnGenerator;
	{
		const uint32 OriginalCount = ChosenSpawnGroups.Num();
		
		uint32 LoopCount = 0;
		do
		{
			SpawnGenerator = Cast<ASpawnGenerator>(ChosenSpawnGroups[FMath::RandRange(0, ChosenSpawnGroups.Num() - 1)]);

			LoopCount++;
			if (LoopCount > OriginalCount) // Failsafe
				break;

			ChosenSpawnGroups.Remove(SpawnGenerator);
			ChosenSpawnGroups.Shrink();
		}
		while (SpawnGenerator && SpawnGenerator->GetSpawnTeam() != TagToTeamType);
	}

	// Choose a random player start from the chosen spawn generator
	if (SpawnGenerator)
	{
		TArray<APlayerStart*> PlayerStarts = SpawnGenerator->GetAllPlayerStarts();
		const uint32 OriginalCount = PlayerStarts.Num();

		APlayerStart* ChosenPlayerStart;
		{
			uint32 LoopCount = 0;
			do
			{
				ChosenPlayerStart = PlayerStarts[FMath::RandRange(0, PlayerStarts.Num() - 1)];
				
				LoopCount++;
				if (LoopCount > OriginalCount) // Failsafe
					break;

				PlayerStarts.Remove(ChosenPlayerStart);
				PlayerStarts.Shrink();
			}
			while (ChosenPlayerStart && ChosenPlayerStart->PlayerStartTag != Tag);
		}

		return ChosenPlayerStart != nullptr ? ChosenPlayerStart : Super::FindPlayerStartWithTag(Tag);
	}

	return Super::FindPlayerStartWithTag(Tag);
}

void AIncriminationGM::RespawnPlayer(APlayerController* Player, const bool bForceSpectator)
{
	if (!ChosenSpawnGroup_BlueTeam)
		ChosenSpawnGroup_BlueTeam = FindTeamSpawnGroup(ETeamType::TT_SERT_BLUE);
		
	if (!ChosenSpawnGroup_RedTeam)
		ChosenSpawnGroup_RedTeam = FindTeamSpawnGroup(ETeamType::TT_SERT_RED);
		
	Super::RespawnPlayer(Player, bForceSpectator);
}

bool AIncriminationGM::ShouldSpawnAtStartSpot(AController* Player)
{
	return false;
}

void AIncriminationGM::OnEvidencePickedUp_Implementation(AActor* PickupActor)
{
	if (AIncriminationGS* IncriminationGS = GetGameState<AIncriminationGS>())
	{
		if (AEvidenceActor* EvidenceActor = Cast<AEvidenceActor>(PickupActor))
		{
			if (APlayerCharacter* PC = Cast<APlayerCharacter>(EvidenceActor->GetPickupInstigator()))
			{
				IncriminationGS->PickupTeam = PC->GetTeam();
				IncriminationGS->IntelState = EEvidenceActorState::Collected;

				for (AIncriminationClue* Clue : IncriminationGS->Clues)
				{
					if (Clue)
						Clue->HideClue();
				}
			}
		}
	}
}

void AIncriminationGM::OnEvidenceDropped_Implementation(AActor* DropActor)
{
	if (AIncriminationGS* IncriminationGS = GetGameState<AIncriminationGS>())
	{
		IncriminationGS->PickupTeam = ETeamType::TT_NONE;
		IncriminationGS->IntelState = EEvidenceActorState::Dropped;
	}
}

void AIncriminationGM::OnClueFound_Implementation(AIncriminationClue* ClueActor, AActor* ClueFounder)
{
	if (!ClueActor)
		return;

	if (AIncriminationGS* IncriminationGS = GetGameState<AIncriminationGS>())
	{
		IncriminationGS->PreviousActiveClue = ClueActor;
		
		if (ClueActor->GetNextClue())
		{
			if (ClueActor->GetNextClue()->IsClueFound())
			{
				OnClueFound_Implementation(ClueActor->GetNextClue(), ClueFounder);
				return;
			}
			
			IncriminationGS->ActiveClue = ClueActor->GetNextClue();
				
			IncriminationGS->ActiveClue->Delegate_OnClueFound.Remove(this, "OnClueFound");
			IncriminationGS->ActiveClue->Delegate_OnClueFound.AddDynamic(this, &AIncriminationGM::OnClueFound);
				
			IncriminationGS->ActiveClue->ShowClue();
		}
	}
}

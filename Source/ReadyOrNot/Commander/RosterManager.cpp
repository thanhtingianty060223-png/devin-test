// Copyright Void Interactive, 2023

#include "RosterManager.h"

#include "CommanderGM.h"
#include "CommanderProfile.h"
#include "Engine/AssetManager.h"
#include "Characters/AI/SWATCharacter.h"

// TODO(killo): just serialize all characters into one array and then sort them on load
TAutoConsoleVariable<int32> CVarRonUnlockAllTraits(TEXT("ron.RosterUnlockAllTraits"), 0, TEXT("Ignore roster trait unlocked state for all characters"));
TAutoConsoleVariable<int32> CVarRonUnlimitedFiring(TEXT("ron.RosterUnlimitedFiring"), 0, TEXT("Allow an unlimited amount of characters to be fired at once"));

template<typename T>
T GetRandomArrayItem(const TArray<T>& InArray, const FRandomStream& RandomStream)
{
	if (InArray.Num() <= 0)
		return T();

	return InArray[UKismetMathLibrary::RandomIntegerInRangeFromStream(0, InArray.Num() - 1, RandomStream)];
}

FText GetRandomTextEvent(const TArray<FText>& InEvents)
{
	if (InEvents.Num() <= 0)
		return FText::AsCultureInvariant("No record found");

	return InEvents[UKismetMathLibrary::RandomIntegerInRange(0, InEvents.Num() - 1)];
}

URosterManager::URosterManager()
{
	if (IsTemplate())
		return;

	PossibleCharacters = GetAllCharacterArchetypes();
	
	Settings = GetDefault<URosterManagerSettings>();
	for (const FRosterTraitEntry& TraitEntry : Settings->AvailableTraits)
	{
		if (!TraitEntry.bEnabled)
			continue;
	
		PossibleTraits.Add(TraitEntry.Name, TraitEntry.Trait.LoadSynchronous());
		PossibleTraits[TraitEntry.Name]->Reference = TraitEntry.Name;
	}

	EventData = Settings->EventData.LoadSynchronous();
}

void SerializeCharacters(const TArray<URosterCharacter*>& Characters, TArray<FRosterCharacterSaveData>& OutSaveData)
{
	for (URosterCharacter* Character : Characters)
	{
		if (!ensure(Character))
			continue;

		FRosterCharacterSaveData CharacterSaveData;
		FMemoryWriter MemoryWriter(CharacterSaveData.ObjectBytes, true);

		FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
		Character->Serialize(Ar);

		OutSaveData.Add(CharacterSaveData);
	}
}

void URosterManager::SaveToProfile(UCommanderProfile* Profile)
{
	if (!ensure(Profile))
		return;
	
	Profile->RosterSaveData.RosterSeed = RandomStream.GetCurrentSeed(); // Store the current roster seed, prevents cheesing the generation
	Profile->RosterSaveData.FiredCount = FiredCount;
	Profile->RosterSaveData.UsedSerialNumbers = UsedSerialNumbers;
	
	// Serialize every character and store them in the profile roster data
	Profile->RosterSaveData.Characters.Empty();
	Profile->RosterSaveData.PreviousCharacters.Empty();
	SerializeCharacters(Characters, Profile->RosterSaveData.Characters);
	SerializeCharacters(PreviousCharacters, Profile->RosterSaveData.PreviousCharacters);
}

void URosterManager::LoadFromProfile(UCommanderProfile* Profile)
{
	Characters.Empty();
	PreviousCharacters.Empty();

	CommanderProfile = Profile;
	if (!CommanderProfile)
		return;

	// Attempt to deserialize every character from the roster data and add them to the roster
	FRosterSaveData& RosterSaveData = Profile->RosterSaveData;
	for (const FRosterCharacterSaveData& CharacterSaveData : RosterSaveData.Characters)
	{
		URosterCharacter* Character = NewObject<URosterCharacter>();
		
		FMemoryReader MemoryReader(CharacterSaveData.ObjectBytes, true);
		FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
		
		Character->Serialize(Ar);
		Characters.Add(Character);
	}
	for (const FRosterCharacterSaveData& CharacterSaveData : RosterSaveData.PreviousCharacters)
	{
		URosterCharacter* Character = NewObject<URosterCharacter>();

		FMemoryReader MemoryReader(CharacterSaveData.ObjectBytes, true);
		FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
		
		Character->Serialize(Ar);
		PreviousCharacters.Add(Character);
	}
	
	// Set the seed from profile or generate it if it hasn't been generated before
	int32 Seed = RosterSaveData.RosterSeed >= 0 ? RosterSaveData.RosterSeed : FMath::Rand();
	RandomStream = FRandomStream(Seed);

	// Regenerate all characters if the flag is set (only for new profiles)
	if (RosterSaveData.bGenerateRoster)
	{
		GenerateCharacters();
		RosterSaveData.bGenerateRoster = false;
	}

	FiredCount = RosterSaveData.FiredCount;
	UsedSerialNumbers = RosterSaveData.UsedSerialNumbers;
	
	ProcessRoster();
}

void URosterManager::StartMission()
{
	// Clear out old data and set historic data
	for (URosterCharacter* Character : Characters)
	{
		Character->PreviousState = Character->State;
		Character->PreviousStressLevel = Character->StressLevel;

		Character->bTherapistIntervened = false;
		Character->bJustUnlockedTrait = false;
		
		if (Character->MissionsUntilReturn <= 0)
			Character->State = ERosterCharacterState::Available;
	}
	
	// Make sure even dead or quit characters get updated
	for (URosterCharacter* Character : PreviousCharacters)
	{
		Character->PreviousState = Character->State;
		Character->PreviousStressLevel = Character->StressLevel;
	}
}

void URosterManager::CompleteMission(const FString& MapName)
{
	TickTimers();
	
	// Increment squad character mission played tracker, stress value minimums for certain levels
	for (const TPair<ERosterSquadPosition, URosterCharacter*>& Pair : SquadCharacters)
	{
		Pair.Value->MissionsPlayed++;

		if (MapName == "ron_campus_core")
		{
			Pair.Value->StressLevel = FMath::Max(Pair.Value->StressLevel, 0.6f);
		}
	}
	ProcessTraitUnlocks();

	// Reset the fired count once a mission has been started
	FiredCount = 0;
}

void URosterManager::PreReview()
{
	// Ensure stress values stay capped once we're done with a mission
	// Do this immediately since the post mission screen with stress values is about to come up
	for (URosterCharacter* Character : GetAllCharacters())
	{
		Character->StressLevel = FMath::Clamp(Character->StressLevel, 0.0f, 1.0f);
	}
}

FRosterLoadout URosterManager::GetCharacterLoadout(URosterCharacter* Character)
{
	if (!Character)
		return FRosterLoadout();
	
	FString Position;
	switch (Character->Position)
	{
	case ERosterSquadPosition::RedOne: Position = "defaultredone"; break;
	case ERosterSquadPosition::RedTwo: Position = "defaultredtwo"; break;
	case ERosterSquadPosition::BlueOne: Position = "defaultblueone"; break;
	case ERosterSquadPosition::BlueTwo: Position = "defaultbluetwo"; break;
	default: return FRosterLoadout();
	}

	FSavedLoadout SavedLoadout;
	UBpGameplayHelperLib::LoadLoadout(SavedLoadout, Position);
	
	FRosterLoadout RosterLoadout;
	RosterLoadout.Primary = SavedLoadout.Primary;
	RosterLoadout.Secondary = SavedLoadout.Secondary;
	RosterLoadout.LongTactical = SavedLoadout.LongTactical;
	RosterLoadout.GrenadeSlots = SavedLoadout.GrenadeSlots;
	RosterLoadout.TacticalSlots = SavedLoadout.TacticalSlots;
	RosterLoadout.GrenadeSlots.SetNum(SavedLoadout.GrenadeSlotsCount);
	RosterLoadout.TacticalSlots.SetNum(SavedLoadout.TacticalSlotsCount);
	
	return RosterLoadout;
}

int32 URosterManager::GetCurrentRosterSize() const
{
	if (!CommanderProfile)
		return 0;
	
	int32 CompletedMissions = CommanderProfile->CompletedLevels.Num();
	
	int32 CurrentSize = Settings->RosterSize;
	for (int32 RequiredMissions : Settings->AdditionalRosterSlots)
	{
		if (RequiredMissions <= CompletedMissions)
			CurrentSize++;
	}

	return CurrentSize;
}

int32 URosterManager::GetMaximumRosterSize() const
{
	return Settings->RosterSize + Settings->AdditionalRosterSlots.Num();
}

TArray<int32> URosterManager::GetUnlockableSlotsMissionsRemaining() const
{
	if (!CommanderProfile)
		return {};
	
	int32 CompletedMissions = CommanderProfile->CompletedLevels.Num();
	TArray<int32> UnlockableSlotsMissionsRemaining;
	
	for (int32 RequiredMissions : Settings->AdditionalRosterSlots)
	{
		int32 MissionsRemaining = RequiredMissions - CompletedMissions;
		if (MissionsRemaining > 0)
			UnlockableSlotsMissionsRemaining.Add(MissionsRemaining);
	}
	
	return UnlockableSlotsMissionsRemaining;
}

bool URosterManager::IsRosterSlotUnlocked() const
{
	if (!CommanderProfile)
		return false;

	const int32 CompletedMissions = CommanderProfile->CompletedLevels.Num();

	for (const int32 RequiredMissions : Settings->AdditionalRosterSlots)
	{
		const int32 MissionsRemaining = RequiredMissions - CompletedMissions;
		if (MissionsRemaining == 0)
			return true;
	}

	return false;
}

URosterCharacter* URosterManager::GetSquadCharacter(ERosterSquadPosition SquadPosition, bool bGenerateStandIn)
{
	// Return null as unassigned is not in the squad
	if (SquadPosition == ERosterSquadPosition::Unassigned)
		return nullptr;
	
	// Try to find an existing character in this position
	for (URosterCharacter* Character : Characters)
	{
		if (Character->Position == SquadPosition)
		{
			return Character;
		}
	}

	// Generate a stand-in if we've got no one and it is desired
	return bGenerateStandIn ? GenerateCharacter() : nullptr;
}

void URosterManager::SetCharacterSquadPosition(URosterCharacter* TargetCharacter, ERosterSquadPosition Position)
{
	if (!TargetCharacter)
		return;
	
	if (!ensure(Characters.Contains(TargetCharacter)))
		return;

	if (TargetCharacter->State != ERosterCharacterState::Available)
		return;
	
	// Swap roles if a character already exists in that role
	URosterCharacter** PreviousCharacter = SquadCharacters.Find(Position);
	if (PreviousCharacter)
	{
		(*PreviousCharacter)->Position = TargetCharacter->Position;
		SquadCharacters.Add(TargetCharacter->Position, *PreviousCharacter);
	}

	TargetCharacter->Position = Position;
	SquadCharacters.Add(Position, TargetCharacter);

	ValidateRoster();
}

void URosterManager::RecruitCharacter(URosterCharacter* Character)
{
	if (!Character)
		return;
	
	int32 RosterSize = GetCurrentRosterSize();
	if (Characters.Num() >= RosterSize)
		return;

	if (Characters.Contains(Character))
		return;

	Characters.Add(Character);
	GenerateRecruitableCharacters();

	ValidateSquad();
}

void URosterManager::FireCharacter(URosterCharacter* Character)
{
	if (!Character)
		return;

	if (!Characters.Contains(Character))
		return;
	
	for (URosterCharacter* OtherCharacter : Characters)
	{
		float FinalStress = OtherCharacter->StressLevel + Settings->StressGainForOfficerFired;
		float StressCap = FMath::Max(Settings->MaxStressForOfficerFired, OtherCharacter->StressLevel);

		OtherCharacter->StressLevel = FMath::Min(FinalStress, StressCap);
	}

	Character->State = ERosterCharacterState::Deceased;
	Character->Position = ERosterSquadPosition::Unassigned;
	Character->RemovalReason = ERosterRemovalReason::Fired;
	Characters.Remove(Character);

	FiredCount++;
	ValidateRoster();
	ValidateSquad();
}

bool URosterManager::CanFireCharacter()
{
	if (CVarRonUnlimitedFiring.GetValueOnAnyThread() != 0)
		return true;

	if (Settings->MaxOfficersFiredAtOnce < 0)
		return true;
	
	return FiredCount < Settings->MaxOfficersFiredAtOnce;
}

void URosterManager::SetCharacterInTherapy(URosterCharacter* TargetCharacter)
{
	if (!TargetCharacter)
		return;

	// Clamp stress on arrival just in case they were automatically grabbed by the therapist
	TargetCharacter->StressLevel = FMath::Clamp(TargetCharacter->StressLevel, 0.0f, 0.99f);

	// Adjust the time scale so it starts at the minimum stress value
	float StressLevel = TargetCharacter->StressLevel;
	const float MinimumStressForTherapy = Settings->MinimumStressForTherapy;
	float AdjustedStressLevel = (TargetCharacter->StressLevel - MinimumStressForTherapy) / MinimumStressForTherapy;
	
	int32 MissionsUntilReturn = FMath::RoundFromZero(AdjustedStressLevel * Settings->TherapistTimeScale);
	MissionsUntilReturn = FMath::Max(MissionsUntilReturn, 1); // Minimum one mission away
	
	TargetCharacter->State = ERosterCharacterState::InTherapy;
	TargetCharacter->Position = ERosterSquadPosition::Unassigned;
	TargetCharacter->MissionsUntilReturn = MissionsUntilReturn;

	if (TargetCharacter->Storyline)
	{
		URosterStoryline* Storyline = TargetCharacter->Storyline;
		TargetCharacter->MostRecentEventText = GetRandomTextEvent(Storyline->TherapistAssessmentEvents);
	}
	
	ValidateRoster();
	ValidateSquad();
}

bool URosterManager::CanUseTherapist(URosterCharacter* Character)
{
	if (Character->State != ERosterCharacterState::Available)
		return false;

	if (Character->StressLevel < Settings->MinimumStressForTherapy)
		return false;

	return !IsTherapyFull();
}

bool URosterManager::IsTherapyFull() const
{
	if (GetMaxCharactersInTherapy() < 0)
		return true;
	
	return GetNumCharactersInTherapy() >= GetMaxCharactersInTherapy();
}

int32 URosterManager::GetNumCharactersInTherapy() const
{
	int32 NumCharactersInTherapy = 0;
	for (URosterCharacter* Character : Characters)
	{
		if (Character->State == ERosterCharacterState::InTherapy)
			NumCharactersInTherapy++;
	}
	return NumCharactersInTherapy;
}

int32 URosterManager::GetMaxCharactersInTherapy() const
{
	return Settings->MaxCharactersInTherapy;
}

float URosterManager::GetSquadTraitValue(FName Trait) const
{
	int32 Count;
	return GetSquadTraitValue(Trait, Count);
}

float URosterManager::GetSquadTraitValue(FName Trait, int32& OutCount) const
{
	if (!ensureMsgf(PossibleTraits.Contains(Trait), TEXT("No trait defined for %s"), *Trait.ToString()))
		return 0.0f;
	
	const URosterTrait* TraitAsset = PossibleTraits[Trait];
	if (!ensure(TraitAsset))
		return 0.0f;

	bool bCheckUnlocked = CVarRonUnlockAllTraits.GetValueOnAnyThread() == 0;
	
	int32 SquadCount = 0;
	for (const TPair<ERosterSquadPosition, URosterCharacter*>& Pair : SquadCharacters)
	{
		// If they're dead or incapacitated, they do not contribute to the trait
		if (Pair.Value->State != ERosterCharacterState::Available)
			continue;
		
		if (bCheckUnlocked && Pair.Value->bTraitUnlocked)
			continue;
		
		if (Pair.Value->Trait == TraitAsset)
			SquadCount++;
	}

	const FRichCurve* Curve = TraitAsset->TraitCurve.GetRichCurveConst();
	return Curve->HasAnyData() ? Curve->Eval(SquadCount, 0.0f) : 0.0f;
}

float URosterManager::GetSquadTraitValue(FName Trait, UWorld* World)
{
	if (!ensure(World))
		return 0.0f;

	ACommanderGM* CommanderGM = World->GetAuthGameMode<ACommanderGM>();
	if (!CommanderGM)
		return 0.0f;

	const URosterManager* RosterManager = CommanderGM->RosterManager;
	if (!RosterManager)
		return 0.0f;

	return RosterManager->GetSquadTraitValue(Trait);
}

void URosterManager::OnPlayerKilled(AReadyOrNotCharacter* Instigator)
{
	TickTimers();
	
	float StressGain = Settings->StressGainForPlayerKilled * GetSquadTraitValue("SBAGS");
	for (const TPair<ERosterSquadPosition, URosterCharacter*>& Pair : SquadCharacters)
	{
		Pair.Value->StressLevel += StressGain;
	}
}

void URosterManager::OnCharacterKilled(URosterCharacter* Character, bool bForceIncapacitation)
{
	CharactersKilled++;
	
	float DeathChance = FMath::Clamp(Settings->DeathChance, 0.0f, 1.0f);
	bool bKilled = UKismetMathLibrary::RandomBoolWithWeightFromStream(DeathChance, RandomStream);

	// Prevent officer deaths up to the amount of paramedics on the team
	int32 ParamedicCount = FMath::RoundToInt(GetSquadTraitValue("Paramedic"));
	if (ParamedicCount >= CharactersKilled)
		bKilled = false;
	
	if (bKilled && !bForceIncapacitation)
	{
		Character->State = ERosterCharacterState::Deceased;
		Character->RemovalReason = ERosterRemovalReason::Deceased;

		URosterStoryline* Storyline = Character->Storyline;
		if (Storyline)
		{
			bool bUnfit = UKismetMathLibrary::RandomBoolFromStream(RandomStream);
			const TArray<FText>& Events = bUnfit ? Storyline->UnfitForDutyEvents : Storyline->DeathEvents;

			Character->MostRecentEventText = GetRandomTextEvent(Events);
		}
	}
	else
	{
		Character->State = ERosterCharacterState::Incapacitated;
		Character->MissionsUntilReturn = UKismetMathLibrary::RandomIntegerInRangeFromStream(1, Settings->MaxIncapacitationTime, RandomStream);

		// Cap stress level when character is incapacitated for balance
		Character->StressLevel = FMath::Clamp(Character->StressLevel, 0.0f, Settings->MaxStressWhenIncapacitated);
	}

	float StressGain = Settings->StressGainForOfficerKilled * GetSquadTraitValue("SBAGS");
	for (const TPair<ERosterSquadPosition, URosterCharacter*>& Pair : SquadCharacters)
	{
		Pair.Value->StressLevel += StressGain;
	}
}

void URosterManager::OnSuspectKilled(URosterCharacter* Instigator, bool bCivilian)
{
	for (const TPair<ERosterSquadPosition, URosterCharacter*>& Pair : SquadCharacters)
	{
		float BaseStress = UKismetMathLibrary::RandomFloatInRangeFromStream(Settings->MinBaseStressGainForKill, Settings->MaxBaseStressGainForKill, RandomStream);
		float InstigatorFactor = Pair.Value == Instigator ? Settings->StressMultiplierForKillInstigator : 1.0f;
		float CivilianFactor = bCivilian ? Settings->StressMultiplierForCivilianKill : 1.0f;
		float TraitFactor = GetSquadTraitValue("SBAGS");
		
		float FinalStressGain = BaseStress * InstigatorFactor * CivilianFactor * TraitFactor;
		Pair.Value->StressLevel += FinalStressGain;
	}
}

void URosterManager::OnSuspectArrested(URosterCharacter* Instigator, bool bCivilian)
{
	float StressLoss = bCivilian ? -Settings->StressLossForCivilianArrest : -Settings->StressLossForSuspectArrest;
	float TraitFactor = FMath::Max(GetSquadTraitValue("MoralOfficer"), 1.0f);
	
	float FinalStressLoss = StressLoss * TraitFactor;
	
	for (const TPair<ERosterSquadPosition, URosterCharacter*>& Pair : SquadCharacters)
	{
		Pair.Value->StressLevel += FinalStressLoss;
	}
}

void URosterManager::OnFriendlyOrSurrenderedKilled()
{
	PlayerFriendlyKills++;
	if (PlayerFriendlyKills <= 1)
		return;

	for (URosterCharacter* Character : Characters)
	{
		float Initial = Settings->StressGainForFriendlyKilledByPlayer;
		float Factor = Settings->StressGainForFriendlyKilledByPlayerBase;
		
		int32 Exponent = PlayerFriendlyKills - 1;
		
		Character->StressLevel += Initial * FMath::Pow(Factor, Exponent);
	}
}

void URosterManager::OnExfiltratedMission(UWorld* World, TArray<ASWATCharacter*> ExfiltratedOfficers, bool bActiveBombThreat, bool bIsActiveShooter)
{
	TArray<URosterCharacter*> ExfiltratedRosterCharacters;
	for (ASWATCharacter* SWATCharacter : ExfiltratedOfficers)
	{
		if (!IsValid(SWATCharacter))
			continue;
		
		URosterCharacter* RosterCharacter = SWATCharacter->GetRosterCharacter();
		if (!IsValid(RosterCharacter))
			continue;
		
		RosterCharacter->StressLevel += bIsActiveShooter ? Settings->StressGainForActiveShooterExfil : Settings->StressGainForExfil;
		ExfiltratedRosterCharacters.Emplace(RosterCharacter);
	}

	for (const TPair<ERosterSquadPosition, URosterCharacter*>& Pair : SquadCharacters)
	{
		if (!ExfiltratedRosterCharacters.Contains(Pair.Value))
		{
			if (bActiveBombThreat)
			{
				// Just call onkilled here rather than going through and killing the physical AI
				// TODO: Kill actual AI too so it shows they died on the grade screen
				OnCharacterKilled(Pair.Value);
			}
			else
			{
				Pair.Value->StressLevel += Settings->StressGainForOfficerNotExfil;
			}
		}
	}
}

void URosterManager::ProcessRoster()
{
	for (auto It = Characters.CreateIterator(); It; ++It)
	{
		URosterCharacter* Character = *It;
		bool bIsDead = Character->State == ERosterCharacterState::Deceased;
		
		// Ensure accumulated negative stress is reset
		Character->StressLevel = FMath::Max(Character->StressLevel, 0.0f);

		// Remove character when overstressed
		if (!bIsDead && Character->StressLevel >= 1.0f)
		{
			// Save the character if no one else is in therapy
			if (!IsTherapyFull())
			{
				SetCharacterInTherapy(Character);
				Character->bTherapistIntervened = true;
				Character->MissionsUntilReturn++; // Extra punishment turn for not using therapist
				
				continue;
			}
			
			Character->State = ERosterCharacterState::Deceased;
			Character->Position = ERosterSquadPosition::Unassigned;
			Character->RemovalReason = ERosterRemovalReason::Overstressed;

			URosterStoryline* Storyline = Character->Storyline;
			if (Storyline)
			{
				Character->MostRecentEventText = GetRandomTextEvent(Storyline->StressQuitEvents);
			}
			
			PreviousCharacters.Add(Character);
			It.RemoveCurrent();
		}
		// Collect dead characters
		else if (bIsDead)
		{
			PreviousCharacters.Add(Character);
			It.RemoveCurrent();
		}
	}
	
	ValidateRoster();
	ValidateSquad();
	GenerateRecruitableCharacters();
}

void URosterManager::ValidateRoster()
{
	for (auto It = Characters.CreateIterator(); It; ++It)
	{
		URosterCharacter* Character = *It;
		
		// Ensure unavailable characters are not in the squad
		if (Character->State != ERosterCharacterState::Available)
			Character->Position = ERosterSquadPosition::Unassigned;

		// Ensure dead characters are not in the roster
		if (!ensure(Character->State != ERosterCharacterState::Deceased))
		{
			PreviousCharacters.AddUnique(Character);
			It.RemoveCurrent();
		}
	}
}

void URosterManager::ValidateSquad()
{
	static const TArray<ERosterSquadPosition> SquadPositions =
	{
		ERosterSquadPosition::RedOne,
		ERosterSquadPosition::RedTwo,
		ERosterSquadPosition::BlueOne,
		ERosterSquadPosition::BlueTwo
	};

	// For each possible squad role...
	SquadCharacters.Empty();
	for (ERosterSquadPosition SquadPosition : SquadPositions)
	{
		// Ensure no characters share the same squad position
		URosterCharacter* FoundCharacter = nullptr;
		for (URosterCharacter* Character : Characters)
		{
			if (!FoundCharacter && Character->Position == SquadPosition)
			{
				FoundCharacter = Character;
			}
			else if (FoundCharacter && Character->Position == SquadPosition)
			{
				Character->Position = ERosterSquadPosition::Unassigned;
			}
		}

		// Ensure that every role has a character assigned
		if (!FoundCharacter)
		{
			for (URosterCharacter* Character : Characters)
			{
				if (Character->Position != ERosterSquadPosition::Unassigned)
					continue;

				if (Character->State != ERosterCharacterState::Available)
					continue;
				
				Character->Position = SquadPosition;
				FoundCharacter = Character;
				break;
			}
		}

		if (FoundCharacter)
			SquadCharacters.Add(SquadPosition, FoundCharacter);
	}
}

void URosterManager::TickTimers()
{
	if (bHasTickedTimers)
		return;

	bHasTickedTimers = true;
	
	// Update state timers for unassigned characters on mission success only
	for (URosterCharacter* Character : Characters)
	{
		// Ignore characters who are in the squad
		if (Character->Position != ERosterSquadPosition::Unassigned)
			continue;
		
		// Ignore characters who died during the mission
		if (Character->State == ERosterCharacterState::Deceased)
			continue;

		// Count down return from state timer
		Character->MissionsUntilReturn = FMath::Max(Character->MissionsUntilReturn - 1, 0);
		if (Character->MissionsUntilReturn <= 0)
		{
			URosterStoryline* Storyline = Character->Storyline;
			if (Storyline && Character->State == ERosterCharacterState::Incapacitated)
			{
				Character->MostRecentEventText = GetRandomTextEvent(Storyline->ReturnFromIncapacitationEvents);
			}
			else if (Storyline && Character->State == ERosterCharacterState::InTherapy)
			{
				Character->MostRecentEventText = GetRandomTextEvent(Storyline->ReturnFromTherapyEvents);
			}

			// Reset stress on return from therapy
			if (Character->State == ERosterCharacterState::InTherapy)
				Character->StressLevel = 0.0f;

			// Finally make the character available for use
			Character->State = ERosterCharacterState::Available;
		}

		// Passive stress loss for unused SWAT on completion
		if (Character->State == ERosterCharacterState::Available &&
			Character->Position == ERosterSquadPosition::Unassigned)
		{
			Character->StressLevel = FMath::Max(0.0f, Character->StressLevel - Settings->PassiveStressLoss);
		}
	}
}

void URosterManager::ProcessTraitUnlocks()
{
	// Once the squad has been built check for pending trait unlocks
	TArray<URosterCharacter*> EligibleCharacters;
	for (const TPair<ERosterSquadPosition, URosterCharacter*>& Pair : SquadCharacters)
	{
		// Character already unlocked trait, skip them
		if (Pair.Value->bTraitUnlocked)
			continue;

		// Dead people should not unlock traits
		if (Pair.Value->State == ERosterCharacterState::Deceased)
			continue;
		
		if (Pair.Value->MissionsPlayed >= Settings->MissionsUntilTraitUnlockable)
			EligibleCharacters.Add(Pair.Value);
	}

	// Grant traits
	if (EligibleCharacters.Num() > 0)
	{
		int32 RandomIndex = UKismetMathLibrary::RandomIntegerInRangeFromStream(0, EligibleCharacters.Num() - 1, RandomStream);
		
		URosterCharacter* RandomCharacter = EligibleCharacters[RandomIndex];
		RandomCharacter->bTraitUnlocked = true;
		RandomCharacter->bJustUnlockedTrait = true;
	}
}

void URosterManager::GenerateCharacters()
{
	Characters.Empty();
	GenerateCharacters(Characters, Settings->RosterSize - 1);
}

void URosterManager::GenerateRecruitableCharacters()
{
	RecruitableCharacters.Empty();
	GenerateCharacters(RecruitableCharacters, Settings->NumRecruitableCharacters);
}

int32 GenerateUniqueSerialNumber(const TSet<int32>& UsedSerialNumbers, const FRandomStream& RandomStream)
{
	while (true)
	{
		int32 SerialNumber = UKismetMathLibrary::RandomIntegerInRangeFromStream(0, 999999, RandomStream);

		if (!UsedSerialNumbers.Contains(SerialNumber))
			return SerialNumber;

		// Arbitrary upper limit for serial numbers before we stop caring
		if (UsedSerialNumbers.Num() > 1000)
			return SerialNumber;
	}
}

void URosterManager::GenerateCharacters(TArray<URosterCharacter*>& OutCharacters, int32 Count)
{
	if (PossibleCharacters.Num() <= 0)
		return;
	
	// Generate characters until we hit the desired count
	for (int i = 0; i < Count; i++)
	{
		OutCharacters.Add(GenerateCharacter());
	}
}

URosterCharacter* URosterManager::GenerateCharacter()
{
	// Get a random character from the available archetypes
	URosterCharacterArchetype* RandomCharacter = nullptr;

	// Try not to generate the same archetype twice
	int32 Attempts = 0;
	while (Attempts < 3)
	{
		RandomCharacter = GetRandomArrayItem<URosterCharacterArchetype*>(PossibleCharacters, RandomStream);
		Attempts++;

		if (LastArchetype != RandomCharacter)
			break;
	}
	LastArchetype = RandomCharacter;

	if (!ensure(RandomCharacter))
		return nullptr;
	
	URosterCharacter* NewCharacter = NewObject<URosterCharacter>();
	NewCharacter->State = ERosterCharacterState::Available;
	NewCharacter->Position = ERosterSquadPosition::Unassigned;
	NewCharacter->StressLevel = UKismetMathLibrary::RandomFloatInRangeFromStream(0.0f, Settings->MaximumStartingStress, RandomStream);
	
	NewCharacter->Archetype = RandomCharacter;
	NewCharacter->Character = GetRandomArrayItem<UCustomizationCharacter*>(RandomCharacter->CharacterPool, RandomStream);
	NewCharacter->Voice = GetRandomArrayItem<UCustomizationVoice*>(RandomCharacter->VoicePool, RandomStream);
	NewCharacter->FirstName = GetRandomArrayItem<FText>(RandomCharacter->FirstNamePool, RandomStream);
	NewCharacter->LastName = GetRandomArrayItem<FText>(RandomCharacter->LastNamePool, RandomStream);
	NewCharacter->YearsInSWAT = UKismetMathLibrary::RandomIntegerInRangeFromStream(1, 15, RandomStream);
	NewCharacter->Description = GetRandomArrayItem<FText>(RandomCharacter->BackgroundTextPool, RandomStream);

	// Choose a random trait to give to this character
	if (PossibleTraits.Num() > 0)
	{
		TArray<FName> TraitNames;
		PossibleTraits.GetKeys(TraitNames);
		
		NewCharacter->Trait = PossibleTraits[GetRandomArrayItem<FName>(TraitNames, RandomStream)];
	}

	// Generate a (mostly) unique serial number for this officer
	NewCharacter->SerialNumber = GenerateUniqueSerialNumber(UsedSerialNumbers, RandomStream);
	UsedSerialNumbers.Add(NewCharacter->SerialNumber);
	
	// Choose a random storyline to assign to this character
	if (EventData)
	{
		NewCharacter->Storyline = GetRandomArrayItem<URosterStoryline*>(EventData->Storylines, RandomStream);
	}
	
	NewCharacter->bIsNew = true;
	return NewCharacter;
}

TArray<URosterCharacterArchetype*> URosterManager::GetAllCharacterArchetypes()
{
	UAssetManager& AssetManager = UAssetManager::Get();

	TArray<FAssetData> DataAssets;
	AssetManager.GetPrimaryAssetDataList("RosterCharacterArchetype", DataAssets);

	TArray<URosterCharacterArchetype*> Archetypes;
	for (const FAssetData& AssetData : DataAssets)
	{
		// Get asset is slow but the roster manager is created at the end of load time anyway
		URosterCharacterArchetype* DataAsset = Cast<URosterCharacterArchetype>(AssetData.GetAsset());
		if (!ensure(DataAsset))
			continue;

		Archetypes.AddUnique(DataAsset);
	}
	return Archetypes;
}

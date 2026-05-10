// Copyright Void Interactive, 2024

#include "Info/AIFactionManager.h"

#include "Characters/CyberneticController.h"
#include "Characters/AI/CivilianCharacter.h"
#include "Characters/AI/SuspectCharacter.h"

TAutoConsoleVariable<int32> CVarFactionDebug(TEXT("Faction.Debug"), 0, TEXT("0 = Disable faction debug. 1 = Enable faction debug"));

AAIFactionManager::AAIFactionManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.033f;

	bReplicates = true;
	bAlwaysRelevant = true;
	
	SetCanBeDamaged(false);
	SetActorEnableCollision(false);
	bFindCameraComponentWhenViewTarget = false;
	bBlockInput = true;
	bRelevantForLevelBounds = false;
	AutoReceiveInput = EAutoReceiveInput::Disabled;
	bEnableAutoLODGeneration = false;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
}

void AAIFactionManager::BeginPlay()
{
	Super::BeginPlay();

	Characters.Empty(10);
	Leaders.Empty(2);

	#if !UE_BUILD_SHIPPING
	DebugColor = FColor::MakeRandomColor();
	#endif

	if (bGroupIntoTeams)
	{
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, FTimerDelegate::CreateUObject(this, &AAIFactionManager::TryFindTeam, GetAllSuspects()), 1.0f, true);
	}
}

void AAIFactionManager::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	TeamBehaviourOverrideStrength = FMath::Max(TeamBehaviourOverrideStrength - (DeltaTime * TeamBehaviourStrengthReductionSpeed), 0.0f);

	#if !UE_BUILD_SHIPPING
	if (CVarFactionDebug.GetValueOnAnyThread() > 0)
	{
		LOG_NUMBER(TeamBehaviourOverrideStrength);
		
		for (ACyberneticCharacter* Character : Characters)
		{
			if (IsValid(Character))
			{
				FVector DebugLocation = Character->GetMesh()->GetBoneLocation("spine_3") + Character->GetActorForwardVector() * 25.0f;
				
				DrawDebugSphere(GetWorld(), DebugLocation, 15.0f, 4, FColor::Yellow, false, PrimaryActorTick.TickInterval, 0, 1.5f);
				
				DrawDebugString(GetWorld(), DebugLocation + FVector(0.0f, 0.0f, 35.0f), Character->FactionData.Name, nullptr, DebugColor, PrimaryActorTick.TickInterval, true, 1.25f);

			}
		}
	}
	#endif
}

void AAIFactionManager::OnAIAdded(ACyberneticCharacter* Character)
{
	OnAIAdded_Blueprint(Character);
}

void AAIFactionManager::OnAISpottedEnemy(ACyberneticCharacter* Spotter, AReadyOrNotCharacter* Character)
{
	OnAISpottedEnemy_Blueprint(Spotter, Character);

	if (ASuspectCharacter* Suspect = Cast<ASuspectCharacter>(Spotter))
	{
		AlertOtherSuspectsInTeam(Suspect, Character);
	}
}

void AAIFactionManager::OnAISpottedFriendly(ACyberneticCharacter* Spotter, AReadyOrNotCharacter* Character)
{
	OnAISpottedFriendly_Blueprint(Spotter, Character);
}

void AAIFactionManager::OnAISpottedNeutral(ACyberneticCharacter* Spotter, AReadyOrNotCharacter* Character)
{
	OnAISpottedNeutral_Blueprint(Spotter, Character);
}

void AAIFactionManager::AddCharacter(ACyberneticCharacter* Character)
{
	if (IsValid(Character) && !Characters.Contains(Character))
	{
		Character->OnSpottedEnemy.RemoveAll(this);
		Character->OnSpottedEnemy.AddDynamic(this, &AAIFactionManager::OnAISpottedEnemy);
		Character->OnSpottedFriendly.RemoveAll(this);
		Character->OnSpottedFriendly.AddDynamic(this, &AAIFactionManager::OnAISpottedFriendly);
		Character->OnSpottedNeutral.RemoveAll(this);
		Character->OnSpottedNeutral.AddDynamic(this, &AAIFactionManager::OnAISpottedNeutral);
		
		Characters.AddUnique(Character);

		if (Character->AssignedAIData->bIsLeaderOfFaction)
		{
			Leaders.AddUnique(Character);
		}

		OnAIAdded(Character);
	}
}

void AAIFactionManager::OnAllAISpawned()
{
	OnAllAISpawned_Blueprint();

	if (bGroupIntoTeams)
	{
		InitializeSuspectTeams();

		#if !UE_BUILD_SHIPPING
		int32 i = 0;
		for (FFactionSuspectTeam& It : SuspectTeams)
		{
			if (It.Suspects.Num() > 0)
			{
				ULog::Info("Team " + FString::FromInt(i) + ": ");

				for (ASuspectCharacter* Suspect : It.Suspects)
				{
					ULog::ObjectName(Suspect);
				}

				ULog::LineBreak();
			}

			i++;
		}
		#endif
	}
}

void AAIFactionManager::InitializeSuspectTeams()
{
	SuspectTeams.Empty();

	if (TeamsOf > 1)
	{
		const TArray<ASuspectCharacter*> SuspectPool = GetAllSuspects();

		const int32 NumSuspects = SuspectPool.Num();
		
		if (NumSuspects < 2)
			return;

		const int32 NumTeams = (NumSuspects / TeamsOf) + (NumSuspects % TeamsOf);

		for (int32 i = 0; i < NumTeams; ++i)
		{
			FFactionSuspectTeam EmptyTeam;
			SuspectTeams.Add(EmptyTeam);
		}

		TryFindTeam(SuspectPool);
	}
}

void AAIFactionManager::TryFindTeam(TArray<ASuspectCharacter*> InSuspects)
{
	TArray<ASuspectCharacter*> SuspectPool = InSuspects;
	
	TArray<ASuspectCharacter*> ClosestSuspects;
	ClosestSuspects.Reserve(InSuspects.Num());

	for (ASuspectCharacter* Suspect : InSuspects)
	{
		if (!IsValid(Suspect))
			continue;
		
		if (SuspectPool.Num() > 0)
		{
			FFactionSuspectTeam EmptyTeam = {};
			int32 TeamIndex = -1;
			const bool bTeamSpotsAvailable = AreTeamSpotsAvailable(TeamIndex);
			const bool bIsInTeam = IsSuspectInTeam(Suspect, EmptyTeam);
			
			if (!bIsInTeam && bTeamSpotsAvailable)
			{
				ClosestSuspects.Empty();

				for (int32 i = 0; i <= TeamsOf - 2; ++i)
				{
					if (ASuspectCharacter* ClosestSuspect = FindClosestSuspect(SuspectPool, Suspect, TeamsOf * 500))
					{
						ClosestSuspects.AddUnique(ClosestSuspect);
						SuspectPool.Remove(ClosestSuspect);
						SuspectPool.Remove(Suspect);
					}
				}

				if (ClosestSuspects.Num() > 0)
				{
					FFactionSuspectTeam* Team = nullptr;
					
					int32 i = 0;
					for (FFactionSuspectTeam& It : SuspectTeams)
					{
						if (i == TeamIndex)
						{
							Team = &It;
							break;
						}

						i++;
					}

					if (Team)
					{
						Team->Suspects.AddUnique(Suspect);
						Team->Tactics.Add(EAITeamTactic::None);

						for (ASuspectCharacter* ClosestSuspect : ClosestSuspects)
						{
							Team->Suspects.AddUnique(ClosestSuspect);
							Team->Tactics.Add(EAITeamTactic::None);
						}

						int32 j = 0;
						for (EAITeamTactic& Tactic : Team->Tactics)
						{
							if (bAssignRandomTeamTactics)
							{
								Tactic = (EAITeamTactic)FMath::RandRange(1, ((int32)EAITeamTactic::Custom)-1);
							}
							else
							{
								if (TacticsPool.IsValidIndex(TeamIndex))
								{
									if (TacticsPool[TeamIndex].Tactics.IsValidIndex(j))
										Tactic = TacticsPool[TeamIndex].Tactics[j];
								}
							}
							
							j++;
						}
					}
				}
			}
		}
	}
}

bool AAIFactionManager::IsSuspectInTeam(ASuspectCharacter* InSuspect, FFactionSuspectTeam& OutTeam) const
{
	if (!InSuspect)
		return false;
	
	for (const FFactionSuspectTeam& It : SuspectTeams)
	{
		if (It.Suspects.Contains(InSuspect))
		{
			OutTeam = It;
			return true;
		}
	}

	OutTeam = FFactionSuspectTeam();
	return false;
}

bool AAIFactionManager::AreTeamSpotsAvailable(int32& OutIndex) const
{
	int32 i = 0;
	for (const FFactionSuspectTeam& It : SuspectTeams)
	{
		if (!IsTeamFull(It))
		{
			OutIndex = i;
			return true;
		}

		i++;
	}

	OutIndex = -1;
	return false;
}

bool AAIFactionManager::IsTeamFull(const FFactionSuspectTeam& InTeam) const
{
	return InTeam.Suspects.Num() >= TeamsOf;
}

ASuspectCharacter* AAIFactionManager::FindClosestSuspect(const TArray<ASuspectCharacter*>& OtherSuspects, ASuspectCharacter* Suspect, float MaxDistance) const
{
	float ClosestDistance = MaxDistance;
	ASuspectCharacter* ClosestSuspect = nullptr;

	for (ASuspectCharacter* OtherSuspect : OtherSuspects)
	{
		if (!IsValid(OtherSuspect))
			continue;
		
		if (Suspect != OtherSuspect)
		{
			FFactionSuspectTeam Team;
			if (!IsSuspectInTeam(OtherSuspect, Team))
			{
				const float Distance = FVector::Distance(OtherSuspect->GetActorLocation(), Suspect->GetActorLocation());

				if (Distance < ClosestDistance)
				{
					ClosestDistance = Distance;
					ClosestSuspect = OtherSuspect;
				}
			}
		}
	}
	
	return ClosestSuspect;
}

bool AAIFactionManager::GetSuspectsInTeam(ASuspectCharacter* InSuspect, TArray<ASuspectCharacter*>& OutSuspects, bool bIncludeSelf)
{
	FFactionSuspectTeam Team;
	if (IsSuspectInTeam(InSuspect, Team))
	{
		if (bIncludeSelf)
		{
			OutSuspects = Team.Suspects;
			return true;
		}

		Team.Suspects.Remove(InSuspect);
		OutSuspects = Team.Suspects;
		return true;
	}

	OutSuspects.Empty();
	return false;
}

bool AAIFactionManager::GetTeamTacticFor(ASuspectCharacter* InSuspect, EAITeamTactic& OutTactic) const
{
	for (const FFactionSuspectTeam& Team : SuspectTeams)
	{
		const int32 Index = Team.Suspects.Find(InSuspect);
		
		if (Team.Tactics.IsValidIndex(Index))
		{
			OutTactic = Team.Tactics[Index];
			return true;
		}
	}
	
	OutTactic = EAITeamTactic::None;
	return false;
}

int32 AAIFactionManager::GetTeamIndex(ASuspectCharacter* InSuspect) const
{
	int32 TeamIndex = -1;
	int32 i = 0;
	for (auto& It : SuspectTeams)
	{
		if (It.Suspects.Find(InSuspect) != INDEX_NONE)
		{
			TeamIndex = i;
			break;
		}
		
		i++;
	}
	
	return TeamIndex;
}

void AAIFactionManager::AlertOtherSuspectsInTeam(ASuspectCharacter* Suspect, AReadyOrNotCharacter* Enemy)
{
	TArray<ASuspectCharacter*> OtherSuspects;
	if (GetSuspectsInTeam(Suspect, OtherSuspects))
	{
		for (const ASuspectCharacter* OtherSuspect : OtherSuspects)
		{
			if (!IsValid(OtherSuspect))
				continue;
			
			if (OtherSuspect->GetCyberneticsController())
			{
				if (UTargetingComponent* TargetingComponent = OtherSuspect->GetCyberneticsController()->GetTargetingComp())
				{
					TargetingComponent->AddKnownEnemy(Enemy, true);
					TargetingComponent->AddCharacterToSeenMap(Enemy);
				}
			}
		}

		AlertOtherSuspectsInTeam_Blueprint(Suspect, Enemy);
	}
}

TArray<ASuspectCharacter*> AAIFactionManager::GetAllSuspects() const
{
	TArray<ASuspectCharacter*> AllSuspects;

	for (ACyberneticCharacter* Character : Characters)
	{
		if (ASuspectCharacter* Suspect = Cast<ASuspectCharacter>(Character))
		{
			AllSuspects.AddUnique(Suspect);
		}
	}

	return AllSuspects;
}

TArray<ACivilianCharacter*> AAIFactionManager::GetAllCivilians() const
{
	TArray<ACivilianCharacter*> AllCivilians;

	for (ACyberneticCharacter* Character : Characters)
	{
		if (ACivilianCharacter* Civilian = Cast<ACivilianCharacter>(Character))
		{
			AllCivilians.AddUnique(Civilian);
		}
	}

	return AllCivilians;
}

FString AAIFactionManager::GetFactionDebugInfo(ACyberneticCharacter* Character) const
{
	if (bGroupIntoTeams)
	{
		if (ASuspectCharacter* Suspect = Cast<ASuspectCharacter>(Character))
		{
			FString DebugString = FString::Printf(TEXT("Team: %i"), GetTeamIndex(Suspect));
			return DebugString;
		}
	}
	
	return "";
}

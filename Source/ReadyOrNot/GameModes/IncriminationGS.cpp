// Void Interactive, 2020

#include "IncriminationGS.h"
#include "Actors/Gameplay/IncriminationClue.h"

void AIncriminationGS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AIncriminationGS, ChosenEvidenceSpawn);
	DOREPLIFETIME(AIncriminationGS, ChosenEvidenceActor);
	DOREPLIFETIME(AIncriminationGS, ActiveClue);
	DOREPLIFETIME(AIncriminationGS, PreviousActiveClue);
	DOREPLIFETIME(AIncriminationGS, Clues);
	DOREPLIFETIME(AIncriminationGS, ClueSpawnPoints);
	DOREPLIFETIME(AIncriminationGS, ChosenExtractionDevice);
	DOREPLIFETIME(AIncriminationGS, ChosenEvidenceSearchArea);
	DOREPLIFETIME(AIncriminationGS, ChosenEvidenceBuilding);
	DOREPLIFETIME(AIncriminationGS, CurrentExtractionDevice);
	DOREPLIFETIME(AIncriminationGS, NonMainIntelSearchZones);
	DOREPLIFETIME(AIncriminationGS, PickupTeam);
	DOREPLIFETIME(AIncriminationGS, IntelState);
	DOREPLIFETIME(AIncriminationGS, bIntelExtracted);
}

AIncriminationClue* AIncriminationGS::GetClue(const int32 ClueNumber, bool& bSuccess, const bool bMustBeFound)
{
	for (AIncriminationClue* Clue : Clues)
	{
		if (IsValid(Clue) && Clue->GetClueNumber() == ClueNumber)
		{
			if (bMustBeFound)
			{
				if (Clue->IsClueFound())
				{
					bSuccess = true;
					return Clue;
				}
			}
			else
			{
				bSuccess = true;
				return Clue;
			}
		}
	}

	bSuccess = false;
	return nullptr;
}

TArray<AIncriminationClue*> AIncriminationGS::GetAllCluesOfNumber(const int32 ClueNumber)
{
	TArray<AIncriminationClue*> CluesOfNumber;
	
	for (AIncriminationClue* Clue : Clues)
	{
		if (IsValid(Clue) && Clue->GetClueNumber() == ClueNumber)
		{
			CluesOfNumber.Add(Clue);
		}
	}

	return CluesOfNumber;
}

bool AIncriminationGS::DoesPlayerPossessAnyClue(APlayerCharacter* PlayerCharacter)
{
	for (AIncriminationClue* Clue : Clues)
	{
		if (IsValid(Clue) && Clue->GetPickupInstigator() == PlayerCharacter)
		{
			return true;
		}
	}

	return false;
}

bool AIncriminationGS::AnyHigherCluesFound(const int32 ClueNumber)
{
	if (Clues.Num() > 0)
	{
		for (AIncriminationClue* Clue : Clues)
		{
			if (IsValid(Clue) && Clue->GetClueNumber() > ClueNumber)
			{
				if (APlayerCharacter* CluePickupInstigator = Cast<APlayerCharacter>(Clue->GetPickupInstigator()))
				{
					if (APlayerCharacter* LocalPlayer = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
					{
						if (Clue->IsClueFound() && CluePickupInstigator->GetTeam() == LocalPlayer->GetTeam())
							return true;
					}
				}
			}
		}
	}

	return false;
}

bool AIncriminationGS::AnyLowerCluesFound(const int32 ClueNumber)
{
	if (Clues.Num() > 0)
	{
		for (AIncriminationClue* Clue : Clues)
		{
			if (IsValid(Clue) && Clue->GetClueNumber() < ClueNumber)
			{
				if (APlayerCharacter* CluePickupInstigator = Cast<APlayerCharacter>(Clue->GetPickupInstigator()))
				{
					if (APlayerCharacter* LocalPlayer = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
					{
						if (Clue->IsClueFound() && CluePickupInstigator->GetTeam() == LocalPlayer->GetTeam())
							return true;
					}
				}
			}
		}
	}

	return false;
}

void AIncriminationGS::OnRep_OnIntelStateChanged()
{
	if (IntelState != EEvidenceActorState::Unclaimed)
	{
		for (AIncriminationClue* Clue : Clues)
		{
			if (Clue)
			{
				Clue->HideClue();
			}
		}
	}
	
	OnIntelStateChanged.Broadcast(ChosenEvidenceActor, IntelState, bIntelExtracted);
}

void AIncriminationGS::OnRep_OnIntelSearchAreaChosen()
{
	OnIntelSearchAreaChosen.Broadcast(ChosenEvidenceSearchArea);
}

void AIncriminationGS::OnRep_OnIntelBuildingChosen()
{
	OnIntelBuildingChosen.Broadcast(ChosenEvidenceBuilding);
}

void AIncriminationGS::OnRep_OnActiveClueChanged()
{
	if (ActiveClue)
	{
		ActiveClue->ShowClue();
	}
	
	OnActiveClueChanged.Broadcast(ActiveClue);
}

void AIncriminationGS::OnRep_OnPreviousActiveClueChanged()
{
	if (PreviousActiveClue)
	{
		PreviousActiveClue->HideClue();
	}
	
	OnPreviousActiveClueChanged.Broadcast(PreviousActiveClue);
}

void AIncriminationGS::OnRep_OnCluesChanged()
{
	//for (AIncriminationClue* Clue : Clues)
	//{
	//	if (Clue)
	//	{
	//		Clue->HideClue();
	//	}
	//}

	OnCluesChanged.Broadcast(Clues);
}

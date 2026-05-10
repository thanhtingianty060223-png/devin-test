// Copyright Void Interactive, 2023

#include "Info/Activities/Team/SearchAndSecureActivity.h"

#include "ArrestTargetActivity.h"
#include "CollectEvidenceActivity.h"
#include "ReportTargetActivity.h"
#include "Actors/CoverLandmark.h"
#include "Actors/Gameplay/EvidenceActor.h"
#include "Characters/AI/SWATController.h"
#include "Info/SWATManager.h"
#include "Characters/CyberneticController.h"
#include "Info/Activities/ActivityManagerTemplates.h"
#include "Info/Activities/SearchLandmarkActivity.h"

USearchAndSecureActivity::USearchAndSecureActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "SearchAndSecure");
	bAbortIfTrackingEnemy = false;
	bPauseIfTrackingEnemy = true;
	bNoMoveActivity = true;
	bFinishActivityWhenOverriden = false;
	bIsProgressActivity = true;
	bPauseIfRecentlyInCombat = true;
}

void USearchAndSecureActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	OriginalLocation = FVector::ZeroVector;
	
	if (bReturnToOriginalLocation)
	{
		OriginalLocation = GetCharacter()->GetNavAgentLocation();
	}
	
	TArray<AActor*> Securables;
	UGameplayStatics::GetAllActorsWithInterface(this, USecurable::StaticClass(), Securables);

	TArray<AActor*> SortedSecurables;
	SortedSecurables.Reserve(Securables.Num());
	
	for (AActor* Actor : Securables)
	{
		if (ACoverLandmark* Landmark = Cast<ACoverLandmark>(Actor))
		{
			if (Landmark->Type != ECoverLandmarkType::Corner)
				SortedSecurables.AddUnique(Actor);
		}
	}
	
	for (AActor* Actor : Securables)
	{
		if (Cast<ACyberneticCharacter>(Actor))
		{
			SortedSecurables.AddUnique(Actor);
		}
	}
	
	for (AActor* Actor : Securables)
	{
		if (ABaseItem* Item = Cast<ABaseItem>(Actor))
		{
			if (Item->IsEvidence())
			{
				SortedSecurables.AddUnique(Actor);
			}
		}
		else if (Cast<AEvidenceActor>(Actor))
		{
			SortedSecurables.AddUnique(Actor);
		}
	}

	AllSecurables = SortedSecurables;
}

void USearchAndSecureActivity::PerformActivity(float DeltaTime)
{
	Super::PerformActivity(DeltaTime);
	
	if (!IsValid(ClosestSecurable) || ISecurable::Execute_IsSecured(ClosestSecurable) || !ISecurable::Execute_CanBeSecured(ClosestSecurable))
	{
		if (ClosestSecurable)
		{
			SecureCooldown = 1.0f;
			TimeSinceLastSecure = 0.0f;
		}

		ClosestSecurable = nullptr;
		
		TimeSinceLastSecure += DeltaTime;
		if (TimeSinceLastSecure < SecureCooldown)
		{
			return;
		}
		
		float ClosestDistance = FLT_MAX;
		for (AActor* Actor : AllSecurables)
		{
			if (!IsValid(Actor))
				continue;
			
			if (ISecurable::Execute_CanBeSecured(Actor) && !ISecurable::Execute_IsSecured(Actor))
			{
				if (SearchingRoom)
				{
					const FRoom* SecurableRoomLocation = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(Actor->GetActorLocation());
					if (!SecurableRoomLocation)
						continue;
					
					// skip if not in same room
					if (SecurableRoomLocation->Name != SearchingRoom->Name)
						continue;
				}
				
				bool bAnyAISecuringThis = false;
				for (const ASWATCharacter* Swat : USWATManager::Get(this)->SwatAI)
				{
					if (Swat != GetCharacter() && Swat->GetCyberneticsController())
					{
						if (const USearchAndSecureActivity* OtherActivity = Swat->GetCyberneticsController<ASWATController>()->GetSearchAndSecureActivity())
						{
							if (OtherActivity->ClosestSecurable == Actor)
							{
								bAnyAISecuringThis = true;
								break;
							}
						}
					}
				}
				
				if (bAnyAISecuringThis)
					continue;

				const float Distance = FVector::Distance(GetCharacter()->GetActorLocation(), ISecurable::Execute_GetLocation(Actor));
				if (bAuto)
				{
					if (Distance > 1000.0f)
						continue;
				}
				
				if (Distance < ClosestDistance)
				{
					ClosestDistance = Distance;
					ClosestSecurable = Actor;
				}
			}
		}

		// all securables found...
		if (!IsValid(ClosestSecurable))
		{
			if (bReturnToOriginalLocation)
			{
				OwningController->GiveMoveTo(OriginalLocation);
			}
			
			OnSearchComplete.Broadcast(this, BreachDoor);
			OwningController->FinishActivity(this, true, true);
			
			return;
		}
	}
	
	TrySecure();
}

bool USearchAndSecureActivity::CanFinishActivity() const
{
	return false;
}

TSubclassOf<UNavigationQueryFilter> USearchAndSecureActivity::GetNavigationQueryOverride()
{
	if (!SearchingRoom)
	{
		return UNavigationQueryFilter::StaticClass();
	}

	return nullptr;
}

void USearchAndSecureActivity::TrySecure()
{
	if (Cast<ABaseItem>(ClosestSecurable) || Cast<AEvidenceActor>(ClosestSecurable))
	{
		const bool bAnyAISecuringThis = UActivityManager::AnyAIHasActivity<UCollectEvidenceActivity>([&](const UCollectEvidenceActivity* Activity)
		{
			if (Activity->EvidenceItem == ClosestSecurable)
			{
				return true;
			}

			return false;
		});
		
		if (bAnyAISecuringThis)
		{
			ClosestSecurable = nullptr;
			return;
		}
		
		if (const ABaseItem* ItemAsEvidence = Cast<ABaseItem>(ClosestSecurable))
		{
			ProgressState = FText::Format(FText::FromStringTable("SwatCommandTable", "SecuringEvidenceName"), ItemAsEvidence->ItemName);
		}
		else if (const AEvidenceActor* EvidenceActor = Cast<AEvidenceActor>(ClosestSecurable))
		{
			ProgressState = FText::Format(FText::FromStringTable("SwatCommandTable", "SecuringEvidenceName"), EvidenceActor->GetEvidenceName());
		}

		if (!OwningController->GetCurrentActivity<UCollectEvidenceActivity>())
		{
			if (UCollectEvidenceActivity* CollectEvidenceActivity = GetOwningController<ASWATController>()->GetCollectEvidenceActivity())
			{
				OwningController->RemoveActivitiesOfType(UCollectEvidenceActivity::StaticClass());
				CollectEvidenceActivity->EvidenceItem = ClosestSecurable;

				UActivityManager::GiveActivityTo(CollectEvidenceActivity, GetCharacter(), true, false);
			}
		}
	}
	else if (ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(ClosestSecurable))
	{
		const bool bAnyAISecuringThis = UActivityManager::AnyAIHasActivity<UArrestTargetActivity>([&](const UArrestTargetActivity* Activity)
		{
			if (Activity->ArrestTarget == AI)
			{
				return true;
			}

			return false;
		});
		
		if (bAnyAISecuringThis)
		{
			ClosestSecurable = nullptr;
			return;
		}
		
		if (AI->IsSuspect())
		{
			ProgressState = FText::FromStringTable("SwatCommandTable", "SecuringSuspect");
		}
		else
		{
			ProgressState = FText::FromStringTable("SwatCommandTable", "SecuringCivilian");
		}
		
		if (!OwningController->GetActivity<UArrestTargetActivity>())
		{
			if ((AI->IsSurrendered() && !AI->IsArrested()) || AI->IsDeadOrUnconscious() || AI->IsIncapacitated())
			{
				if (UArrestTargetActivity* ArrestTargetActivity = GetOwningController<ASWATController>()->GetArrestTargetActivity())
				{
					ArrestTargetActivity->ArrestTarget = AI;
					
					UActivityManager::GiveActivityTo(ArrestTargetActivity, GetCharacter(), true, false);
				}
			}
			else if (AI->IsArrested() && !AI->bHasBeenReported)
			{
				if (UReportTargetActivity* ReportTargetActivity = GetOwningController<ASWATController>()->GetReportTargetActivity())
				{
					ReportTargetActivity->ReportTarget = ClosestSecurable;	
					
					UActivityManager::GiveActivityTo(ReportTargetActivity, GetCharacter(), true, false);
				}
			}
		}
	}
	else if (ACoverLandmark* Landmark = Cast<ACoverLandmark>(ClosestSecurable))
	{
		const bool bAnyAISecuringThis = UActivityManager::AnyAIHasActivity<USearchLandmarkActivity>([&](const USearchLandmarkActivity* Activity)
		{
			if (Activity->CoverLandmark == Landmark)
			{
				return true;
			}

			return false;
		});
		
		if (bAnyAISecuringThis)
		{
			ClosestSecurable = nullptr;
			return;
		}
	
		if (!OwningController->GetCurrentActivity<USearchLandmarkActivity>())
		{
			if (USearchLandmarkActivity* SearchLandmarkActivity = GetOwningController<ASWATController>()->GetSearchLandmarkActivity())
			{
				SearchLandmarkActivity->CoverLandmark = Landmark;
				
				UActivityManager::GiveActivityTo(SearchLandmarkActivity, GetCharacter(), true, false);
			}
		}
	}
}

void USearchAndSecureActivity::ResetData()
{
	Super::ResetData();

	OnSearchComplete.Clear();
	OriginalLocation = FVector::ZeroVector;
	bReturnToOriginalLocation = false;
	SearchingRoom = nullptr;
	ClosestSecurable = nullptr;
	TimeSinceLastSecure = 0.0f;
	SecureCooldown = 0.0f;
	bBreachedFromFront = false;
	CommandTeam = ETeamType::TT_NONE;
	CommandLocation = FVector::ZeroVector;
	bAuto = false;
}

// Copyright Void Interactive, 2023

#include "Info/Activities/TrailerSearchAndSecureActivity.h"

#include "PickUpCharacterActivity.h"
#include "Actors/CoverLandmark.h"
#include "Actors/Gameplay/CollectedEvidenceActor.h"
#include "Actors/Gameplay/EvidenceActor.h"
#include "Characters/AI/SWATController.h"
#include "Characters/AI/TrailerSWATCharacter.h"
#include "Info/SWATManager.h"
#include "Team/CollectEvidenceActivity.h"

// todo: teleport back when finished putting down AI

UTrailerSearchAndSecureActivity::UTrailerSearchAndSecureActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "SearchAndSecure");
	bAbortIfTrackingEnemy = false;
	bPauseIfTrackingEnemy = true;
	bFinishActivityWhenOverriden = false;
	bIsProgressActivity = true;
}

void UTrailerSearchAndSecureActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	if (AllSecurables.Num() == 0)
	{
		ACTIVITY_FAILED("No securables to secure");
		return;
	}
}

void UTrailerSearchAndSecureActivity::PerformActivity(float DeltaTime)
{
	Super::PerformActivity(DeltaTime);
	
	if (!IsValid(ClosestSecurable) || ClosestSecurable->IsHidden() || !ISecurable::Execute_CanBeSecuredByTrailers(ClosestSecurable))
	{
		ClosestSecurable = nullptr;
		
		float ClosestDistance = FLT_MAX;
		for (AActor* Actor : AllSecurables) // assumed to be sorted and filtered beforehand
		{
			if (!IsValid(Actor))
				continue;
			
			if (ISecurable::Execute_CanBeSecuredByTrailers(Actor) && !Actor->IsHidden())
			{
				bool bAnyAISecuringThis = false;
				for (const ATrailerSWATCharacter* Swat : USWATManager::Get(this)->SwatTrailers)
				{
					if (Swat != GetCharacter() && Swat->GetCyberneticsController())
					{
						if (const UTrailerSearchAndSecureActivity* OtherActivity = Swat->GetCyberneticsController<ASWATController>()->GetTrailerSearchAndSecureActivity())
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
			MoveAcceptanceRadius = 100.0f;
			SetLocation(SpawnLocation, true);

			if (HasReachedLocation(300.0f))
			{
				OwningController->FinishActivity(this, true, true);
			}
			
			return;
		}
	}
	
	TrySecure();
}

bool UTrailerSearchAndSecureActivity::CanFinishActivity() const
{
	return false;
}

void UTrailerSearchAndSecureActivity::TrySecure()
{
	if (Cast<ACollectedEvidenceActor>(ClosestSecurable) || Cast<ABaseItem>(ClosestSecurable) || Cast<AEvidenceActor>(ClosestSecurable))
	{
		ProgressState = FText::FromStringTable("SwatCommandTable", "SecuringEvidence");

		if (!OwningController->GetActivity<UCollectEvidenceActivity>())
		{
			if (UCollectEvidenceActivity* CollectEvidenceActivity = GetOwningController<ASWATController>()->GetCollectEvidenceActivity())
			{
				CollectEvidenceActivity->EvidenceItem = ClosestSecurable;

				UActivityManager::GiveActivityTo(CollectEvidenceActivity, GetCharacter(), true, false);
			}
		}
	}
	else if (ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(ClosestSecurable))
	{
		if (AI->IsSuspect())
			ProgressState = FText::FromStringTable("SwatCommandTable", "SecuringSuspect");
		else
			ProgressState = FText::FromStringTable("SwatCommandTable", "SecuringCivilian");
		
		if (!OwningController->GetActivity<UPickUpCharacterActivity>())
		{
			if (AI->IsArrested() || AI->IsArrestedAndDead() || AI->IsInRagdoll() || AI->IsDeadOrUnconscious() || AI->IsIncapacitated())
			{
				if (UPickUpCharacterActivity* PickUpCharacterActivity = GetOwningController<ASWATController>()->GetPickUpCharacterActivity())
				{
					PickUpCharacterActivity->PickUpCharacter = AI;
					PickUpCharacterActivity->FinalDestinationLocation = SpawnLocation;
					
					UActivityManager::GiveActivityTo(PickUpCharacterActivity, GetCharacter(), true, false);
				}
			}
		}
	}
}

void UTrailerSearchAndSecureActivity::ResetData()
{
	Super::ResetData();

	SearchingRoom = nullptr;
	ClosestSecurable = nullptr;
	CommandLocation = FVector::ZeroVector;
	SpawnLocation = FVector::ZeroVector;
}

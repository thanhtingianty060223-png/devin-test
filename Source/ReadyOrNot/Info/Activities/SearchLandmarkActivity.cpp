// Copyright Void Interactive, 2023

#include "Info/Activities/SearchLandmarkActivity.h"

#include "TakeCoverAtLandmarkActivity.h"
#include "Actors/CoverLandmark.h"
#include "Actors/CoverLandmarkProxy.h"
#include "Actors/Attachments/LaserAttachment.h"
#include "Actors/Attachments/LightAttachment.h"
#include "Actors/Items/BallisticsShield.h"
#include "Characters/CyberneticController.h"
#include "Info/Activities/ActivityManagerTemplates.h"

USearchLandmarkActivity::USearchLandmarkActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "SearchLandmark");

	bPauseIfTrackingEnemy = true;
	
	MoveAcceptanceRadius = 120.0f;

	ActivityStateMachine->AddState("Move To")
                        .BindEventEnter(MAKE_DELEGATE_BINDING(this, &USearchLandmarkActivity::EnterMoveToStage))
                        .CreateTransition("Search", MAKE_DELEGATE_BINDING(this, &USearchLandmarkActivity::CanSearchNow))
                        .CreateTransition("Complete", MAKE_DELEGATE_BINDING(this, &USearchLandmarkActivity::CanAbortSearch), 1);
	
	ActivityStateMachine->AddState("Search")
                        .BindEventEnter(MAKE_DELEGATE_BINDING(this, &USearchLandmarkActivity::EnterSearchStage))
                        .BindEventTick(MAKE_DELEGATE_BINDING(this, &USearchLandmarkActivity::TickSearchStage))
                        .CreateTransition("Complete", MAKE_DELEGATE_BINDING(this, &USearchLandmarkActivity::IsSearchFinished));
	
	ActivityStateMachine->AddState("Complete")
                        .BindEventEnter(MAKE_DELEGATE_BINDING(this, &USearchLandmarkActivity::EnterCompleteStage));
}

void USearchLandmarkActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	if (GetCharacter()->GetEquippedItem<ABallisticsShield>())
	{
		ACTIVITY_FAILED("Cannot search landmark with a shield", true);
		return;
	}
	
	if (!CoverLandmark)
	{
		ACTIVITY_FAILED("No cover landmark actor specified");
		return;
	}
	
	if (CoverLandmark->bClearedBySwat)
	{
		ACTIVITY_FAILED(CoverLandmark->GetName() + " has already been cleared", true);
		return;
	}
	
	CoverLandmark->bClearedBySwat = true;
	
	if (!CoverLandmark->bEnabled)
	{
		ACTIVITY_FAILED(CoverLandmark->GetName() + " is disabled", true);
		return;
	}

	if (CoverLandmark->EntryPoints.Num() == 0)
	{
		ACTIVITY_FAILED("No entry points available for " + CoverLandmark->GetName());
		return;
	}

	if (CoverLandmark->ExitPoints.Num() == 0)
	{
		ACTIVITY_FAILED("No exit points available for " + CoverLandmark->GetName());
		return;
	}
	
	if (!CoverLandmark->IdlePoint)
	{
		ACTIVITY_FAILED("No idle point specified for " + CoverLandmark->GetName());
		return;
	}

	FText LandmarkTypeString = FText::GetEmpty();
	switch (CoverLandmark->Type)
	{
		case ECoverLandmarkType::Closet:	LandmarkTypeString = FText::FromStringTable("SwatCommandTable", "Closet"); break;
		case ECoverLandmarkType::Bed:		LandmarkTypeString = FText::FromStringTable("SwatCommandTable", "Bed"); break;
		case ECoverLandmarkType::Table:		LandmarkTypeString = FText::FromStringTable("SwatCommandTable", "Table"); break;
		case ECoverLandmarkType::Corner:	LandmarkTypeString = FText::FromStringTable("SwatCommandTable", "Corner"); break;
		//TODO (Max): Make LandmarkName FText
		case ECoverLandmarkType::Custom:	LandmarkTypeString = FText::FromString(CoverLandmark->LandmarkName); break;
		default: break;
	}
	
	ActivityName = FText::Format(FText::FromStringTable("SwatCommandTable", "SearchingName"), LandmarkTypeString);
	
	bSeenAIExiting = false;
}

void USearchLandmarkActivity::PerformActivity(float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	if (CoverLandmark)
	{
		if (CoverLandmark->Type == ECoverLandmarkType::Closet)
			MoveAcceptanceRadius = 40.0f;
		
		if (!bSeenAIExiting)
		{
			bSeenAIExiting = UActivityManager::AnyAIHasActivity<UTakeCoverAtLandmarkActivity>([&](const UTakeCoverAtLandmarkActivity* Activity)
			{
				if (Activity->CoverLandmark == CoverLandmark && !Activity->IsActivityComplete())
				{
					if (Activity->IsExitingLandmark() && GetCharacter()->HasLineOfSightTo(CoverLandmark->GetActorLocation()))
					{
						return true;
					}
				}

				return false;
			});
		}
	}
}

float USearchLandmarkActivity::GetDestinationTolerance() const
{
	if (CoverLandmark && CoverLandmark->Type == ECoverLandmarkType::Closet)
		return 60.0f;
	
	return 125.0f;
}

bool USearchLandmarkActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (CoverLandmark)
	{
		if (FVector::Distance(CoverLandmark->GetActorLocation(), GetCharacter()->GetActorLocation()) < 600.0f)
		{
			FocalPoint = CoverLandmark->IdlePoint->GetActorLocation();
			return true;
		}
		
		FVector OutDir;	
		float OutLength;
		GetCharacter()->GetVelocity().ToDirectionAndLength(OutDir, OutLength);

		if (OutLength <= 0.0f)
			return false;
	
		FocalPoint = GetCharacter()->GetActorLocation() + OutDir * 500.0f;
		return true;
	}

	return false;
}

bool USearchLandmarkActivity::CanBePushed() const
{
	return true;
}

void USearchLandmarkActivity::RequestMoveFromPath(const FVector& InLocation, FNavPathSharedPtr NavPath)
{
	Super::RequestMoveFromPath(InLocation, NavPath);

	if (CoverLandmark)
	{
		if (!bPlayedVO)
		{
			bPlayedVO = true;
			
			FString VO = VO_SWAT_GENERAL::CALL_SEARCHING;
			switch (CoverLandmark->Type)
			{
				case ECoverLandmarkType::Closet:			VO = VO_SWAT_GENERAL::CALL_SEARCHING_CLOSET; break;
				case ECoverLandmarkType::Bed:				VO = VO_SWAT_GENERAL::CALL_SEARCHING_BED; break;
				case ECoverLandmarkType::Table:				VO = VO_SWAT_GENERAL::CALL_SEARCHING_TABLE; break;
				default:									break;
			}

			GetCharacter()->PlayRawVO(VO);
		}
	}
}

void USearchLandmarkActivity::ResetData()
{
	Super::ResetData();

	CoverLandmark = nullptr;
	SearchAnimation = "";
	bSearchStarted = false;
	bSeenAIExiting = false;
	bPlayedVO = false;
}

void USearchLandmarkActivity::EnterMoveToStage()
{
	if (const ACoverLandmarkProxy* Proxy = UReadyOrNotFunctionLibrary::FindFurthestActorFromLocation<ACoverLandmarkProxy>(GetCharacter()->GetActorLocation(), CoverLandmark->EntryPoints))
	{
		SetLocation(Proxy->GetActorLocation(), true);
	}
}

void USearchLandmarkActivity::EnterSearchStage()
{
	if (!CoverLandmark)
	{
		return;
	}
	
	switch (CoverLandmark->Type)
	{
		case ECoverLandmarkType::Closet:			SearchAnimation = "tp_search_closet"; break;
		case ECoverLandmarkType::Bed:				SearchAnimation = "tp_search_bed"; break;
		case ECoverLandmarkType::Table:				SearchAnimation = "tp_search_table"; break;
		case ECoverLandmarkType::Custom:			SearchAnimation = CoverLandmark->SwatSearchAnimation; break;
		default:									SearchAnimation = ""; break;
	}

	if (!SearchAnimation.IsEmpty())
	{
		if (GetCharacter()->PlayMontageFromTableWithFocalPoint(SearchAnimation, CoverLandmark->IdlePoint->GetActorLocation()))
		{
			CoverLandmark->bClearedBySwat = true;
			bSearchStarted = true;
		}
	}
	else // failsafe
	{
		CoverLandmark->bClearedBySwat = true;
		bSearchStarted = true;
		Notify_SearchLandmark();
	}
}

void USearchLandmarkActivity::TickSearchStage(float DeltaTime, float Uptime)
{
	if (CoverLandmark)
		CoverLandmark->bClearedBySwat = true;
	
	if (bSearchStarted)
	{
		if (ABaseMagazineWeapon* Weapon = GetCharacter()->GetEquippedWeapon())
		{
			if (Weapon->GetLightAttachment())
			{
				Weapon->GetLightAttachment()->ToggleLight(true);
			}
		
			if (Weapon->GetLaserAttachment())
			{
				Weapon->GetLaserAttachment()->ToggleLaser(true);
			}
		}
	}
}

void USearchLandmarkActivity::EnterCompleteStage()
{
	CoverLandmark->bClearedBySwat = true;
	
	// TODO: override light system? could be the case the we wanted it on even after finishing a search
	if (ABaseMagazineWeapon* Weapon = GetCharacter()->GetEquippedWeapon())
	{
		if (Weapon->GetLightAttachment())
		{
			Weapon->GetLightAttachment()->ToggleLight(false);
		}
	
		if (Weapon->GetLaserAttachment())
		{
			Weapon->GetLaserAttachment()->ToggleLaser(false);
		}
	}
	
	OwningController->FinishActivity(this, true, true);
}

bool USearchLandmarkActivity::CanSearchNow() const
{
	if (bSeenAIExiting)
		return false;
	
	return HasReachedLocation(GetDestinationTolerance());
}

bool USearchLandmarkActivity::CanAbortSearch() const
{
	return !CoverLandmark || bSeenAIExiting;
}

bool USearchLandmarkActivity::IsSearchFinished() const
{
	if (!CoverLandmark)
		return true;
	
	return SearchAnimation.IsEmpty() || (!bSearchStarted || (bSearchStarted && !GetCharacter()->IsTableMontagePlaying(SearchAnimation)));
}

void USearchLandmarkActivity::Notify_SearchLandmark_Implementation()
{
	if (CoverLandmark)
	{
		CoverLandmark->bClearedBySwat = true;
		
		if (CoverLandmark->IdlePoint)
			CoverLandmark->IdlePoint->OnProxyUse(true);
	}

	UActivityManager::IterateAllActivitiesOfType<UTakeCoverAtLandmarkActivity>([&](UTakeCoverAtLandmarkActivity* Activity)
	{
		if (Activity->CoverLandmark == CoverLandmark)
		{
			GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_YELL_HIDING);
			Activity->AbortCoverNow();
			return false;
		}
		
		return true;
	});
}

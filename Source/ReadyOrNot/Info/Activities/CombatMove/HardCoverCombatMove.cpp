// Void Interactive, 2020

#include "HardCoverCombatMove.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Actors/CoverLandmark.h"

#include "Info/Activities/BaseCombatActivity.h"
#include "Info/Activities/TakeCoverActivity.h"
#include "Info/Activities/TakeCoverAtLandmarkActivity.h"

#include "Info/Activities/Tasks/FindCoverTask.h"

#include "CoverSystem.h"

#include "ReadyOrNotAIConfig.h"
#include "Info/Activities/InvestigateStimulusActivity.h"
#include "Info/Activities/ReloadSafelyActivity.h"

void UHardCoverCombatMove::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	// Swat don't take hard cover
	if (GetCharacter()->IsOnSWATTeam())
	{
		OwningController->GetCombatActivity()->FinishCombatMove();
		return;
	}

	if (GetCharacter()->IsCivilian())
	{
		GetCharacter()->ReasonsToSprint.AddUnique("hard cover");
	}

	if (!IsValid(TakeCoverActivity))
	{
		TakeCoverActivity = UActivityManager::CreateActivity<UTakeCoverActivity>(this);
		if (TakeCoverActivity)
		{
			TakeCoverActivity->InitActivity(Owner);

			TakeCoverActivity->OnFinishActivity.RemoveAll(this);
			TakeCoverActivity->OnFinishActivity.AddDynamic(this, &UHardCoverCombatMove::OnTakeCoverActivityFinished);
		}
	}
	else
	{
		TakeCoverActivity->ResetCoverData();
	}
	
	if (!IsValid(TakeCoverAtLandmarkActivity))
	{
		TakeCoverAtLandmarkActivity = UActivityManager::CreateActivity<UTakeCoverAtLandmarkActivity>(this);
		if (TakeCoverAtLandmarkActivity)
		{
			TakeCoverAtLandmarkActivity->InitActivity(Owner);
			
			TakeCoverAtLandmarkActivity->OnFinishActivity.RemoveAll(this);
			TakeCoverAtLandmarkActivity->OnFinishActivity.AddDynamic(this, &UHardCoverCombatMove::OnTakeCoverAtLandmarkActivityFinished);
		}
	}
	
	OwningController->GetCombatActivity()->OnTrackNewEnemy.RemoveAll(this);
	OwningController->GetCombatActivity()->OnTrackNewEnemy.AddDynamic(this, &UHardCoverCombatMove::TrackNewEnemy);
}

void UHardCoverCombatMove::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	if (GetCharacter()->IsCivilian())
	{
		GetCharacter()->ReasonsToSprint.Remove("hard cover");
	}
	
	if (TakeCoverActivity && OwningController->GetCurrentActivity() == TakeCoverActivity)
	{
		TakeCoverActivity->AbortCoverNow(EAbortCoverReason::Forced);
	}

	if (TakeCoverAtLandmarkActivity && OwningController->GetCurrentActivity() == TakeCoverAtLandmarkActivity)
	{
		TakeCoverAtLandmarkActivity->AbortCoverNow();
	}
	
	if (!GetWorld())
		return;

	if (COVER_SYSTEM)
		COVER_SYSTEM->ReleaseCover(LastBestCover.CoverLocation);

	if (AReadyOrNotLevelScript* LevelScript = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
	{
		LevelScript->AIRequestingCover.Remove(GetCharacter());
	}
}

bool UHardCoverCombatMove::TryMoveIntoCover(const FCoverInstigatorStimulus& InstigatorStimulus, bool bRequireLOS)
{
	if (!OwningController || !GetCharacter() || !InstigatorStimulus.IsValid())
		return false;

	if (IsValid(InstigatorStimulus.InstigatorCharacter))
	{
		if (!InstigatorStimulus.InstigatorCharacter->IsActive())
			return false;
	}
	
	if (!GetCharacter()->IsActive())
		return false;

	if (OwningController->GetActivity<UTakeCoverActivity>())
		return false;
	
	if (OwningController->GetActivity<UTakeCoverAtLandmarkActivity>())
		return false;

	// Still searching.. (only allow one AI to search at a time, to make sure we filter out occupied cover points)
	if (AReadyOrNotLevelScript::bAnyAIRequestingCover || IsRequestingCover())
		return true;
	
	// Don't try to cover if instigator is on same team
	if (IsValid(InstigatorStimulus.InstigatorCharacter))
	{
		if (GetCharacter()->GetTeam() == InstigatorStimulus.InstigatorCharacter->GetTeam())
			return false;
	}

	const FVector BaseLocation = GetCharacter()->GetActorLocation() - GetCharacter()->GetActorUpVector() * 80.0f;

	// If already on a cover point, don't find new cover
	if (COVER_SYSTEM->IsCoverPointOccupied(LastBestCover.CoverLocation) && FVector::Distance(BaseLocation, LastBestCover.CoverLocation) < GetCharacter()->GetCapsuleComponent()->GetUnscaledCapsuleRadius())
		return false;

	if (bRequireLOS)
	{
		FHitResult HitResult;
		const FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), InstigatorStimulus.InstigatorCharacter);
		FVector TraceStart = GetCharacter()->GetMesh()->GetSocketLocation("head_end");
		FVector TraceEnd = InstigatorStimulus.ThreatTransform.GetLocation();
		
		// Do we have LOS to the instigator?
		if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, CollisionQueryParams))
			return false;
	}

	if (InstigatorStimulus.InstigatorCharacter)
		LastTrackedEnemy = InstigatorStimulus.InstigatorCharacter;
		
	LastCoverInstigatorStimulus = InstigatorStimulus;
	
	return RequestCover(InstigatorStimulus);
}

bool UHardCoverCombatMove::TryMoveIntoCoverLandmark(const FVector& ThreatLocation, const FVector& ThreatDirection, const float MinDistanceFromInstigator, AReadyOrNotCharacter* InstigatorCharacter)
{
	if (!GetCharacter())
		return false;
	
	if (!GetCharacter()->IsActive())
		return false;

	if (OwningController->GetActivity<UTakeCoverActivity>())
		return false;
	
	if (OwningController->GetActivity<UTakeCoverAtLandmarkActivity>())
		return false;
	
	if (AReadyOrNotLevelScript::bAnyAIRequestingCover || IsRequestingCover())
		return false;

	const FVector DirectionToThreat = (GetCharacter()->GetActorLocation() - ThreatLocation).GetSafeNormal();
	const float ForwardDotProduct = FVector::DotProduct(DirectionToThreat, ThreatDirection.GetSafeNormal());
	
	// Before finding any cover..
	// Is the threat direction looking at us?
	if (ForwardDotProduct < 0.95f)
		return false;

	return TryMoveIntoCoverLandmark(MinDistanceFromInstigator, InstigatorCharacter);
}

bool UHardCoverCombatMove::TryMoveIntoCoverLandmark(const float MinDistanceFromInstigator, AReadyOrNotCharacter* InstigatorCharacter)
{
	const float SearchRadius = MinDistanceFromInstigator <= 0.0f ? AI_CONFIG_GET_FLOAT("CoverLandmarkSearchExtent", 750.0f) : MinDistanceFromInstigator;

	const FVector Origin = GetCharacter()->GetActorLocation() - FVector::UpVector * 100.0f;

	const FVector SearchBoxExtent = FVector(SearchRadius, SearchRadius, 205.0f);

	const TArray<ACoverLandmark*>& NearbyCoverLandmarks = FindCoverLandmarksInArea(Origin, SearchBoxExtent);
	
	if (ACoverLandmark* ClosestCoverLandmark = UReadyOrNotFunctionLibrary::FindClosestActorFromLocation(GetCharacter()->GetActorLocation(), NearbyCoverLandmarks))
	{
		LastTrackedEnemy = InstigatorCharacter;
		
		GiveTakeCoverAtLandmarkActivity(ClosestCoverLandmark);
		
		OnRequestCoverLandmark.Broadcast();
		
		return true;
	}

	return false;
}

bool UHardCoverCombatMove::RequestCover(const FCoverInstigatorStimulus& InstigatorStimulus)
{
	if (!GetCharacter() || !InstigatorStimulus.IsValid())
		return false;

	if (!IsActive())
		return false;
	
	// Find cover asynchronously

	AReadyOrNotLevelScript::bAnyAIRequestingCover = true;
	
	COVER_SYSTEM->ReleaseCover(LastBestCover.CoverLocation);

	if (AReadyOrNotLevelScript* LevelScript = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
	{
		LevelScript->AIRequestingCover.AddUnique(GetCharacter());
	}

	float SearchRadius = InstigatorStimulus.SearchRadius <= 0.0f ? FMath::Clamp(AI_CONFIG_GET_FLOAT("CoverSearchExtent", 1250.0f), 100.0f, 10000.0f) : InstigatorStimulus.SearchRadius;
	const float DangerRadius = InstigatorStimulus.ExclusionRadiusFromInstigator <= 0.0f ? FMath::Clamp(AI_CONFIG_GET_FLOAT("CoverSearchExclusionRadiusAroundInstigator", 850.0f), 100.0f, SearchRadius+100.0f) : InstigatorStimulus.ExclusionRadiusFromInstigator;
	
	FFindCoverDelegate NewCoverFoundDelegate;
	NewCoverFoundDelegate.BindUObject(this, &UHardCoverCombatMove::OnNewCoverFoundAsync);
	
	FFindCoverDelegate NoCoverFoundDelegate;
	NoCoverFoundDelegate.BindUObject(this, &UHardCoverCombatMove::OnNoCoverFoundAsync);
	
	LastFindCoverQuery = {};
	LastFindCoverQuery.World = GetWorld();

	// Set the transforms
	FTransform OurTransform = GetCharacter()->GetActorTransform();
	OurTransform.SetLocation(OurTransform.GetLocation() + OurTransform.GetUnitAxis(EAxis::Z) * 70.0f);

	FTransform InstigatorTransform = FTransform::Identity;
	
	if (InstigatorStimulus.bUseThreatTransformAsInstigatorTransform)
	{
		InstigatorTransform = InstigatorStimulus.ThreatTransform;
	}
	else
	{
		InstigatorTransform = InstigatorStimulus.InstigatorCharacter->GetActorTransform();
		InstigatorTransform.SetLocation(InstigatorTransform.GetLocation() + InstigatorTransform.GetUnitAxis(EAxis::Z) * 70.0f);
	}

	LastFindCoverQuery.OurTransform = OurTransform;
	LastFindCoverQuery.InstigatorTransform = InstigatorTransform;

	if (InstigatorStimulus.bUseThreatTransformAsInstigatorTransform)
		LastFindCoverQuery.InstigatorTransform = InstigatorStimulus.ThreatTransform;
	
	LastFindCoverQuery.CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(GetCharacter(), InstigatorStimulus.InstigatorCharacter);

	// Knife/Other weapon wielding characters should find a wall only cover and ambush attck
	const bool bHoldingFirearm = GetCharacter()->GetEquippedItem<ABaseMagazineWeapon>() != nullptr;
	
	LastFindCoverQuery.SearchMode = bHoldingFirearm ? ECoverSearchMode::NonWallOnly : ECoverSearchMode::Both;
	//LastFindCoverQuery.SearchMode = ECoverSearchMode::NonWallOnly;
	LastFindCoverQuery.CoverStance = ECoverStance::Both;
	
	LastFindCoverQuery.SearchDangerRadiusFromInstigator = bHoldingFirearm ? DangerRadius : DangerRadius/2;
	
	if (SearchRadius < LastFindCoverQuery.SearchDangerRadiusFromInstigator)
		SearchRadius = LastFindCoverQuery.SearchDangerRadiusFromInstigator + 200.0f;
	
	LastFindCoverQuery.SearchExtent = SearchRadius;

	LastFindCoverQuery.NavQueryFilter = OwningController->GetNavQueryFilter();

	#if !UE_BUILD_SHIPPING
	LastFindCoverQuery.bDrawDebug = false;
	LastFindCoverQuery.DrawTime = 10.0f;
	#endif

	CurrentFindCoverData = MakeShared<FCoverData>();
	CurrentFindCoverTask = new FAutoDeleteAsyncTask<FFindCoverTask>(CurrentFindCoverData, LastFindCoverQuery, NewCoverFoundDelegate, NoCoverFoundDelegate);
	
	#if !UE_BUILD_SHIPPING
	if (LastFindCoverQuery.bDrawDebug)
		CurrentFindCoverTask->StartSynchronousTask();
	else
		CurrentFindCoverTask->StartBackgroundTask();
	#else
		CurrentFindCoverTask->StartBackgroundTask();
	#endif

	OnRequestCover.Broadcast();

	return true;
}

EAbortCoverReason UHardCoverCombatMove::GetLastAbortCoverReason() const
{
	return TakeCoverActivity->LastAbortCoverReason;
}

bool UHardCoverCombatMove::ShouldForceStrafe() const
{
	if (GetCharacter()->Rep_HidingAnimState.bIsHiding)
		return false;

	return true;
}

bool UHardCoverCombatMove::ShouldForceNoStrafe() const
{
	if (GetCharacter()->Rep_HidingAnimState.bIsHiding)
		return true;

	return false;
}

bool UHardCoverCombatMove::CanShoot() const
{
	if (GetCharacter()->Rep_HidingAnimState.bIsHiding)
		return false;
	
	return Super::CanShoot();
}

void UHardCoverCombatMove::RequestCombatMove(const float DeltaTime)
{
	Super::RequestCombatMove(DeltaTime);

	// Failsafe, if the delegates are not called to remove from request list
	if (!CurrentFindCoverTask)
	{
		if (AReadyOrNotLevelScript* LevelScript = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
		{
			LevelScript->AIRequestingCover.Remove(GetCharacter());
		}
	}
	
	if (!IsRequestingCover() && !OwningController->GetActivity<UTakeCoverActivity>() && !PendingNewCover)
	{
		FinishCombatMove(false);
	}
}

void UHardCoverCombatMove::OnKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter)
{
	Super::OnKilled(InstigatorCharacter, KilledCharacter);

	// Make sure we release this cover if FinishedActivity somehow is not called, rendering this cover point useless for the entire game for other AI
	COVER_SYSTEM->ReleaseCover(LastBestCover.CoverLocation);
}

TArray<ACoverLandmark*> UHardCoverCombatMove::FindCoverLandmarksInArea(const FVector& InLocation, const FVector& InExtent) const
{
	TArray<ACoverLandmark*> FoundLandmarks;
	FoundLandmarks.Reserve(5);
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		for (ACoverLandmark* CoverLandmark : GS->AllCoverLandmarks)
		{
			if (!CoverLandmark->bEnabled)
			{
				continue;
			}

			bool bBypassChecks = false;
			#if !UE_BUILD_SHIPPING
			if (const UAIArchetypeData* Archetype = GetCharacter()->GetAIArchetype())
			{
				bBypassChecks = Archetype->Name == "DEBUG Hide Only";
			}
			#endif
			
			if (!bBypassChecks)
			{
				if (!CoverLandmark->AllowedTeamsForCover.Contains(GetCharacter()->GetTeam()))
				{
					continue;
				}
			}
			
			if (CoverLandmark->OccupiedByController != nullptr)
			{
				continue;
			}
			
			if (UKismetMathLibrary::IsPointInBox(CoverLandmark->GetActorLocation(), InLocation, InExtent))
			{
				FoundLandmarks.AddUnique(CoverLandmark);
			}
		}
	}

	return FoundLandmarks;
}

void UHardCoverCombatMove::GiveTakeCoverActivity(const FCoverData& InCoverData)
{
	if (!OwningController || !GetCharacter())
		return;

	if (!GetCharacter()->IsActive())
		return;

	if (!IsCoverValid(InCoverData))
		return;

	bool bAnyAIOccupiedCoverPoint = false;
	for (TObjectIterator<UTakeCoverActivity> It; It; ++It)
	{
		const UTakeCoverActivity* OtherTakeCoverActivity = *It;
		if (OtherTakeCoverActivity->GetOwningController() != GetOwningController())
		{
			if (OtherTakeCoverActivity->CoverData == InCoverData)
			{
				bAnyAIOccupiedCoverPoint = true;
			}
		}
	}

	if (bAnyAIOccupiedCoverPoint)
		return;

	if (UTakeCoverActivity* CurrentTakeCoverActivity = OwningController->GetCurrentActivity<UTakeCoverActivity>())
	{
		if (!CurrentTakeCoverActivity->IsMovingToCover())
		{
			PendingNewCover = &InCoverData;
			PendingCoverInstigatorStimulus = LastCoverInstigatorStimulus;
			CurrentTakeCoverActivity->AbortCoverNow();
		}
		
		return;
	}

	//#if !UE_BUILD_SHIPPING
	//DrawDebugBox(GetWorld(), InCoverData.CoverLocation, FVector(25.0f), FColor::Yellow, false, 10.0f);
	//#endif

	TakeCoverActivity->ResetCoverData();
	TakeCoverActivity->CoverData = InCoverData;
	TakeCoverActivity->InstigatorStimulus = LastCoverInstigatorStimulus;
	TakeCoverActivity->SetLocation(InCoverData.CoverLocation);
	
	UActivityManager::GiveActivityTo(TakeCoverActivity, GetCharacter(), true, false);
}

void UHardCoverCombatMove::GiveTakeCoverAtLandmarkActivity(ACoverLandmark* InCoverLandmark)
{
	if (!OwningController || !GetCharacter())
		return;
	
	if (OwningController->GetCurrentActivity<UTakeCoverActivity>())
		return;

	if (OwningController->GetCurrentActivity<UTakeCoverAtLandmarkActivity>())
		return;

	#if !UE_BUILD_SHIPPING
	//if (InCoverLandmark)
	//	DrawDebugBox(GetWorld(), InCoverLandmark->GetActorLocation(), FVector(25.0f), FColor::Purple, false, 10.0f);
	#endif

	TakeCoverAtLandmarkActivity->ResetData();
	TakeCoverAtLandmarkActivity->CoverLandmark = InCoverLandmark;
	
	UActivityManager::GiveActivityTo(TakeCoverAtLandmarkActivity, GetCharacter(), true, false);
}

bool UHardCoverCombatMove::IsRequestingCover() const
{
	if (const AReadyOrNotLevelScript* LevelScript = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
	{
		return LevelScript->AIRequestingCover.Contains(GetCharacter());
	}

	return false;
}

bool UHardCoverCombatMove::IsMovingToCover() const
{
	if (const UTakeCoverActivity* CurrentTakeCoverActivity = OwningController->GetCurrentActivity<UTakeCoverActivity>())
	{
		return CurrentTakeCoverActivity->IsMovingToCover();
	}

	if (OwningController->GetCurrentActivity<UTakeCoverAtLandmarkActivity>())
	{
		return true;
	}

	return false;
}

void UHardCoverCombatMove::AbortCoverNow()
{
	if (UTakeCoverActivity* CurrentTakeCoverActivity = OwningController->GetCurrentActivity<UTakeCoverActivity>())
	{
		CurrentTakeCoverActivity->AbortCoverNow();
	}
}

void UHardCoverCombatMove::OnMoveInterrupted(UBaseActivity* Activity)
{
	InterruptActivity = nullptr;
	
	if (Cast<UTakeCoverActivity>(Activity))
		return;
	
	if (Cast<UTakeCoverAtLandmarkActivity>(Activity))
		return;

	if (Cast<UInvestigateStimulusActivity>(Activity))
		return;
	
	if (Cast<UReloadSafelyActivity>(Activity))
		return;
	
	Super::OnMoveInterrupted(Activity);
}

#if !UE_BUILD_SHIPPING
void UHardCoverCombatMove::GatherDebugString(FString& OutString)
{
	if (TakeCoverActivity == OwningController->GetCurrentActivity())
	{
		FString TakeCoverDebug;
		TakeCoverActivity->GatherDebugString(TakeCoverDebug);
		
		OutString += TakeCoverDebug;
	}

	if (TakeCoverAtLandmarkActivity == OwningController->GetCurrentActivity())
	{
		FString TakeCoverDebug;
		TakeCoverAtLandmarkActivity->GatherDebugString(TakeCoverDebug);

		OutString += TakeCoverDebug;
	}
}
#endif

FName UHardCoverCombatMove::GetMoveStyleOverride_Implementation() const
{
	if (GetCharacter()->IsCivilian())
	{
		return "male01_civilian_cowering";
	}

	return NAME_None;
}

void UHardCoverCombatMove::OnTakeCoverActivityFinished(UBaseActivity* Activity, ACyberneticController* Controller)
{
	if (!OwningController)
		return;

	if (PendingNewCover)
	{
		#if !UE_BUILD_SHIPPING
		DrawDebugBox(GetWorld(), PendingNewCover->CoverLocation, FVector(25.0f), FColor::Yellow, false, 10.0f);
		#endif
		
		TakeCoverActivity->ResetCoverData();
		TakeCoverActivity->CoverData = *PendingNewCover;
		TakeCoverActivity->InstigatorStimulus = PendingCoverInstigatorStimulus;
		TakeCoverActivity->SetLocation(PendingNewCover->CoverLocation);

		PendingNewCover = nullptr;
		PendingCoverInstigatorStimulus = {};

		UActivityManager::GiveActivityTo(TakeCoverActivity, GetCharacter(), true, false);
		return;
	}
	
	OnCoverExit.Broadcast();

	FinishCombatMove();
}

void UHardCoverCombatMove::OnTakeCoverAtLandmarkActivityFinished(UBaseActivity* Activity, ACyberneticController* Controller)
{
	if (OwningController->GetCombatActivity())
		OwningController->GetCombatActivity()->FinishCombatMove();

	OnCoverLandmarkExit.Broadcast();
}

void UHardCoverCombatMove::OnNewCoverFoundAsync(uint32 NumCoverFound, float TimeMs)
{
	if (!OwningController || !GetCharacter())
		return;

	if (!IsActive())
		return;

	if (!ensure(CurrentFindCoverData))
		return;
	
	// Set our last best cover we got from the FindCoverTask
	LastBestCover = *CurrentFindCoverData;
	
	// #if !UE_BUILD_SHIPPING
	// ensure(Result.BestCover != nullptr);
	// #endif
	//
	// if (!Result.BestCover)
	// 	return;
	
	#if !UE_BUILD_SHIPPING
	ULog::Info(GetNameSafe(OwningController) + " | New Cover Found " + LastBestCover.CoverLocation.ToString());
	
	ULog::Number(NumCoverFound, GetNameSafe(OwningController) + " | Found ", " cover points in area");
	ULog::Number(TimeMs, GetNameSafe(OwningController) + " | Cover Search Time: ", "ms");

	/*if (Result.Path.IsValid())
	{
		TArray<FNavPathPoint> PathPoints = Result.Path->GetPathPoints();

		for (int32 i = 1; i < PathPoints.Num(); i++)
		{
			DrawDebugLine(GetWorld(), PathPoints[i-1].Location + FVector(0.0f, 0.0f, 35.0f), PathPoints[i].Location + FVector(0.0f, 0.0f, 35.0f), FColor::Green, false, 10.0f, 0, 5);
		}
	}*/

	//Debug_DrawCoverFound(LastTrackedEnemy, LastBestCover.CoverLocation);
	#endif

	if (!GetWorld())
		return;
	
	OnCoverFound(LastBestCover);

	if (AReadyOrNotLevelScript* LevelScript = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
	{
		LevelScript->AIRequestingCover.Remove(GetCharacter());
	}

	NewCoverFound.Broadcast();
}

void UHardCoverCombatMove::OnNoCoverFoundAsync(uint32 NumCoverFound, float TimeMs)
{
	if (!OwningController || !GetCharacter())
		return;

	#if !UE_BUILD_SHIPPING
	ULog::Info(OwningController->GetName() + " | No Cover Found");

	ULog::Number(NumCoverFound, OwningController->GetName() + " | Found ", " cover points in area");
	ULog::Number(TimeMs, OwningController->GetName() + " | Cover Search Time: ", "ms");
	#endif

	if (!GetWorld())
		return;

	if (AReadyOrNotLevelScript* LevelScript = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor()))
	{
		LevelScript->AIRequestingCover.Remove(GetCharacter());
	}

	NoCoverFound.Broadcast();
}

void UHardCoverCombatMove::OnCoverFound(const FCoverData& InCoverData)
{
	if (!IsCoverValid(InCoverData))
		return;

	if (!OwningController || !GetCharacter())
		return;

	if (!GetCharacter()->IsActive())
		return;
	
	if (!GetCharacter()->IsActiveForCombat())
		return;
	
	if (!IsActive())
		return;

	COVER_SYSTEM->OccupyCover(InCoverData.CoverLocation);

	GiveTakeCoverActivity(InCoverData);
}

void UHardCoverCombatMove::TrackNewEnemy(AReadyOrNotCharacter* NewTrackedEnemy)
{
	if (!NewTrackedEnemy)
		return;

	if (!LastTrackedEnemy)
		return;

	if (GetCharacter()->IsFiringFromCover())
	{
		if (UTakeCoverActivity* Activity = OwningController->GetCurrentActivity<UTakeCoverActivity>())
		{
			Activity->InstigatorStimulus.InstigatorCharacter = NewTrackedEnemy;
		}
		return;
	}

	if (LastTrackedEnemy == NewTrackedEnemy)
		return;

	LastTrackedEnemy = NewTrackedEnemy;

	ResetCoverData();

	#if !UE_BUILD_SHIPPING
	ULog::Info(CUR_CLASS_FUNC_2 + LastTrackedEnemy->GetName());
	#endif
}

void UHardCoverCombatMove::ResetCoverData()
{
	COVER_SYSTEM->ReleaseCover(LastBestCover.CoverLocation);

	AbortCoverNow();
}

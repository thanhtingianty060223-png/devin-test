// Void Interactive, 2020

#include "CollectEvidenceActivity.h"

#include "Actors/Gameplay/EvidenceActor.h"
#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Info/SWATManager.h"

UCollectEvidenceActivity::UCollectEvidenceActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "SecureEvidence");
	bIsProgressActivity = true;
	MaxActivityTime = 30.0f;
	bPauseIfTrackingEnemy = true;
	MoveAcceptanceRadius = 50.0f;
	bPauseIfRecentlyInCombat = true;
	bAbortActivityIfCannotReachLocation = true;

	ActivityStateMachine->AddState("Move To")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UCollectEvidenceActivity::EnterMoveToStage))
						.CreateTransition("Collect", MAKE_DELEGATE_BINDING(this, &UCollectEvidenceActivity::CanCollectEvidence));

	ActivityStateMachine->AddState("Collect")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UCollectEvidenceActivity::EnterCollectStage))
						.BindEventTick(MAKE_DELEGATE_BINDING(this, &UCollectEvidenceActivity::TickCollectStage));
}

void UCollectEvidenceActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	if (CheckEdgeCases())
	{
		BindEvents();
	}
}

void UCollectEvidenceActivity::ResumeActivity()
{
	Super::ResumeActivity();

	if (CheckEdgeCases())
	{
		BindEvents();
	}
}

void UCollectEvidenceActivity::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	UnbindEvents();

	if (!bSuccess)
		StopCollectEvidenceAnim();
}

void UCollectEvidenceActivity::PerformActivity(float DeltaTime)
{
	Super::PerformActivity(DeltaTime);
}

void UCollectEvidenceActivity::FinishedActivity_NoOwner(bool bSuccess)
{
	Super::FinishedActivity_NoOwner(bSuccess);

	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		UBpGameplayHelperLib::EnableInteractionForController(EvidenceItem, *It);
	}
}

void UCollectEvidenceActivity::ActivityOverriden(UBaseActivity* OverridingActivity)
{
	Super::ActivityOverriden(OverridingActivity);

	UnbindEvents();

	StopCollectEvidenceAnim();
}

bool UCollectEvidenceActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (FVector::Distance(EvidenceItem->GetActorLocation(), GetCharacter()->GetActorLocation()) < 500)
	{
		if (const ABaseItem* ItemAsEvidence = Cast<ABaseItem>(EvidenceItem))
		{
			FocalPoint = ItemAsEvidence->GetItemLocation();
		}
		else if (const AEvidenceActor* EvidenceActor = Cast<AEvidenceActor>(EvidenceItem))
		{
			FocalPoint = EvidenceActor->GetActorLocation();
		}
		
		const FVector StartTrace = GetCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
		const FVector EndTrace = FocalPoint;
		FCollisionQueryParams CollisionQueryParams = GetCharacter()->GetCollisionQueryParameters();
		CollisionQueryParams.AddIgnoredActor(EvidenceItem);
		if (!GetWorld()->LineTraceTestByChannel(StartTrace, EndTrace, ECC_Visibility, CollisionQueryParams))
		{
			return true;
		}
	}

	return false;
}

bool UCollectEvidenceActivity::CanFinishActivity() const
{
	// Must be force finished
	return false;
}

void UCollectEvidenceActivity::ResetData()
{
	Super::ResetData();
	
	EvidenceItem = nullptr;

	bCalledOutMove = false;
	
	GetWorld()->GetTimerManager().ClearTimer(TH_CollectEvidenceAnim);
}

void UCollectEvidenceActivity::RequestMoveFromPath(const FVector& InLocation, FNavPathSharedPtr NavPath)
{
	if (CheckEdgeCases())
	{
		Super::RequestMoveFromPath(InLocation, NavPath);
		
		if (!bCalledOutMove && FVector::Distance(EvidenceItem->GetActorLocation(), GetCharacter()->GetActorLocation()) > 300.0f)
		{
			bCalledOutMove = GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_COLLECT_EVIDENCE_MOVE);
		}
	}
}

void UCollectEvidenceActivity::EnterMoveToStage()
{
	if (CheckEdgeCases())
	{
		SetLocation(EvidenceItem->GetActorLocation(), true, FVector(200.0f));
	}
}

void UCollectEvidenceActivity::EnterCollectStage()
{
	if (CheckEdgeCases())
	{
		GetCharacter()->PickupEvidence(EvidenceItem);
	}
}

void UCollectEvidenceActivity::TickCollectStage(float DeltaTime, float Uptime)
{
	if (HasReachedLocation(150.0f) && Uptime > 1.0f) // failsafe
	{
		if (CheckEdgeCases())
		{
			GetCharacter()->PickupEvidence(EvidenceItem);
		}
	}
}

bool UCollectEvidenceActivity::CanCollectEvidence() const
{
	return HasReachedLocation(150.0f) && EvidenceItem && !EvidenceItem->IsHidden();
}

void UCollectEvidenceActivity::OnCollectEvidenceBegin()
{
	GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_COLLECT_EVIDENCE, "", false);

	ProgressState = FText::FromStringTable("SwatCommandTable", "Collecting");
}

void UCollectEvidenceActivity::OnCollectEvidenceEnd()
{
	UnbindEvents();

	float TimeRemaining;
	if (GetCharacter()->IsTableMontagePlayingWithTimeRemaining("tp_swat_collect_evidence", TimeRemaining))
	{
		UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_CollectEvidenceAnim, this, &UCollectEvidenceActivity::OnCollectEvidenceAnimFinished, TimeRemaining - 0.1f);
	}

	if (!TH_CollectEvidenceAnim.IsValid())
	{
		OnCollectEvidenceAnimFinished();
	}

	ProgressState = FText::FromStringTable("ScoringTable", "EvidenceSecured");
}

void UCollectEvidenceActivity::OnCollectEvidenceAnimFinished()
{
	OwningController->FinishActivity(this, true, true);
}

void UCollectEvidenceActivity::OnEvidenceCollected()
{
	if (OwningController)
		OwningController->FinishActivity(this, true, true);
}

bool UCollectEvidenceActivity::CheckEdgeCases()
{
	if (!IsValid(EvidenceItem))
	{
		ACTIVITY_FAILED("No valid evidence item specified", true);
		return false;
	}
	
	if (EvidenceItem->IsPendingKill()) // base item evidence get destroyed upon collection
	{
		ACTIVITY_FAILED("Evidence item has already been collected", true);
		return false;
	}

	if (EvidenceItem->IsHidden())
	{
		ACTIVITY_FAILED("Evidence item has already been collected", true);
		return false;
	}

	return true;
}

void UCollectEvidenceActivity::BindEvents()
{
	UnbindEvents();

	if (ABaseItem* Item = Cast<ABaseItem>(EvidenceItem))
	{
		Item->OnEvidenceCollected.RemoveAll(this);
		Item->OnEvidenceCollected.AddDynamic(this, &UCollectEvidenceActivity::OnEvidenceCollected);
	}
	else if (AEvidenceActor* ItemAsEvidenceActor = Cast<AEvidenceActor>(EvidenceItem))
	{
		ItemAsEvidenceActor->OnActorPickedUp_NoParam.RemoveAll(this);
		ItemAsEvidenceActor->OnActorPickedUp_NoParam.AddDynamic(this, &UCollectEvidenceActivity::OnEvidenceCollected);
	}

	GetCharacter()->OnCollectPendingEvidenceBegin_FromAnimNotify.RemoveAll(this);
	GetCharacter()->OnCollectPendingEvidenceBegin_FromAnimNotify.AddDynamic(this, &UCollectEvidenceActivity::OnCollectEvidenceBegin);
	GetCharacter()->OnCollectPendingEvidenceEnd_FromAnimNotify.RemoveAll(this);
	GetCharacter()->OnCollectPendingEvidenceEnd_FromAnimNotify.AddDynamic(this, &UCollectEvidenceActivity::OnCollectEvidenceEnd);
}

void UCollectEvidenceActivity::UnbindEvents()
{
	if (ABaseItem* Item = Cast<ABaseItem>(EvidenceItem))
	{
		Item->OnEvidenceCollected.RemoveAll(this);
	}
	else if (AEvidenceActor* ItemAsEvidenceActor = Cast<AEvidenceActor>(EvidenceItem))
	{
		ItemAsEvidenceActor->OnActorPickedUp.RemoveAll(this);
	}
	
	GetCharacter()->OnCollectPendingEvidenceBegin_FromAnimNotify.RemoveAll(this);
	GetCharacter()->OnCollectPendingEvidenceEnd_FromAnimNotify.RemoveAll(this);
}

void UCollectEvidenceActivity::StopCollectEvidenceAnim(const float BlendOutTime)
{
	GetCharacter()->StopTPMontageFromTable("tp_swat_collect_evidence", BlendOutTime);
	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, TH_CollectEvidenceAnim);
}

// Copyright Void Interactive, 2023

#include "Info/Activities/PickUpCharacterActivity.h"

#include "Info/ReadyOrNotSignificanceManager.h"
#include "Info/SWATManager.h"

UPickUpCharacterActivity::UPickUpCharacterActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "PickUpTarget");
	MoveAcceptanceRadius = 30.0f;

	ActivityStateMachine->AddState("Move To").
						BindEventEnter(MAKE_DELEGATE_BINDING(this, &UPickUpCharacterActivity::EnterMoveToStage)).
						CreateTransition("Pick Up", MAKE_DELEGATE_BINDING(this, &UPickUpCharacterActivity::CanPickUpNow));
	
	ActivityStateMachine->AddState("Pick Up").
						BindEventEnter(MAKE_DELEGATE_BINDING(this, &UPickUpCharacterActivity::EnterPickUpStage)).
						BindEventTick(MAKE_DELEGATE_BINDING(this, &UPickUpCharacterActivity::TickPickUpStage)).
						CreateTransition("Transit", MAKE_DELEGATE_BINDING(this, &UPickUpCharacterActivity::CanTransitNow));
	
	ActivityStateMachine->AddState("Transit").
						BindEventEnter(MAKE_DELEGATE_BINDING(this, &UPickUpCharacterActivity::EnterTransitStage)).
						CreateTransition("Place Down", MAKE_DELEGATE_BINDING(this, &UPickUpCharacterActivity::CanPlaceDownNow));
	
	ActivityStateMachine->AddState("Place Down").
						BindEventEnter(MAKE_DELEGATE_BINDING(this, &UPickUpCharacterActivity::EnterPlaceDownStage)).
						//BindEventTick(MAKE_DELEGATE_BINDING(this, &UPickUpCharacterActivity::TickPlaceDownStage)).
						CreateTransition("Complete", MAKE_DELEGATE_BINDING(this, &UPickUpCharacterActivity::IsPlaceDownComplete));
	
	ActivityStateMachine->AddState("Complete").
						BindEventEnter(MAKE_DELEGATE_BINDING(this, &UPickUpCharacterActivity::EnterCompleteStage));
}

float UPickUpCharacterActivity::GetDestinationTolerance() const
{
	return 200.0f;
}

bool UPickUpCharacterActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (PickUpCharacter && !PickUpCharacter->IsBeingCarried())
	{
		FocalPoint = PickUpCharacter->GetActorLocation();
		return true;
	}
	
	return false;
}

bool UPickUpCharacterActivity::CanFinishActivity() const
{
	return false;
}

void UPickUpCharacterActivity::ResetData()
{
	Super::ResetData();
	
	PickUpCharacter = nullptr;
	FinalDestinationLocation = FVector::ZeroVector;
}

void UPickUpCharacterActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	if (!PickUpCharacter)
	{
		ACTIVITY_FAILED("No pick up character specified");
		return;
	}
	
	if (PickUpCharacter->IsActive() ||
		!(PickUpCharacter->IsArrested() || PickUpCharacter->IsArrestedAndDead() || PickUpCharacter->IsInRagdoll() || PickUpCharacter->IsDeadOrUnconscious() || PickUpCharacter->IsIncapacitated()))
	{
		ACTIVITY_FAILED("The pick up character is still active");
		return;
	}
	
	if (FinalDestinationLocation == FVector::ZeroVector) // failsafe
	{
		if (USWATManager* SwatManager = USWATManager::Get(this))
		{
			if (AReadyOrNotCharacter* SquadLeader = SwatManager->GetSquadLeader())
			{
				FinalDestinationLocation = SquadLeader->OriginalSpawnLocation;
			}
		}
	}

	if (FinalDestinationLocation == FVector::ZeroVector)
	{
		for (TActorIterator<APlayerStartPIE>It(GetWorld()); It; ++It)
		{
			FinalDestinationLocation = It->GetActorLocation();
			break;
		}
	}
	
	if (FinalDestinationLocation == FVector::ZeroVector)
	{
		ACTIVITY_FAILED("No valid final destination location specified");
		return;
	}
}

void UPickUpCharacterActivity::PerformActivity(float DeltaTime)
{
	Super::PerformActivity(DeltaTime);
	
	if (GetActiveStateID() == 0)
	{
		GetCharacter()->ReasonsToStandStill.Empty();
		Location = PickUpCharacter->GetNavAgentLocation();
		RequestMoveAsync();
	}

	if (GetActiveStateID() == 3)
	{
		Location = FVector::ZeroVector;
		GetCharacter()->Server_DropArrestedTarget_Implementation(PickUpCharacter);

		AbortMove();
	}
}

void UPickUpCharacterActivity::EnterMoveToStage()
{
	Location = PickUpCharacter->GetNavAgentLocation();
	RequestMoveAsync();
}

bool UPickUpCharacterActivity::CanPickUpNow() const
{
	return HasReachedLocation(GetDestinationTolerance());
}

void UPickUpCharacterActivity::EnterPickUpStage()
{
	PickUpCharacter->GetHealthComponent()->SetUnlimitedResource(true);
	GetCharacter()->Server_CarryArrestedTarget_Implementation(PickUpCharacter);
}

bool UPickUpCharacterActivity::CanTransitNow() const
{
	return PickUpCharacter->IsBeingCarried() && !UInteractionsData::IsPairedInteractionPlayingOn(OwningCharacter);
}

void UPickUpCharacterActivity::EnterTransitStage()
{
	SetLocation(FinalDestinationLocation, true);
}

bool UPickUpCharacterActivity::CanPlaceDownNow() const
{
	return HasReachedLocation(GetDestinationTolerance());
}

void UPickUpCharacterActivity::EnterPlaceDownStage()
{
	Location = FVector::ZeroVector;
	GetCharacter()->Server_DropArrestedTarget_Implementation(PickUpCharacter);
}

bool UPickUpCharacterActivity::IsPlaceDownComplete() const
{
	return !PickUpCharacter->IsBeingCarried() && !UInteractionsData::IsPairedInteractionPlayingOn(OwningCharacter);
}

void UPickUpCharacterActivity::EnterCompleteStage()
{
	PickUpCharacter->SetActorHiddenInGame(true);
	PickUpCharacter->ForEachAttachedActors([](AActor* Actor)
	{
		Actor->SetActorHiddenInGame(true);
		return true;
	});
	
	if (ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(PickUpCharacter))
	{
		AI->bDeactivated = true;
		UReadyOrNotSignificanceManager::ForceActorNotRelevant(AI);
	}

	GetCharacter()->ReasonsToStandStill.Remove("placing down");
	
	OwningController->FinishActivity(this, true, true);
}

void UPickUpCharacterActivity::TickPickUpStage(float DeltaTime, float Uptime)
{
	GetCharacter()->Server_CarryArrestedTarget_Implementation(PickUpCharacter);
}

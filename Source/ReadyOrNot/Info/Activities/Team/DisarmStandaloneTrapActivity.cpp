// Void Interactive, 2020

#include "DisarmStandaloneTrapActivity.h"

#include "Actors/Gameplay/TrapActor.h"
#include "Components/InteractableComponent.h"

#include "Info/SWATManager.h"

UDisarmStandaloneTrapActivity::UDisarmStandaloneTrapActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "DisarmTrap");
	bIsProgressActivity = true;

	ActivityStateMachine->AddState("Get In Position")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UDisarmStandaloneTrapActivity::EnterGetInPositionStage))
						.CreateTransition("Disarm", MAKE_DELEGATE_BINDING(this, &UDisarmStandaloneTrapActivity::CanPerformDisarm));
    
	ActivityStateMachine->AddState("Disarm")
						.BindEventEnter(MAKE_DELEGATE_BINDING(this, &UDisarmStandaloneTrapActivity::EnterDisarmStage));
}

void UDisarmStandaloneTrapActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	if (!TrapToDisarm)
	{
		ACTIVITY_FAILED("StartActivity | No valid trap specified");
		return;
	}

	GetCharacter()->bDisableTurnInPlace = true;
	
	StopDisarmTrapAnim();

	GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_ROGER_GENERIC);

	BindEvents();
}

void UDisarmStandaloneTrapActivity::ActivityOverriden(UBaseActivity* OverridingActivity)
{
	Super::ActivityOverriden(OverridingActivity);
	
	GetCharacter()->bDisableTurnInPlace = false;

	StopDisarmTrapAnim();

	UnbindEvents();
}

void UDisarmStandaloneTrapActivity::ResumeActivity()
{
	Super::ResumeActivity();

	GetCharacter()->bDisableTurnInPlace = true;
	
	StopDisarmTrapAnim();

	BindEvents();
}

void UDisarmStandaloneTrapActivity::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);
	
	GetCharacter()->bDisableTurnInPlace = false;

	UnbindEvents();

	if (!bSuccess)
		StopDisarmTrapAnim();
}

bool UDisarmStandaloneTrapActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (TrapToDisarm)
	{
		const FVector DirectionToLocation = (DisarmLocation - TrapToDisarm->GetActorLocation()).GetSafeNormal2D();
		const bool bRightOfTrap = FVector::DotProduct(DirectionToLocation, TrapToDisarm->GetActorRightVector()) > 0.0f;
		if (bRightOfTrap)
		{
			FocalPoint = TrapToDisarm->GetActorLocation() + TrapToDisarm->GetActorForwardVector() * 60.0f;
		}
		else
		{
			FocalPoint = TrapToDisarm->GetActorLocation() - TrapToDisarm->GetActorForwardVector() * 100.0f;
		}
		
		return true;
	}

	return Super::OverrideFocalPoint(FocalPoint);
}

bool UDisarmStandaloneTrapActivity::CanFinishActivity() const
{
	return false;
}

bool UDisarmStandaloneTrapActivity::CanOverrideActivity() const
{
	return false;
}

bool UDisarmStandaloneTrapActivity::IsTrapLive() const
{
	return TrapToDisarm && TrapToDisarm->TrapStatus != ETrapState::TS_Disabled;
}

void UDisarmStandaloneTrapActivity::EnterGetInPositionStage()
{
	if (!TrapToDisarm)
	{
		ACTIVITY_FAILED("EnterGetInPositionStage | No valid trap specified");
		return;
	}

	if (TrapToDisarm->TrapStatus != ETrapState::TS_Live)
	{
		GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);
		ACTIVITY_FAILED("EnterGetInPositionStage | Trap is not live", true);
		return;
	}

	const FVector DirectionToCharacter = (GetCharacter()->GetActorLocation() - TrapToDisarm->GetActorLocation()).GetSafeNormal2D();
	const bool bRightOfTrap = FVector::DotProduct(DirectionToCharacter, TrapToDisarm->GetActorRightVector()) > 0.0f;

	constexpr float Offset = 80.0f;
	DisarmLocation = TrapToDisarm->GetActorLocation() + TrapToDisarm->GetActorForwardVector() * 50.0f + TrapToDisarm->GetActorRightVector() * (bRightOfTrap ? Offset : -Offset);
	SetLocation(DisarmLocation, true);

	ProgressState = FText::FromStringTable("SwatCommandTable", "PreparingForDisarm");
}

bool UDisarmStandaloneTrapActivity::CanPerformDisarm() const
{
	if (!TrapToDisarm)
		return false;

	return HasReachedLocation(GetDestinationTolerance()) && TrapToDisarm->TrapStatus != ETrapState::TS_Disabled;
}

void UDisarmStandaloneTrapActivity::EnterDisarmStage()
{
	if (!TrapToDisarm)
	{
		ACTIVITY_FAILED("EnterDisarmStage | No valid trap specified");
		return;
	}

	if (TrapToDisarm->TrapStatus != ETrapState::TS_Live)
	{
		GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);
		ACTIVITY_FAILED("EnterDisarmStage | Trap is already disarmed", true);
		return;
	}

	Location = FVector::ZeroVector;
    AbortMove(true);

	PlayDisarmTrapAnim();

	ProgressState = FText::FromStringTable("SwatCommandTable", "CuttingWire");
}

void UDisarmStandaloneTrapActivity::OnTrapDisarmed()
{
	if (!TrapToDisarm)
	{
		ACTIVITY_FAILED("OnTrapDisarmed | No valid trap specified");
		return;
	}
	
	TrapToDisarm->OnTrapDisarmed(GetCharacter());

	GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_TRAP_DISARMED);

	float TimeRemaining = 0.0f;
	GetCharacter()->IsTableMontagePlayingWithTimeRemaining("tp_swt_disarmtrap", TimeRemaining);

	FTimerDelegate Delegate;
	Delegate.BindWeakLambda(this, [&]()
	{
		OwningController->FinishActivity(this, true, true);
	});
	
	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, Delegate, TimeRemaining-0.1f, false);
}

void UDisarmStandaloneTrapActivity::OnTrapTriggered(ATrapActor* Trap, AReadyOrNotCharacter* TriggeredBy)
{
	if (Trap == TrapToDisarm)
	{
		if (TriggeredBy)
			ACTIVITY_FAILED("OnTrapTriggered | Trap triggered by " + TriggeredBy->GetName(), true);
		else
			ACTIVITY_FAILED("OnTrapTriggered | Trap triggered", true);
	}
}

void UDisarmStandaloneTrapActivity::PlayDisarmTrapAnim(const FVector& InFocalPoint)
{
	GetCharacter()->PlayMontageFromTableWithFocalPoint("tp_swt_disarmtrap", InFocalPoint);
}

void UDisarmStandaloneTrapActivity::StopDisarmTrapAnim(float FadeOutTime)
{
	GetCharacter()->StopTPMontageFromTable("tp_swt_disarmtrap", FadeOutTime);
}

void UDisarmStandaloneTrapActivity::BindEvents()
{
	UnbindEvents();
	
	GetCharacter()->OnTrapDisarmEnd_FromAnimNotify.AddDynamic(this, &UDisarmStandaloneTrapActivity::OnTrapDisarmed);

	if (TrapToDisarm)
	{
		TrapToDisarm->TrapTriggered.AddDynamic(this, &UDisarmStandaloneTrapActivity::OnTrapTriggered);

		TrapToDisarm->InteractableComponent->ResetToOriginalLocation();
		
		// Disable player's from interacting with the trap if this AI is disarming it
		for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
		{
			UBpGameplayHelperLib::DisableInteractionForController(TrapToDisarm, *It);
		}
	}
}

void UDisarmStandaloneTrapActivity::UnbindEvents()
{
	GetCharacter()->OnTrapDisarmEnd_FromAnimNotify.RemoveAll(this);

	if (TrapToDisarm)
	{
		TrapToDisarm->TrapTriggered.RemoveAll(this);

		TrapToDisarm->InteractableComponent->ResetToOriginalLocation();
		
		for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
		{
			UBpGameplayHelperLib::EnableInteractionForController(TrapToDisarm, *It);
		}
	}
}

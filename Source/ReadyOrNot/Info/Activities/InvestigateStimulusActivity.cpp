// Copyright Void Interactive, 2022

#include "Info/Activities/InvestigateStimulusActivity.h"

#include "ReadyOrNotAIConfig.h"
#include "Characters/CyberneticController.h"
#include "Info/SuspectsAndCivilianManager.h"

UInvestigateStimulusActivity::UInvestigateStimulusActivity()
{
	bFinishActivityWhenOverriden = true;
	bAbortMoveWhenActivityFinished = true;
	bAbortActivityIfCannotReachLocation = true;
	bAllowPartialMove = true;

	MoveAcceptanceRadius = 50.0f;
}

void UInvestigateStimulusActivity::StartActivity(AAIController* Owner)
{
	bGlobalCooldownRandomRange = true;
	GlobalCooldownRange = AI_CONFIG_GET_VECTOR2D("SuspectGlobalInvestigationCooldown", FVector2D(5.0f, 7.0f));
	
	Super::StartActivity(Owner);

	if (!Stimulus.IsValid())
	{
		ACTIVITY_FAILED("Provided Stimulus is invalid");
		return;
	}

	if (Stimulus.StimulusLocation == FVector::ZeroVector ||
		!FAISystem::IsValidLocation(Stimulus.StimulusLocation))
	{
		ACTIVITY_FAILED("Provided Stimulus contains an invalid location");
		return;
	}

	const bool bShouldSprint = Stimulus.Strength > 1.0f;
		
	if (bShouldSprint)
	{
		GetCharacter()->ReasonsToSprint.AddUnique("investigating stimulus");
	}
	else
	{
		GetCharacter()->ReasonsToWalk.AddUnique("investigating stimulus");
	}

	CurrentInvestigationLocation = Stimulus.StimulusLocation;
	SetLocation(Stimulus.StimulusLocation);
	
	USuspectsAndCivilianManager::Get(this)->StartedInvestigating();
}

void UInvestigateStimulusActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);
	
	if (OwningController->GetTrackedTarget() ||
		GetCharacter()->IsStunned() ||
		StimulusSourceLOSTime > 1.0f)
	{
		OwningController->FinishActivity(this, true, true);
		return;
	}

	PlayAISpeech(VO_SUSPECTS_AND_CIVILIAN::SUSPICIOUS, true, 7.0f);
	
	if (StimulusLOSTime > 1.0f)
	{
		TimeInvestigatingWhenArrived += DeltaTime;
		if (TimeInvestigatingWhenArrived > 4.0f && !bInvestigatingSource)
		{
			// Enter second stage.. investigate the source (if there is one)
			if (Instigator)
			{
				bInvestigatingSource = true;
				
				CurrentInvestigationLocation = Instigator->GetActorLocation();
				SetLocation(CurrentInvestigationLocation, true);
			}
			else
			{
				OwningController->FinishActivity(this, true, true);
				return;
			}
		}

		if (bInvestigatingSource && Instigator)
		{
			if (!bEverHadLOSToStimulusSource && GetCharacter()->HasLineOfSightTo(FVector(Instigator->GetActorLocation().X, Instigator->GetActorLocation().Y, GetCharacter()->GetMesh()->GetBoneLocation("head").Z)))
			{
				bEverHadLOSToStimulusSource = true;
			}

			if (bEverHadLOSToStimulusSource)
			{
				OwningController->FinishActivity(this, true, true);
				return;
			}
		}
	}
	
	if (!bEverHadLOSToStimulus && GetCharacter()->HasLineOfSightTo(FVector(CurrentInvestigationLocation.X, CurrentInvestigationLocation.Y, GetCharacter()->GetMesh()->GetBoneLocation("head").Z)))
	{
		bEverHadLOSToStimulus = true;
	}

	if (bEverHadLOSToStimulus)
	{
		StimulusLOSTime += DeltaTime;
	}

	if (bEverHadLOSToStimulus && HasReachedLocation(300.0f))
	{
		Location = FVector::ZeroVector;
		AbortMove();
	}
}

void UInvestigateStimulusActivity::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	GetCharacter()->ReasonsToWalk.Remove("investigating stimulus");
	GetCharacter()->ReasonsToSprint.Remove("investigating stimulus");
}

#if !UE_BUILD_SHIPPING
void UInvestigateStimulusActivity::GatherDebugString(FString& OutString)
{
	OutString += AddDebugString("Investigation Location", CurrentInvestigationLocation.ToCompactString());
	OutString += AddDebugString("Stimulus LOS Time", FString::Printf(TEXT("%.2f"), StimulusLOSTime));
	OutString += AddDebugString("Time Investigating At Source", FString::Printf(TEXT("%.2f"), TimeInvestigatingWhenArrived));
}
#endif

bool UInvestigateStimulusActivity::CanFinishActivity() const
{
	return false; // Force finished
}

bool UInvestigateStimulusActivity::ShouldForceStrafe() const
{
	return true;
}

float UInvestigateStimulusActivity::GetDestinationTolerance() const
{
	return 100.0f;
}

bool UInvestigateStimulusActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (Stimulus.IsValid())
	{
		/*
		FVector OutDir;	
		float OutLength;
		GetCharacter()->GetVelocity().ToDirectionAndLength(OutDir, OutLength);

		FocalPoint = GetCharacter()->GetActorLocation() + OutDir * 500.0f;
		return true;
		*/
		if (FVector::Distance(Stimulus.StimulusLocation, GetCharacter()->GetActorLocation()) < 500.0f)
		{
			if (Stimulus.StimulusLocation != FVector::ZeroVector &&
				FAISystem::IsValidLocation(Stimulus.StimulusLocation))
			{
				FocalPoint = FVector(CurrentInvestigationLocation.X, CurrentInvestigationLocation.Y, GetCharacter()->GetActorLocation().Z + 45.0f);
				return true;
			}
		}
	}

	return false;
}

void UInvestigateStimulusActivity::ResetData()
{
	Super::ResetData();

	Stimulus = FAIStimulus();
	
	Instigator = nullptr;

	CurrentInvestigationLocation = FVector::ZeroVector;
	bEverHadLOSToStimulus = false;
	bEverHadLOSToStimulusSource = false;
	bInvestigatingSource = false;
	
	StimulusLOSTime = 0.0f;
	StimulusSourceLOSTime = 0.0f;
	TimeInvestigatingWhenArrived = 0.0f;

	bHasEverSpoken = true;
}

void UInvestigateStimulusActivity::OnAIHearingSense_Implementation(ACyberneticController* InSenseController, FAIStimulus InStimulus, AActor*& OutOverrideSensedActor)
{
	if (bHasEverSpoken)
		return;
	
	// ignore stimulus above or below us (floors of a building for example)
	const float ZHeightDifference = FMath::Abs(InStimulus.ReceiverLocation.Z - InStimulus.StimulusLocation.Z);

	if (ZHeightDifference > 100.0f)
	{
		return;
	}
	
	const AReadyOrNotCharacter* SensedCharacter = Cast<AReadyOrNotCharacter>(OutOverrideSensedActor);
	if (SensedCharacter)
	{
		if (SensedCharacter->IsOnSWATTeam())
		{
			GetCharacter()->PlayBarkOrStartConversation(VO_SUSPECTS_AND_CIVILIAN::SUSPICIOUS, true, 30.0f);
			bHasEverSpoken = true;
		}
	}
}

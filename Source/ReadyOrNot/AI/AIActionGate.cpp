// Copyright Void Interactive, 2022

#include "AI/AIActionGate.h"

#include "Characters/CyberneticController.h"

UWorld* UAIActionGate::GetWorld() const
{
	if (HasAllFlags(RF_ClassDefaultObject))
		return nullptr;

	return UBpGameplayHelperLib::GetWorldStatic();
}

bool UAGValidTarget::CanOpen_Implementation(const FAIActionDecisionContext& Context) const
{
	AReadyOrNotCharacter* TrackedTarget = Context.Controller->GetTargetingComp()->GetTrackedTarget();
	if (!TrackedTarget)
	{
		if (bAllowLastTrackedTarget)
			TrackedTarget = Context.Controller->GetTargetingComp()->GetLastTrackedTarget();
	}

	if (!TrackedTarget)
		return false;
	
	return  (bAllowEnemy && Context.Controller->GetTargetingComp()->KnownEnemies.Contains(TrackedTarget)) ||
			(bAllowFriendly && Context.Controller->GetTargetingComp()->KnownFriendlies.Contains(TrackedTarget)) ||
			(bAllowNeutral && Context.Controller->GetTargetingComp()->KnownNeutrals.Contains(TrackedTarget));
}

bool UAGAnyNearbyAI::CanOpen_Implementation(const FAIActionDecisionContext& Context) const
{
	for (ACyberneticCharacter* AI : Context.World->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
	{
		const bool bIsSwatTeamType =  TeamType == ETeamType::TT_SQUAD || TeamType == ETeamType::TT_SERT_RED || TeamType == ETeamType::TT_SERT_BLUE;
		const bool bSameTeam = AI->GetTeam() == TeamType || (bIsSwatTeamType && AI->IsOnSWATTeam());
		if (AI != Context.Controller->GetCharacter() && bSameTeam)
		{
			if (FVector::Distance(AI->GetActorLocation(), Context.Controller->GetCharacter()->GetActorLocation()) < SearchRange)
			{
				if (!bMustBeVisible)
				{
					return true;
				}

				const FVector StartTrace = Context.Controller->GetCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
				const FVector EndTrace = AI->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
				if (!GetWorld()->LineTraceTestByChannel(StartTrace, EndTrace, ECC_Visibility, UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(Context.Controller->GetCharacter(), AI)))
				{
					return true;
				}
			}
		}
	}
	
	return false;
}

bool UAGNumActiveAI::CanOpen_Implementation(const FAIActionDecisionContext& Context) const
{
	uint8 NumActive = 0;
	
	if (Team == ETeamType::TT_SUSPECT)
		NumActive = Context.World->GetGameState<AReadyOrNotGameState>()->NumSuspectsActive;
	else if (Team == ETeamType::TT_CIVILIAN)
		NumActive = Context.World->GetGameState<AReadyOrNotGameState>()->NumCiviliansActive;
	else if (Team == ETeamType::TT_SQUAD || Team == ETeamType::TT_SERT_RED || Team == ETeamType::TT_SERT_BLUE)
		NumActive = Context.World->GetGameState<AReadyOrNotGameState>()->NumSwatActive;

	switch (ComparisonType)
	{
		case EComparisonOp::Equals:					return NumActive == NumAI;
		case EComparisonOp::NotEquals:				return NumActive != NumAI;
		case EComparisonOp::GreaterThan:			return NumActive > NumAI;
		case EComparisonOp::GreaterThanEquals:		return NumActive >= NumAI;
		case EComparisonOp::LessThan:				return NumActive < NumAI;
		case EComparisonOp::LessThanEquals:			return NumActive <= NumAI;
		default: return false;
	}
}

bool UAG_HasEquippedItem::CanOpen_Implementation(const FAIActionDecisionContext& Context) const
{
	if (!Context.Controller->GetCharacter())
		return false;
	return Context.Controller->GetCharacter()->GetEquippedItem() != nullptr;
}

bool UAG_HasEquippedWeapon::CanOpen_Implementation(const FAIActionDecisionContext& Context) const
{
	if (!Context.Controller->GetCharacter())
		return false;
	return Context.Controller->GetCharacter()->GetEquippedWeapon() != nullptr;
}

bool UAG_IsSurrendered::CanOpen_Implementation(const FAIActionDecisionContext& Context) const
{
	if (!Context.Controller->GetCharacter())
		return false;
	return Context.Controller->GetCharacter()->bSurrendered && Time <= Context.Controller->GetCharacter()->TimeSurrendered;
}

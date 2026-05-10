// Copyright Void Interactive, 2022

#include "AIActionConsideration.h"

#include "Characters/CyberneticController.h"
#include "Characters/AI/TrailerSWATCharacter.h"
#include "Components/MoraleComponent.h"
#include "lib/ReadyOrNotMathLibrary.h"

#if WITH_EDITOR
void UAIActionConsideration::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	UpdateCurve();
}
#endif

#if WITH_EDITORONLY_DATA
void UAIActionConsideration::Serialize(FArchive& Ar)
{
	UpdateCurve();
	
	UObject::Serialize(Ar);
}
#endif

UWorld* UAIActionConsideration::GetWorld() const
{
	if (HasAllFlags(RF_ClassDefaultObject))
		return nullptr;

	return UBpGameplayHelperLib::GetWorldStatic();
}

void UAIActionConsideration::UpdateCurve()
{
	if (bManualCurveEdit)
		return;

	Curve.EditorCurveData.Reset();
	
	for (uint8 i = 0; i < SubStep+1; ++i)
	{
		const float Alpha = (float)i/SubStep;
		
		float EaseAlpha;
		if (bCustomCurveFunction)
		{
			EaseAlpha = FMath::Pow(CalculateCurve(Alpha), Exponent);
		}
		else
		{
			EaseAlpha = UReadyOrNotMathLibrary::EaseAlpha(Alpha, CurveType, Exponent, 1);
		}
		
		float Time = Alpha + OffsetX;
		if (bClamp)
		{
			if (Time < MinRange || Time > MaxRange)
				continue;
		}
		
		if (bInverseX)
			Time = 1.0f-Time;

		float Value = EaseAlpha + OffsetY;
		if (bClamp)
		{
			if (Value < MinRange || Value > MaxRange)
				continue;
		}
		
		if (bInverseY)
			Value = 1.0f-Value;
			
		Curve.EditorCurveData.AddKey(Time, Value);
	}
	
	for (FRichCurveKey& Key : Curve.EditorCurveData.Keys)
	{
		Key.InterpMode = InterpMode;
		Key.TangentMode = TangentMode;
	}
	
	Curve.EditorCurveData.AutoSetTangents();
}

float UAIActionConsideration::EvaluateResponseCurve(const float Score)
{
	UpdateCurve();

	Curve.GetRichCurve()->DefaultValue = 0.0f;
	return Curve.GetRichCurveConst()->Eval(Score, 0.0f);
}

float UACNumberOfNearbyAI::Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const
{
	uint8 NumNearby = 0;
	for (ACyberneticCharacter* AI : Context.World->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
	{
		if (Cast<ATrailerSWATCharacter>(AI))
			continue;

		const bool bIsSwatTeamType =  TeamType == ETeamType::TT_SQUAD || TeamType == ETeamType::TT_SERT_RED || TeamType == ETeamType::TT_SERT_BLUE;
		const bool bSameTeam = AI->GetTeam() == TeamType || (bIsSwatTeamType && AI->IsOnSWATTeam());
		if (AI != Context.Controller->GetCharacter() && bSameTeam)
		{
			if (FVector::Distance(AI->GetActorLocation(), Context.Controller->GetCharacter()->GetActorLocation()) < SearchRange)
			{
				if (!bMustBeVisible)
				{
					NumNearby++;
					continue;
				}

				const FVector StartTrace = Context.Controller->GetCharacter()->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
				const FVector EndTrace = AI->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
				if (!GetWorld()->LineTraceTestByChannel(StartTrace, EndTrace, ECC_Visibility, UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(Context.Controller->GetCharacter(), AI)))
				{
					NumNearby++;
				}
			}
		}
	}
	
	bSuccess = true;
	return FMath::GetMappedRangeValueClamped(FVector2D(MinRange, MaxRange), FVector2D(0.0f, 1.0f), NumNearby);
}

float UAC_Health::Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const
{
	bSuccess = true;
	return Context.Controller->GetCharacter()->GetCurrentHealth() / Context.Controller->GetCharacter()->GetMaxHealth();
}

float UAC_TargetHealth::Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const
{
	if (const AReadyOrNotCharacter* Target = Context.Controller->GetTrackedTarget())
	{
		bSuccess = true;
		return Target->GetCurrentHealth() / Target->GetMaxHealth();
	}

	bSuccess = false;
	return 0.0f;
}

float UAC_IncapacitationHealth::Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const
{
	bSuccess = true;
	const float M = Context.Controller->GetCharacter()->GetHealthComponent()->GetIncapacitationHealthMultiplier();
	return (Context.Controller->GetCharacter()->GetCurrentHealth() / M) / (Context.Controller->GetCharacter()->GetMaxHealth() / M);
}

float UAC_Ammo::Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const
{
	if (const ABaseMagazineWeapon* Weapon = Context.Controller->GetCharacter()->GetEquippedWeapon())
	{
		bSuccess = true;
		return Weapon->GetCurrentAmmoPercentage();
	}

	bSuccess = false;
	return 0.0f;
}

float UAC_Armor::Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const
{
	if (const ABaseArmour* Armor = Context.Controller->GetCharacter()->GetArmour())
	{
		bSuccess = true;
		return Armor->GetDurabilityPercentage();
	}

	bSuccess = false;
	return 0.0f;
}

float UAC_Morale::Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const
{
	bSuccess = true;
	return Context.Controller->GetMoraleComp()->GetMorale();
}

float UAC_Stress::Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const
{
	bSuccess = true;
	return Context.Controller->GetCharacter()->Stress;
}


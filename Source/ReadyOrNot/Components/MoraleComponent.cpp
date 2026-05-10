// Void Interactive, 2020

#include "MoraleComponent.h"

#include "Characters/CyberneticController.h"
#include "Characters/AI/SWATCharacter.h"
#include "Info/SuspectsAndCivilianManager.h"
#include "lib/ReadyOrNotMathLibrary.h"

TAutoConsoleVariable<int32> CVarMoraleDebug(TEXT("Morale.Debug"), 0, TEXT("0 = Don't draw morale debug. 1 = Draw morale debug"));
TAutoConsoleVariable<int32> CVarMoraleRadialDamageDebug(TEXT("Morale.RadialDamageDebug"), 0, TEXT("0 = Don't draw morale debug. 1 = Draw morale debug"));

UMoraleComponent::UMoraleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.033f;

	bCanEverAffectNavigation = false;

	ResourceName = "Morale";
	Resource = 0.0f;
	MaxResource = 1.0f;
	MaxResourceLimit = 10.0f;
	OriginalMaxResource = MaxResource;
	LowResource = 0.25f;
	PreviousResource = 0.0f;
}

void UMoraleComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (auto& It : MoraleDamageHistory)
	{
		It.Value.TimeSinceChange += DeltaTime;
	}
	
	for (auto& It : MoraleGainHistory)
	{
		It.Value.TimeSinceChange += DeltaTime;
	}

	if (OwnerCharacter)
	{
		#if !UE_BUILD_SHIPPING
		if (CVarMoraleDebug.GetValueOnAnyThread() > 0)
		{
			const float ComplianceMorale = OwnerCharacter->GetVisibleSWATPercentage();
			const float TargetMorale = Resource;

			const FString DebugString = "Morale: " + FString::SanitizeFloat(TargetMorale) + LINE_TERMINATOR +
										(!OwnerCharacter->IsSurrendered() ? "Compliance Morale: " + FString::SanitizeFloat(ComplianceMorale) : "");

			DrawDebugString(GetWorld(), OwnerCharacter->GetMesh()->GetBoneLocation("neck_1"), DebugString, nullptr, FColor::White, DeltaTime + 0.001f, true);
		}
		#endif
	}
}

void UMoraleComponent::IncreaseMoraleOnCharacter(ACyberneticCharacter* Character, const float MoraleValue, const FName Reason)
{
	if (!Character)
		return;

	if (Character->GetCyberneticsController())
	{
		if (UMoraleComponent* MoraleComponent = Character->GetCyberneticsController()->GetMoraleComp())
		{
			FMoraleChangeInfo Info;
			Info.Delta = MoraleValue;
			Info.TimeSinceChange = 0.0f;
			MoraleComponent->MoraleGainHistory.Add(Reason, Info);
			
			MoraleComponent->IncreaseResource(FMath::Abs(MoraleValue));

			#if WITH_EDITOR
			//ULog::Info("Morale Increased by " + FString::SanitizeFloat(FMath::Abs(MoraleValue)) + " for " + Character->GetName() + " | Morale: " + FString::SanitizeFloat(MoraleComponent->Morale));
			#endif
		}
	}
}

void UMoraleComponent::LowerMoraleOnCharacter(ACyberneticCharacter* Character, const float MoraleValue, const FName Reason)
{
	if (!Character)
		return;

	if (Character->GetCyberneticsController())
	{
		if (UMoraleComponent* MoraleComponent = Character->GetCyberneticsController()->GetMoraleComp())
		{
			FMoraleChangeInfo Info;
			Info.Delta = MoraleValue;
			Info.TimeSinceChange = 0.0f;
			MoraleComponent->MoraleDamageHistory.Add(Reason, Info);
			
			MoraleComponent->DecreaseResource(FMath::Abs(MoraleValue));

			#if WITH_EDITOR
			//ULog::Info("Morale Lowered by " + FString::SanitizeFloat(MoraleValue) + " for " + Character->GetName() + " | Morale: " + FString::SanitizeFloat(MoraleComponent->Morale));
			#endif
		}
	}
}

void UMoraleComponent::ResetMoraleOnCharacter(ACyberneticCharacter* Character)
{
	if (!Character)
		return;

	if (Character->GetCyberneticsController())
	{
		if (UMoraleComponent* MoraleComponent = Character->GetCyberneticsController()->GetMoraleComp())
		{
			FMoraleChangeInfo Info;
			Info.Delta = MoraleComponent->StartingMorale - MoraleComponent->Resource;
			Info.TimeSinceChange = 0.0f;
			MoraleComponent->MoraleGainHistory.Add("Reset", Info);
			
			MoraleComponent->SetResource(MoraleComponent->StartingMorale);
		}
	}
}

void UMoraleComponent::ApplyRadialMoraleDamage(const UObject* WorldContextObject, FVector Location, float Damage, float Radius, FMoraleDamageTraceParameters LOSParameters, TArray<ETeamType> Teams, FName Reason)
{
	if (Location == FVector::ZeroVector || Damage == 0.0f)
		return;
	
	if (const USuspectsAndCivilianManager* SusAndCivManager = USuspectsAndCivilianManager::Get(WorldContextObject->GetWorld()))
	{
		Damage = FMath::Abs(Damage);
		
		if (Radius == 0.0f)
			Radius = 1.0f;
		
		const TArray<ACyberneticCharacter*>& AllAI = SusAndCivManager->GetAllSuspectsAndCivilians();

		for (const ACyberneticCharacter* AI : AllAI)
		{
			if (AI->IsActive())
			{
				UMoraleComponent* MoraleComponent = AI->GetCyberneticsController()->GetMoraleComp();
				if (IsValid(MoraleComponent))
				{
					if (!Teams.Contains(AI->GetTeam()))
						continue;
					
					const float Dist = FVector::Distance(AI->GetActorLocation(), Location);
					if (Dist > Radius)
						continue;

					if (LOSParameters.bEnable)
					{
						FHitResult Hit;
						
						FCollisionQueryParams CollisionQueryParams = AI->GetCollisionQueryParameters();
						CollisionQueryParams.AddIgnoredActors(LOSParameters.IgnoredActors);
						CollisionQueryParams.AddIgnoredComponents(LOSParameters.IgnoredComponents);
						
						AI->GetWorld()->LineTraceSingleByChannel(Hit, Location, AI->GetActorLocation(), LOSParameters.TraceChannel, CollisionQueryParams);
						
						if (Hit.bBlockingHit)
							continue;
					}

					FMoraleChangeInfo Info;
					Info.Delta = Damage;
					Info.TimeSinceChange = 0.0f;
					MoraleComponent->MoraleDamageHistory.Add(Reason, Info);
					
					MoraleComponent->DecreaseResource(Damage);
					
					#if !UE_BUILD_SHIPPING
					if (CVarMoraleRadialDamageDebug.GetValueOnAnyThread() > 0)
					{
						DrawDebugSphere(AI->GetWorld(), Location, Radius, 32, FColor::Red, false, 5.0f);
						DrawDebugString(AI->GetWorld(), AI->GetMesh()->GetBoneLocation("head"), "Morale Damage Applied: " + FString::Printf(TEXT("%.2f"), Damage), nullptr, FColor::Red, 5.0f, true);
					}
					#endif
				}
			}
		}
	}
}

void UMoraleComponent::ApplyRadialMoraleDamageWithFalloff(const UObject* WorldContextObject, FVector Location, float Damage, float InnerRadius, float OuterRadius, FMoraleDamageTraceParameters LOSParameters, TArray<ETeamType> Teams, EEasingFunc::Type FalloffCurve, FName Reason)
{
	if (Location == FVector::ZeroVector || Damage == 0.0f)
		return;
	
	if (const USuspectsAndCivilianManager* SusAndCivManager = USuspectsAndCivilianManager::Get(WorldContextObject->GetWorld()))
	{
		Damage = FMath::Abs(Damage);

		float BaseDamage = Damage;

		if (InnerRadius == 0.0f)
			InnerRadius = 1.0f;
		
		if (OuterRadius == 0.0f)
			OuterRadius = 1.0f;

		if (OuterRadius < InnerRadius)
			OuterRadius = InnerRadius;

		if (InnerRadius > OuterRadius)
			InnerRadius = OuterRadius;
		
		const TArray<ACyberneticCharacter*>& AllAI = SusAndCivManager->GetAllSuspectsAndCivilians();

		for (const ACyberneticCharacter* AI : AllAI)
		{
			if (AI && AI->IsActive())
			{
				UMoraleComponent* MoraleComponent = AI->GetCyberneticsController()->GetMoraleComp();
				if (IsValid(MoraleComponent))
				{
					if (!Teams.Contains(AI->GetTeam()))
						continue;
					
					// Make sure we're on the same floor level
					{
						const float MaxZ = FMath::Max(Location.Z, AI->GetActorLocation().Z);
						const float MinZ = FMath::Min(Location.Z, AI->GetActorLocation().Z);
						
						const float ZHeightDifference = MaxZ - MinZ;

						if (ZHeightDifference > 150.0f)
						{
							continue;
						}
					}

					const float Dist = FVector::Distance(AI->GetActorLocation(), Location);
					if (Dist > OuterRadius)
						continue;

					if (LOSParameters.bEnable)
					{
						FHitResult Hit;
						
						FCollisionQueryParams CollisionQueryParams = AI->GetCollisionQueryParameters();
						CollisionQueryParams.AddIgnoredActors(LOSParameters.IgnoredActors);
						CollisionQueryParams.AddIgnoredComponents(LOSParameters.IgnoredComponents);
						
						AI->GetWorld()->LineTraceSingleByChannel(Hit, Location, AI->GetActorLocation(), LOSParameters.TraceChannel, CollisionQueryParams);
						
						if (Hit.bBlockingHit)
							continue;
					}

					const float Alpha = FMath::GetMappedRangeValueClamped(FVector2D(InnerRadius, OuterRadius), FVector2D(1.0f, 0.0f), Dist);
					const float EaseAlpha = UReadyOrNotMathLibrary::EaseAlpha(Alpha, FalloffCurve, 1.0f, 1);
					Damage *= EaseAlpha;

					FMoraleChangeInfo Info;
					Info.Delta = Damage;
					Info.TimeSinceChange = 0.0f;
					MoraleComponent->MoraleDamageHistory.Add(Reason, Info);
					
					MoraleComponent->DecreaseResource(Damage);

					#if !UE_BUILD_SHIPPING
					if (CVarMoraleRadialDamageDebug.GetValueOnAnyThread() > 0)
					{
						DrawDebugSphere(AI->GetWorld(), Location, InnerRadius, 32, FColor::Red, false, 5.0f);
						DrawDebugSphere(AI->GetWorld(), Location, OuterRadius, 32, FColor::Yellow, false, 5.0f);
						DrawDebugString(AI->GetWorld(), Location, "Morale Base Damage: " + FString::Printf(TEXT("%.3f"), BaseDamage), nullptr, FColor::White, 5.0f, true);
						DrawDebugString(AI->GetWorld(), AI->GetMesh()->GetBoneLocation("head"), "Morale Damage Applied: " + FString::Printf(TEXT("%.3f"), Damage), nullptr, FColor::MakeRedToGreenColorFromScalar(1.0f-Alpha), 5.0f, true);
					}
					#endif
				}
			}
		}
	}
}

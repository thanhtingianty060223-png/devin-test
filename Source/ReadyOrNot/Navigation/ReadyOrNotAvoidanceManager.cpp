// Void Interactive, 2020

#include "Navigation/ReadyOrNotAvoidanceManager.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"
#include "Info/ReadyOrNotSignificanceManager.h"
#include "Info/Activities/BaseCombatActivity.h"
#include "Info/Activities/MoveToActivity.h"
#include "Info/Activities/ScanDoorActivity.h"
#include "Info/Activities/WorldBuildingActivity.h"
#include "Info/Activities/Team/ArrestTargetActivity.h"
#include "Info/Activities/Team/DoorBreachActivity.h"
#include "Info/Activities/Team/DoorInteractionActivity.h"
#include "Info/Activities/Team/TeamStackUpActivity.h"
#include "NavigationSystem/Public/NavigationSystem.h"

DECLARE_CYCLE_STAT(TEXT("Ready Or Not Avoidance Manager ~ Tick"), STAT_Tick, STATGROUP_ReadyOrNotAvoidanceManager);

TAutoConsoleVariable<int32> CVarDisableAvoidanceManager(TEXT("Avoidance.Enabled"), 1, TEXT(""));

UReadyOrNotAvoidanceManager::UReadyOrNotAvoidanceManager()
{
	TickInterval = 0.05f;
}

void UReadyOrNotAvoidanceManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TimeSinceLastTick = 0.0f;
	AvoidanceIdx = 0;
}

TStatId UReadyOrNotAvoidanceManager::GetStatId() const
{
	return GetStatID();
}

void UReadyOrNotAvoidanceManager::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	SCOPE_CYCLE_COUNTER(STAT_Tick);

	if (CVarDisableAvoidanceManager.GetValueOnAnyThread() == 0)
		return;

	if (!GetWorld()->GetGameState<AReadyOrNotGameState>())
		return;

	TimeSinceLastTick += DeltaTime;
	if (TimeSinceLastTick > TickInterval)
	{
		TimeSinceLastTick = 0.0f;
		
		//AvoidanceIdx++;

		const TArray<ACyberneticCharacter*>& AllAI = GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters;

		for (ACyberneticCharacter* Character : AllAI)
		//if (SpawnedAI.IsValidIndex(AvoidanceIdx))
		{
			if (!UReadyOrNotSignificanceManager::IsActorRelevant(Character))
				continue;
			
			//DrawDebugCapsule(GetWorld(), Character->GetActorLocation(), Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), Character->GetCapsuleComponent()->GetScaledCapsuleRadius() * 3.0f, Character->GetCapsuleComponent()->GetComponentQuat(), FColor::Blue, false, 0.033f, 0, 1.0f);

			if (!Character->IsActive())
			{
				Character->AvoidingCharacter = nullptr;
				continue;
			}
			
			if (Character->GetCyberneticsController()->GetCombatActivity()->GetCombatMoveActivity())
			{
				Character->AvoidingCharacter = nullptr;
				continue;
			}

			if (const UBaseActivity* Activity = Character->GetCyberneticsController()->GetCurrentActivity())
			{
				if (Cast<UArrestTargetActivity>(Activity))
				{
					Character->AvoidingCharacter = nullptr;
					continue;
				}

				if (Cast<UDoorInteractionActivity>(Activity) ||
					Cast<UDoorBreachActivity>(Activity) ||
					Cast<UTeamStackUpActivity>(Activity) ||
					Cast<UScanDoorActivity>(Activity))
				{
					Character->AvoidingCharacter = nullptr;
					continue;
				}
				
				if (!Activity->HasReachedLocation(100.0f))
				{
					Character->AvoidingCharacter = nullptr;
					continue;
				}
			}

			if (ACyberneticCharacter* OtherAI = Character->AvoidingCharacter)
			{
				if (OtherAI->GetCyberneticsController())
				{
					const float CapsuleRadius = OtherAI->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
					const float Dist = FVector::Distance(OtherAI->GetActorLocation(), Character->GetActorLocation());
					const float Tolerance = 100.0f - (CapsuleRadius*2.0f);
					//LOG_NUMBER(Dist);
				
					if (Dist < Tolerance)
					{
						FVector Direction = (OtherAI->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal();
						//if (OtherAI->GetVelocity().Size() > 50.0f)
							//Direction = FVector::CrossProduct(OtherAI->GetVelocity(), FVector::UpVector);
						Character->GetCyberneticsController()->GiveMoveTo(OtherAI->GetActorLocation() + -Direction * 100.0f);
						OtherAI->GetCyberneticsController()->GiveMoveTo(OtherAI->GetActorLocation() + Direction * 100.0f);
						//Character->GetCyberneticsController()->AbortMove(true);
						//Character->GetCyberneticsController()->RemoveActivitiesOfType(UMoveActivity::StaticClass());
						//OtherAI->GetCyberneticsController()->RemoveActivitiesOfType(UMoveActivity::StaticClass());
						//OtherAI->GetCyberneticsController()->AbortMove(true);
						ULog::Info(Character->GetName() + " Avoiding " + OtherAI->GetName());
						//DrawDebugPoint(GetWorld(), OtherAI->GetActorLocation(), 50.0f, FColor::Purple, false, 1.0f);
						//DrawDebugDirectionalArrow(GetWorld(), OtherAI->GetActorLocation(), OtherAI->GetActorLocation() + Direction * 100.0f, 5.0f, FColor::Red, false, 1.0f);
					}
					else
					{
						Character->AvoidingCharacter = nullptr;
					}
				}
				else
				{
					Character->AvoidingCharacter = nullptr;
				}

				continue;
			}
			
			for (ACyberneticCharacter* OtherAI : AllAI)
			{
				if (!UReadyOrNotSignificanceManager::IsActorRelevant(Character))
					continue;
				
				if (OtherAI == Character)
					continue;

				if (OtherAI->AvoidingCharacter)
					continue;
				
				if (!OtherAI->IsActive())
					continue;

				if (OtherAI->GetCyberneticsController()->IsMoving())
					continue;

				if (OtherAI->GetCyberneticsController()->GetCombatActivity()->GetCombatMoveActivity())
					continue;

				if (const UBaseActivity* Activity = OtherAI->GetCyberneticsController()->GetCurrentActivity())
				{
					if (Cast<UArrestTargetActivity>(Activity))
						continue;

					if (Cast<UDoorInteractionActivity>(Activity) ||
						Cast<UDoorBreachActivity>(Activity) ||
						Cast<UTeamStackUpActivity>(Activity) ||
						Cast<UScanDoorActivity>(Activity))
						continue;
					
					if (!Activity->HasReachedLocation(100.0f))
						continue;
				}

				const float CapsuleRadius = OtherAI->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
				const float Dist = FVector::Distance(OtherAI->GetActorLocation(), Character->GetActorLocation());
				const float Tolerance = 80.0f - (CapsuleRadius*2.0f);
				
				if (Dist < Tolerance)
				{
					if (OtherAI->HasLineOfSightToCharacter(Character))
					{
						if (UBaseActivity* Activity = OtherAI->GetCyberneticsController()->GetCurrentActivity())
						{
							if (Activity->HasReachedLocation(Activity->GetDestinationTolerance()))
							{
								OtherAI->NoPathCooldown = 1.0f;
							}
						}
						
						OtherAI->AvoidingCharacter = Character;
						Character->AvoidingCharacter = OtherAI;
						
						Character->GetCyberneticsController()->AbortMove();
						OtherAI->GetCyberneticsController()->AbortMove();
					}
				}
			}
			
			ACyberneticCharacter* OverlappingAI = nullptr;

			if (IsOverlapping(Character, OverlappingAI) && CanOverrideLocation(Character))
			{
				V_LOGM(LogReadyOrNot, "%s AVOIDING %s", *Character->GetName(), *OverlappingAI->GetName());
				
				FVector Location = GetBestFreeLocation(Character, OverlappingAI);
				OverrideActivityLocation(Character, Location);
				
				if (Location == FVector::ZeroVector)
				{
					Location = GetBestFreeLocation(OverlappingAI, Character);
					OverrideActivityLocation(OverlappingAI, Location);
				}
			}
		}
		//else
		//{
		//	AvoidanceIdx = 0;
		//}
	}
}

bool UReadyOrNotAvoidanceManager::IsOverlapping(ACyberneticCharacter* AI, ACyberneticCharacter*& OverlappingAI) const
{
	const TArray<ACyberneticCharacter*>& AllAI = GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters;
	
	for (ACyberneticCharacter* OtherAI : AllAI)
	{
		if (OtherAI == AI)
			continue;

		if (!OtherAI->IsActive())
			continue;

		if (OtherAI->GetCyberneticsController()->IsMoving())
			continue;

		if (const UBaseActivity* Activity = OtherAI->GetCyberneticsController()->GetCurrentActivity())
		{
			if (!Activity->HasReachedLocation(100.0f))
				continue;
		}

		const float Dist = FVector::Distance(OtherAI->GetActorLocation(), AI->GetActorLocation());
		
		if (Dist < OtherAI->GetCapsuleComponent()->GetScaledCapsuleRadius() * 2.5f)
		{
			OverlappingAI = OtherAI;
			return true;
		}
	}
	
	return false;
}

bool UReadyOrNotAvoidanceManager::CanOverrideLocation(ACyberneticCharacter* AI) const
{	
	if (UBaseActivity* BaseActivity = AI->GetCyberneticsController()->GetCurrentActivity())
	{
		if (Cast<UWorldBuildingActivity>(BaseActivity))
			return false;
		
		if (Cast<UArrestTargetActivity>(BaseActivity))
			return false;

		if (Cast<UDoorInteractionActivity>(BaseActivity) || Cast<UTeamStackUpActivity>(BaseActivity) ||
			Cast<UScanDoorActivity>(BaseActivity))
			return false;
		
		if (!BaseActivity->HasReachedLocation(100.0f))
			return false;
	}
	
	if (!AI->GetCyberneticsController()->IsMoving())
		return true;

	return false;
}

void UReadyOrNotAvoidanceManager::OverrideActivityLocation(ACyberneticCharacter* AI, FVector Location)
{
	if (Location == FVector::ZeroVector)
		return;

	AI->AvoidingCharacter = nullptr;
	
	if (UBaseActivity* BaseActivity = AI->GetCyberneticsController()->GetCurrentActivity())
	{
		if (!BaseActivity->IsNoMoveActivity() && BaseActivity->HasReachedLocation(200.0f))
		{
			BaseActivity->SetLocation(Location, true);
		}
		else
		{
			UMoveToActivity* MoveActivity = AI->GetCyberneticsController()->GetAvoidanceMoveToActivity();
			MoveActivity->SetLocation(Location);
			AI->GetCyberneticsController()->AddActivity(MoveActivity, true);
		}
	}
	else
	{
		UMoveToActivity* MoveActivity = AI->GetCyberneticsController()->GetAvoidanceMoveToActivity();
		MoveActivity->SetLocation(Location);
		AI->GetCyberneticsController()->AddActivity(MoveActivity, true);
	}
}

FVector UReadyOrNotAvoidanceManager::GetBestFreeLocation(ACyberneticCharacter* AI, ACyberneticCharacter* OverlappingAI) const
{
	if (UTeamBaseActivity* TeamActivity = AI->GetCyberneticsController()->GetCurrentActivity<UTeamBaseActivity>())
	{
		if (TeamActivity->OverrideAvoidanceLocation())
		{
			FVector Location = TeamActivity->GetBestAvoidanceLocation(OverlappingAI);
			
			DrawDebugSphere(GetWorld(), Location, 10.0f, 4, FColor::Yellow, false, 1.0f, 0);
			
			if (TeamActivity->GetLocation() != FVector::ZeroVector && TeamActivity->GetLocation() == Location && TeamActivity->HasReachedLocation())
				return FVector::ZeroVector;

			return Location;
		}
	}
	
	FCollisionObjectQueryParams CollisionObjectQueryParams;
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOOR);
	
	const FVector Start = AI->GetMesh()->GetBoneLocation("spine_1");

	constexpr float Angle = 45.0f;
	constexpr int32 NumberOfTraces = 360/Angle;
	for (int32 i = 0; i < NumberOfTraces; i++)
	{
		FHitResult LOSTest;

		FRotator TestDirection = FRotator(0.0f, i * Angle + AI->GetActorRotation().Yaw + -180.0f, 0.0f);			
		const FVector V1 = UKismetMathLibrary::FindLookAtRotation(AI->GetActorLocation(), OverlappingAI->GetActorLocation()).Vector();;
		const FVector V2 = UKismetMathLibrary::FindLookAtRotation(AI->GetActorLocation(),  AI->GetActorLocation() + TestDirection.Vector() * 150.0f).Vector();

		float DotProduct = FVector2D::DotProduct(FVector2D(V1.X, V1.Y), FVector2D(V2.X, V2.Y));
		if (DotProduct > 0.0f)
			continue;


		// try move left, then forward, then right, then back
		GetWorld()->LineTraceSingleByObjectType(LOSTest, Start,  Start + TestDirection.Vector() * 150.0f, CollisionObjectQueryParams, AI->GetCollisionQueryParameters());
		DrawDebugLine(GetWorld(), LOSTest.TraceStart, LOSTest.TraceEnd, LOSTest.bBlockingHit ? FColor::Red : FColor::Green, false, 10.0f, 0, 1.0f);
		if (!LOSTest.bBlockingHit)
		{
			FHitResult Hit;
			FVector BoxStart = Start +  TestDirection.Vector() * 150.0f;
			BoxStart.Z += 30.0f;
			UKismetSystemLibrary::BoxTraceSingleForObjects(this, BoxStart, BoxStart , FVector(28.0f, 28.0f, 28.0f), FRotator::ZeroRotator, {UEngineTypes::ConvertToObjectType(ECC_WorldStatic), UEngineTypes::ConvertToObjectType(ECC_WorldDynamic), UEngineTypes::ConvertToObjectType(ECC_DOOR)}, false, {AI}, EDrawDebugTrace::None, Hit, true);
			if (!Hit.bBlockingHit)
			{
				if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
				{
					FNavLocation NavLocation;
					//V_LOGM(LogReadyOrNot, "Pushing AI to %s", *LOSTest.TraceEnd.ToString());
					if (NavSys->ProjectPointToNavigation(LOSTest.TraceEnd + FVector(0.0f, 0.0f, -100.0f), NavLocation, FVector(100.0f, 100.0f, 100.0f)))
					{
						//DrawDebugPoint(GetWorld(), NavLocation, 100.0f, FColor::Green, false, 1.0f, 0);
						return NavLocation;
					}
				}
			}
		}		
	}
	
	return FVector::ZeroVector;
}

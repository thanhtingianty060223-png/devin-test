// Copyright Epic Games, Inc. All Rights Reserved.

#include "ReadyOrNotAISense_Sight.h"

#include "EngineDefines.h"
#include "CollisionQueryParams.h"
#include "Characters/CyberneticController.h"
#include "Characters/AI/SuspectCharacter.h"
#include "Characters/AI/SWATCharacter.h"
#include "Perception/AIPerceptionComponent.h"
#include "VisualLogger/VisualLogger.h"
#include "Perception/AISightTargetInterface.h"
#include "Perception/AISenseConfig_Sight.h"

DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight"), STAT_AI_Sense_Sight, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight, Update Sort"), STAT_AI_Sense_Sight_UpdateSort, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight, Listener Update"), STAT_AI_Sense_Sight_ListenerUpdate, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight, Register Target"), STAT_AI_Sense_Sight_RegisterTarget, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight, Remove By Listener"), STAT_AI_Sense_Sight_RemoveByListener, STATGROUP_AI);
DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight, Remove To Target"), STAT_AI_Sense_Sight_RemoveToTarget, STATGROUP_AI);

static constexpr int32 MaxTracesPerTick = 64;
static constexpr int32 MinQueriesPerTimeSliceCheck = 100;
static constexpr float MaxTimeSlicePerTick = 0.005f; // 5ms
static constexpr float HighImportanceQueryDistanceThreshold = 3000.0f;
static constexpr float HighImportanceDistanceSquare = HighImportanceQueryDistanceThreshold*HighImportanceQueryDistanceThreshold;
static constexpr float MaxQueryImportance = 60.0f;
static constexpr float SightLimitQueryImportance = 10.0f;

static constexpr int32 InitialInvalidItemsSize = 16;

FORCEINLINE_DEBUGGABLE bool CheckIsTargetInSightPie(const FPerceptionListener& Listener, const UReadyOrNotAISense_Sight::FDigestedSightProperties& DigestedProps, const FVector& TargetLocation, const float SightRadiusSq)
{
	if (FVector::DistSquared(Listener.CachedLocation, TargetLocation) <= SightRadiusSq) 
	{
		const FVector DirectionToTarget = (TargetLocation - Listener.CachedLocation).GetUnsafeNormal();
		return FVector::DotProduct(DirectionToTarget, Listener.CachedDirection) > DigestedProps.PeripheralVisionAngleCos;
	}

	return false;
}

enum class EForEachResult : uint8
{
	Break,
	Continue,
};

template <typename T, class PREDICATE_CLASS>
EForEachResult ForEach(T& Array, const PREDICATE_CLASS& Predicate)
{
	for (typename T::ElementType& Element : Array)
	{
		if (Predicate(Element) == EForEachResult::Break)
		{
			return EForEachResult::Break;
		}
	}
	return EForEachResult::Continue;
}

enum class EReverseForEachResult : uint8
{
	UnTouched,
	Modified,
};

template <typename T, class PREDICATE_CLASS>
EReverseForEachResult ReverseForEach(T& Array, const PREDICATE_CLASS& Predicate)
{
	EReverseForEachResult RetVal = EReverseForEachResult::UnTouched;
	for (int32 Index = Array.Num()-1; Index >= 0; --Index)
	{
		if (Predicate(Array, Index) == EReverseForEachResult::Modified)
		{
			RetVal = EReverseForEachResult::Modified;
		}
	}
	return RetVal;
}

//----------------------------------------------------------------------//
// FDigestedSightProperties
//----------------------------------------------------------------------//
UReadyOrNotAISense_Sight::FDigestedSightProperties::FDigestedSightProperties(const UAISenseConfig_Sight& SenseConfig)
{
	SightRadiusSq = FMath::Square(SenseConfig.SightRadius);
	LoseSightRadiusSq = FMath::Square(SenseConfig.LoseSightRadius);
	PeripheralVisionAngleCos = FMath::Cos(FMath::Clamp(FMath::DegreesToRadians(SenseConfig.PeripheralVisionAngleDegrees), 0.f, PI));
	AffiliationFlags = SenseConfig.DetectionByAffiliation.GetAsFlags();
	// keep the special value of FAISystem::InvalidRange (-1.f) if it's set.
	//AutoSuccessRangeSqFromLastSeenLocation = (SenseConfig.AutoSuccessRangeFromLastSeenLocation == FAISystem::InvalidRange) ? FAISystem::InvalidRange : FMath::Square(SenseConfig.AutoSuccessRangeFromLastSeenLocation);
	AutoSuccessRangeSqFromLastSeenLocation = FAISystem::InvalidRange;
}

UReadyOrNotAISense_Sight::FDigestedSightProperties::FDigestedSightProperties()
{
	PeripheralVisionAngleCos = 0.0f;
	SightRadiusSq = -1.0f;
	AutoSuccessRangeSqFromLastSeenLocation = FAISystem::InvalidRange;
	LoseSightRadiusSq = -1.0f;
	AffiliationFlags = -1;
}

const FReadyOrNotAISightTarget::FReadyOrNotTargetId FReadyOrNotAISightTarget::InvalidTargetId = FAISystem::InvalidUnsignedID;

// ##UE5.3UPGRADE##
// #if WITH_EDITOR
// const FAISightTarget::FTargetId FAISightTarget::InvalidTargetId = FAISystem::InvalidUnsignedID;
// #endif
// ##UE5.3UPGRADE##

FReadyOrNotAISightTarget::FReadyOrNotAISightTarget(AActor* InTarget, FGenericTeamId InTeamId)
{
	Target = InTarget;
	SightTargetInterface = nullptr;
	TeamId = InTeamId;
	
	if (InTarget)
	{
		TargetId = InTarget->GetUniqueID();
	}
	else
	{
		TargetId = InvalidTargetId;
	}
}

UReadyOrNotAISense_Sight::UReadyOrNotAISense_Sight(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		UAISenseConfig_Sight* SightConfigCDO = GetMutableDefault<UAISenseConfig_Sight>();
		SightConfigCDO->Implementation = StaticClass();

		OnNewListenerDelegate.BindUObject(this, &UReadyOrNotAISense_Sight::OnNewListenerImpl);
		OnListenerUpdateDelegate.BindUObject(this, &UReadyOrNotAISense_Sight::OnListenerUpdateImpl);
		OnListenerRemovedDelegate.BindUObject(this, &UReadyOrNotAISense_Sight::OnListenerRemovedImpl);
	}

	NotifyType = EAISenseNotifyType::OnPerceptionChange;
	
	bAutoRegisterAllPawnsAsSources = true;
	bNeedsForgettingNotification = true;
}

FORCEINLINE_DEBUGGABLE float UReadyOrNotAISense_Sight::CalcQueryImportance(const FPerceptionListener& Listener, const FVector& TargetLocation, const float SightRadiusSq) const
{
	const float DistanceSq = FVector::DistSquared(Listener.CachedLocation, TargetLocation);
	
	return DistanceSq <= HighImportanceDistanceSquare ? MaxQueryImportance
		: FMath::Clamp((SightLimitQueryImportance - MaxQueryImportance) / SightRadiusSq * DistanceSq + MaxQueryImportance, 0.f, MaxQueryImportance);
}

bool UReadyOrNotAISense_Sight::ShouldAutomaticallySeeTarget(const FDigestedSightProperties& PropDigest, FAISightQuery* SightQuery, FPerceptionListener& Listener, AActor* TargetActor, float& OutStimulusStrength) const
{
	OutStimulusStrength = 1.0f;

	if ((PropDigest.AutoSuccessRangeSqFromLastSeenLocation != FAISystem::InvalidRange) && (SightQuery->LastSeenLocation != FAISystem::InvalidLocation))
	{
		const float DistanceToLastSeenLocationSq = FVector::DistSquared(TargetActor->GetActorLocation(), SightQuery->LastSeenLocation);
		return (DistanceToLastSeenLocationSq <= PropDigest.AutoSuccessRangeSqFromLastSeenLocation);
	}

	return false;
}

float UReadyOrNotAISense_Sight::Update()
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Sense_Sight);

	const UWorld* World = GetWorld();

	if (!World)
	{
		return SuspendNextUpdate;
	}

	// sort Sight Queries
	{
		SCOPE_CYCLE_COUNTER(STAT_AI_Sense_Sight_UpdateSort);
		
		auto RecalcScore = [](FAISightQuery& SightQuery)->EForEachResult
		{
			SightQuery.RecalcScore();
			return EForEachResult::Continue;
		};

        // Sort out of range queries
    	if (bSightQueriesOutOfRangeDirty)
		{
			ForEach(SightQueriesOutOfRange, RecalcScore);
			SightQueriesOutOfRange.Sort(FAISightQuery::FSortPredicate());
			NextOutOfRangeIndex = 0;
			bSightQueriesOutOfRangeDirty = false;
		}

        // Sort in range queries
		ForEach(SightQueriesInRange, RecalcScore);
		SightQueriesInRange.Sort(FAISightQuery::FSortPredicate());
	}

	int32 TracesCount = 0;
	int32 NumQueriesProcessed = 0;
	double TimeSliceEnd = FPlatformTime::Seconds() + MaxTimeSlicePerTick;
	
	/*
	//#define AISENSE_SIGHT_TIMESLICING_DEBUG
	
	#ifdef AISENSE_SIGHT_TIMESLICING_DEBUG
	double TimeSpent = 0.0;
	double LastTime = FPlatformTime::Seconds();
	#endif
	*/
	
	enum class EOperationType : uint8
	{
		Remove,
		SwapList
	};
	
	struct FQueryOperation
	{
		FQueryOperation(bool bInInRange, EOperationType InOpType, int32 InIndex)
		{
			bInRange = bInInRange;
			OpType = InOpType;
			Index = InIndex;
		}
		
		bool bInRange;
		EOperationType OpType;
		int32 Index;
	};
	
	TArray<FQueryOperation> QueryOperations;
	TArray<FReadyOrNotAISightTarget::FReadyOrNotTargetId> InvalidTargets;
	QueryOperations.Reserve(InitialInvalidItemsSize);
	InvalidTargets.Reserve(InitialInvalidItemsSize);

	AIPerception::FListenerMap* ListenersMap = GetListeners();
	
	bool bHitTimeSliceLimit = false;

	int32 InRangeItr = 0;
	int32 OutOfRangeItr = 0;
	for (int32 QueryIndex = 0; QueryIndex < SightQueriesInRange.Num() + SightQueriesOutOfRange.Num(); ++QueryIndex)
	{
		// Calculate next "in range" query
		int32 InRangeIndex = SightQueriesInRange.IsValidIndex(InRangeItr) ? InRangeItr : INDEX_NONE;
		FAISightQuery* InRangeQuery = InRangeIndex != INDEX_NONE ? &SightQueriesInRange[InRangeIndex] : nullptr;

		// Calculate next "out of range" query
		int32 OutOfRangeIndex = SightQueriesOutOfRange.IsValidIndex(OutOfRangeItr) ? (NextOutOfRangeIndex + OutOfRangeItr) % SightQueriesOutOfRange.Num() : INDEX_NONE;
		FAISightQuery* OutOfRangeQuery = OutOfRangeIndex != INDEX_NONE ? &SightQueriesOutOfRange[OutOfRangeIndex] : nullptr;
		
		if (OutOfRangeQuery)
		{
			OutOfRangeQuery->RecalcScore();
		}

		// Compare to real find next query
		const bool bIsInRangeQuery = (InRangeQuery && OutOfRangeQuery) ? FAISightQuery::FSortPredicate()(*InRangeQuery,*OutOfRangeQuery) : !OutOfRangeQuery;
		FAISightQuery* SightQuery = bIsInRangeQuery ? InRangeQuery : OutOfRangeQuery;

		if (!SightQuery)
			continue;

		// Time slice limit check - spread out checks to every N queries so we don't spend more time checking timer than doing work
		NumQueriesProcessed++;
		
		#ifdef AISENSE_SIGHT_TIMESLICING_DEBUG
		TimeSpent += (FPlatformTime::Seconds() - LastTime);
		LastTime = FPlatformTime::Seconds();
		#endif
		
		if (!bHitTimeSliceLimit && (NumQueriesProcessed % MinQueriesPerTimeSliceCheck) == 0 && FPlatformTime::Seconds() > TimeSliceEnd)
		{
			bHitTimeSliceLimit = true;
			// do not break here since that would bypass queue aging
		}

		if (TracesCount < MaxTracesPerTick && !bHitTimeSliceLimit)
		{
			bIsInRangeQuery ? ++InRangeItr : ++OutOfRangeItr;

			FPerceptionListener& Listener = (*ListenersMap)[SightQuery->ObserverId];
			FReadyOrNotAISightTarget& Target = ObservedTargets[SightQuery->TargetId];

			if (!Listener.Listener.IsValid())
				continue;
			
			if (!Target.Target.IsValid())
				continue;
			
			AActor* TargetActor = Target.Target.Get();
			
			if (!TargetActor)
				continue;
			
			ASpectatorPawn* Spectator = Cast<ASpectatorPawn>(TargetActor);
			if (Spectator)
				continue;
			
			if (UAIPerceptionComponent* ListenerPtr = Listener.Listener.Get())
			{
				if (AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(TargetActor))
				{
					if (!Character->bIsRelevant)
						continue;
					
					ACyberneticController* CyberneticController = Cast<ACyberneticController>(ListenerPtr->GetOwner());
					if (CyberneticController)
					{
						// Ignore fellow team mates
						if (!Character->IsPlayerControlled())
						{
							//if (Character->IsSuspect() && CyberneticController->IsSuspect())
								//continue;
							
							if (Character->IsOnSWATTeam() && CyberneticController->IsSWAT())
								continue;
							
						}
					}
					else
					{
						continue;
					}
					
				}

				const FVector TargetLocation = TargetActor->GetActorLocation();
				
				const FDigestedSightProperties& PropDigest = DigestedProperties[SightQuery->ObserverId];
				// ##UE5.3UPGRADE##
				const float SightRadiusSq = SightQuery->FrameInfo.bLastResult ? PropDigest.LoseSightRadiusSq : PropDigest.SightRadiusSq;
				// ##UE5.3UPGRADE##

				// defaulting to 1 to have "full strength" by default instead of "no strength"
				float StimulusStrength = 1.0f;
				
				// @Note that automagical "seeing" does not care about sight range nor vision cone
				const bool bShouldAutomatically = ShouldAutomaticallySeeTarget(PropDigest, SightQuery, Listener, TargetActor, StimulusStrength);
				if (bShouldAutomatically)
				{
					// Pretend like we've seen this target where we last saw them
					Listener.RegisterStimulus(TargetActor, FAIStimulus(*this, StimulusStrength, SightQuery->LastSeenLocation, Listener.CachedLocation));
					// ##UE5.3UPGRADE##
					SightQuery->FrameInfo.bLastResult = true;
					// ##UE5.3UPGRADE##
				}
				else if (CheckIsTargetInSightPie(Listener, PropDigest, TargetLocation, SightRadiusSq))
				{
					//SIGHT_LOG_SEGMENT(ListenerPtr->GetOwner(), Listener.CachedLocation, TargetLocation, FColor::Green, TEXT("%s"), *(Target.TargetId.ToString()));

					FVector OutSeenLocation = FVector::ZeroVector;
					
					// do line of sight checks
					if (Target.SightTargetInterface)
					{
						int32 NumberOfLoSChecksPerformed = 0;

						if (Target.SightTargetInterface->CanBeSeenFrom(Listener.CachedLocation, OutSeenLocation, NumberOfLoSChecksPerformed, StimulusStrength, ListenerPtr->GetBodyActor(), nullptr, nullptr))
						{
							ACyberneticController* CyberneticController = Cast<ACyberneticController>(ListenerPtr->GetOwner());
							if (CyberneticController && CyberneticController->GetCharacter())
							{
								ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(TargetActor);
								if (CyberneticCharacter)
								{
									NumberOfLoSChecksPerformed++;
									if (!CyberneticCharacter->HasLineOfSightToCharacter(CyberneticController->GetCharacter()))
									{
										continue;
									}
								}
							}
							
							Listener.RegisterStimulus(TargetActor, FAIStimulus(*this, StimulusStrength, OutSeenLocation, Listener.CachedLocation));
							// ##UE5.3UPGRADE##
							SightQuery->FrameInfo.bLastResult = true;
							// ##UE5.3UPGRADE##
							SightQuery->LastSeenLocation = OutSeenLocation;
						}
						// communicate failure only if we've seen given actor before
						// ##UE5.3UPGRADE##
						else if (SightQuery->FrameInfo.bLastResult)
						// ##UE5.3UPGRADE##
						{
							Listener.RegisterStimulus(TargetActor, FAIStimulus(*this, 0.0f, TargetLocation, Listener.CachedLocation, FAIStimulus::SensingFailed));
							// ##UE5.3UPGRADE##
							SightQuery->FrameInfo.bLastResult = false;
							// ##UE5.3UPGRADE##
							SightQuery->LastSeenLocation = FAISystem::InvalidLocation;
						}

						/*
						if (!SightQuery->FrameInfo.bLastResult)
						{
							SIGHT_LOG_LOCATION(ListenerPtr->GetOwner(), TargetLocation, 25.f, FColor::Red, TEXT(""));
						}
						*/

						TracesCount += NumberOfLoSChecksPerformed;
					}
					else
					{
						// we need to do tests ourselves
						FHitResult HitResult;
						
						FCollisionQueryParams CollisionQueryParams;

						if (const AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(ListenerPtr->GetBodyActor()))
						{
							CollisionQueryParams = OwnerCharacter->GetCollisionQueryParameters();
							CollisionQueryParams.AddIgnoredActor(TargetActor);
							CollisionQueryParams.bTraceComplex = true;
							CollisionQueryParams.TraceTag = SCENE_QUERY_STAT_NAME_ONLY(AILineOfSight);
							CollisionQueryParams.StatId = SCENE_QUERY_STAT_ONLY(AILineOfSight);
						}
						
						const bool bHit = World->LineTraceSingleByChannel(HitResult, Listener.CachedLocation, TargetLocation, ECC_Visibility, CollisionQueryParams);

						TracesCount++;

						AActor* HitResultActor = HitResult.HitObjectHandle.FetchActor();

						bool bHitResultActorIsOwnedByTargetActor = HitResultActor ? HitResultActor->IsOwnedBy(TargetActor) : false;

						if (!bHit || bHitResultActorIsOwnedByTargetActor)
						{
							Listener.RegisterStimulus(TargetActor, FAIStimulus(*this, 1.0f, TargetLocation, Listener.CachedLocation));
							SightQuery->FrameInfo.bLastResult = true;
							SightQuery->LastSeenLocation = TargetLocation;
						}
						// communicate failure only if we've seen give actor before
						else if (SightQuery->FrameInfo.bLastResult == true)
						{
							Listener.RegisterStimulus(TargetActor, FAIStimulus(*this, 0.0f, TargetLocation, Listener.CachedLocation, FAIStimulus::SensingFailed));
							SightQuery->FrameInfo.bLastResult = false;
							SightQuery->LastSeenLocation = FAISystem::InvalidLocation;
						}

						/*
						if (!SightQuery->FrameInfo.bLastResult)
						{
							SIGHT_LOG_LOCATION(ListenerPtr->GetOwner(), TargetLocation, 25.f, FColor::Red, TEXT(""));
						}
						*/
					}
				}
				// communicate failure only if we've seen give actor before
				else if (SightQuery->FrameInfo.bLastResult)
				{
					//SIGHT_LOG_SEGMENT(ListenerPtr->GetOwner(), Listener.CachedLocation, TargetLocation, FColor::Red, TEXT("%s"), *(Target.TargetId.ToString()));
					
					Listener.RegisterStimulus(TargetActor, FAIStimulus(*this, 0.f, TargetLocation, Listener.CachedLocation, FAIStimulus::SensingFailed));
					SightQuery->FrameInfo.bLastResult = false;
				}

				SightQuery->Importance = CalcQueryImportance(Listener, TargetLocation, SightRadiusSq);
				const bool bShouldBeInRange = SightQuery->Importance > 0.0f;
				if (bIsInRangeQuery != bShouldBeInRange)
				{
					QueryOperations.Add(FQueryOperation(bIsInRangeQuery, EOperationType::SwapList, bIsInRangeQuery ? InRangeIndex : OutOfRangeIndex));
				}

				// restart query
				SightQuery->OnProcessed();
            }
			else
			{
				// put this index to "to be removed" array
				QueryOperations.Add(FQueryOperation(bIsInRangeQuery, EOperationType::Remove, bIsInRangeQuery ? InRangeIndex : OutOfRangeIndex));
				if (!TargetActor)
				{
					InvalidTargets.AddUnique(SightQuery->TargetId);
				}
			}
		}
		else
		{
			break;
		}
	}
	
	NextOutOfRangeIndex = SightQueriesOutOfRange.Num() > 0 ? (NextOutOfRangeIndex + OutOfRangeItr) % SightQueriesOutOfRange.Num() : 0;

	#ifdef AISENSE_SIGHT_TIMESLICING_DEBUG
	UE_LOG(LogAIPerception, VeryVerbose, TEXT("UReadyOrNotAISense_Sight::Update processed %d sources in %f seconds [time slice limited? %d]"), NumQueriesProcessed, TimeSpent, bHitTimeSliceLimit ? 1 : 0);
	#else
	UE_LOG(LogAIPerception, VeryVerbose, TEXT("UReadyOrNotAISense_Sight::Update processed %d sources [time slice limited? %d]"), NumQueriesProcessed, bHitTimeSliceLimit ? 1 : 0);
	#endif

	if (QueryOperations.Num() > 0)
	{
		// Sort by InRange and by descending Index 
		QueryOperations.Sort([](const FQueryOperation& LHS, const FQueryOperation& RHS)->bool
		{
			if (LHS.bInRange != RHS.bInRange)
				return LHS.bInRange;
				
			return LHS.Index > RHS.Index;
		});
		
        // Do all the removes first and save the out of range swaps because we will insert them at the right location to prevent sorting
		TArray<FAISightQuery> SightQueriesOutOfRangeToInsert;
		for (FQueryOperation& Operation : QueryOperations)
		{
			if (Operation.OpType == EOperationType::SwapList)
			{
				if (Operation.bInRange)
				{
					SightQueriesOutOfRangeToInsert.Push(SightQueriesInRange[Operation.Index]);
				}
				else
				{
					SightQueriesInRange.Add(SightQueriesOutOfRange[Operation.Index]);
				}
			}

			if (Operation.bInRange)
			{
				// In range queries are always sorted at the beginning of the update
				SightQueriesInRange.RemoveAtSwap(Operation.Index, 1, /*bAllowShrinking*/false);
			}
			else
			{
				// Preserve the list ordered
				SightQueriesOutOfRange.RemoveAt(Operation.Index, 1, /*bAllowShrinking*/false);
				if (Operation.Index < NextOutOfRangeIndex)
				{
					NextOutOfRangeIndex--;
				}
			}
		}
        // Reinsert the saved out of range swaps
		if (SightQueriesOutOfRangeToInsert.Num() > 0)
		{
			SightQueriesOutOfRange.Insert(SightQueriesOutOfRangeToInsert.GetData(), SightQueriesOutOfRangeToInsert.Num(), NextOutOfRangeIndex);
			NextOutOfRangeIndex += SightQueriesOutOfRangeToInsert.Num();
		}

		if (InvalidTargets.Num() > 0)
		{
			// this should not be happening since UAIPerceptionSystem::OnPerceptionStimuliSourceEndPlay introduction
			UE_VLOG(GetPerceptionSystem(), LogAIPerception, Error, TEXT("Invalid sight targets found during UReadyOrNotAISense_Sight::Update call"));

			for (const auto& TargetId : InvalidTargets)
			{
				// remove affected queries
				RemoveAllQueriesToTarget(TargetId);
				// remove target itself
				ObservedTargets.Remove(TargetId);
			}

			// remove holes
			ObservedTargets.Compact();
		}
	}

	//LOG_NUMBER(TracesCount);

	//return SightQueries.Num() > 0 ? 1.f/6 : FLT_MAX;
	return 0.0f;
}

void UReadyOrNotAISense_Sight::RegisterSource(AActor& SourceActor)
{
	RegisterTarget(SourceActor);
}

void UReadyOrNotAISense_Sight::UnregisterSource(AActor& SourceActor)
{
	const FReadyOrNotAISightTarget::FReadyOrNotTargetId AsTargetId = SourceActor.GetUniqueID();
	FReadyOrNotAISightTarget AsTarget;
	
	if (ObservedTargets.RemoveAndCopyValue(AsTargetId, AsTarget) 
		&& (SightQueriesInRange.Num() + SightQueriesOutOfRange.Num()) > 0)
	{
		AActor* TargetActor = AsTarget.Target.Get();

		if (TargetActor)
		{
			// notify all interested observers that this source is no longer
			// visible		
			AIPerception::FListenerMap& ListenersMap = *GetListeners();
			auto RemoveQuery = [this,&ListenersMap,&AsTargetId,&TargetActor](TArray<FAISightQuery>& SightQueries, const int32 QueryIndex)->EReverseForEachResult
			{
				FAISightQuery* SightQuery = &SightQueries[QueryIndex];
				if (SightQuery->TargetId == AsTargetId)
				{
					if (SightQuery->FrameInfo.bLastResult == true)
					{
						FPerceptionListener& Listener = ListenersMap[SightQuery->ObserverId];
						ensure(Listener.Listener.IsValid());

						Listener.RegisterStimulus(TargetActor, FAIStimulus(*this, 0.f, SightQuery->LastSeenLocation, Listener.CachedLocation, FAIStimulus::SensingFailed));
					}

					SightQueries.RemoveAtSwap(QueryIndex, 1, /*bAllowShrinking=*/false);
					return EReverseForEachResult::Modified;
				}
				return EReverseForEachResult::UnTouched;
			};
			ReverseForEach(SightQueriesInRange, RemoveQuery);
			if (ReverseForEach(SightQueriesOutOfRange, RemoveQuery) == EReverseForEachResult::Modified)
			{
				bSightQueriesOutOfRangeDirty = true;
			}
		}
	}
}

bool UReadyOrNotAISense_Sight::RegisterTarget(AActor& TargetActor, const TFunction<void(FAISightQuery&)>& OnAddedFunc /*= nullptr*/)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Sense_Sight_RegisterTarget);
	
	FReadyOrNotAISightTarget* SightTarget = ObservedTargets.Find(TargetActor.GetUniqueID());
	
	if (SightTarget != nullptr && SightTarget->Target.Get() != &TargetActor)
	{
		// this means given unique ID has already been recycled. 
		FReadyOrNotAISightTarget NewSightTarget(&TargetActor);

		SightTarget = &(ObservedTargets.Add(NewSightTarget.TargetId, NewSightTarget));
		SightTarget->SightTargetInterface = Cast<IAISightTargetInterface>(&TargetActor);
	}
	else if (SightTarget == nullptr)
	{
		FReadyOrNotAISightTarget NewSightTarget(&TargetActor);

		SightTarget = &(ObservedTargets.Add(NewSightTarget.TargetId, NewSightTarget));
		SightTarget->SightTargetInterface = Cast<IAISightTargetInterface>(&TargetActor);
	}

	// set/update data
	SightTarget->TeamId = FGenericTeamId::GetTeamIdentifier(&TargetActor);
	
	// generate all pairs and add them to current Sight Queries
	bool bNewQueriesAdded = false;
	AIPerception::FListenerMap& ListenersMap = *GetListeners();
	const FVector TargetLocation = TargetActor.GetActorLocation();

	for (AIPerception::FListenerMap::TConstIterator ItListener(ListenersMap); ItListener; ++ItListener)
	{
		const FPerceptionListener& Listener = ItListener->Value;
		const IGenericTeamAgentInterface* ListenersTeamAgent = Listener.GetTeamAgent();

		if (Listener.HasSense(GetSenseID()) && Listener.GetBodyActor() != &TargetActor)
		{
			const FDigestedSightProperties& PropDigest = DigestedProperties[Listener.GetListenerID()];
			if (FAISenseAffiliationFilter::ShouldSenseTeam(ListenersTeamAgent, TargetActor, PropDigest.AffiliationFlags))
			{
				// create a sight query		
				const float Importance = CalcQueryImportance(ItListener->Value, TargetLocation, PropDigest.SightRadiusSq);
				const bool bInRange = Importance > 0.0f;
				if (!bInRange)
				{
					bSightQueriesOutOfRangeDirty = true;
				}
				FAISightQuery& AddedQuery = bInRange ? SightQueriesInRange.AddDefaulted_GetRef() : SightQueriesOutOfRange.AddDefaulted_GetRef();
				AddedQuery.ObserverId = ItListener->Key;
				AddedQuery.TargetId = SightTarget->TargetId;
				AddedQuery.Importance = Importance;
				
				if (OnAddedFunc)
				{
					OnAddedFunc(AddedQuery);
				}
				bNewQueriesAdded = true;
			}
		}
	}

	// sort Sight Queries
	if (bNewQueriesAdded)
	{
		RequestImmediateUpdate();
	}

	return bNewQueriesAdded;
}

void UReadyOrNotAISense_Sight::OnNewListenerImpl(const FPerceptionListener& NewListener)
{
	if (UAIPerceptionComponent* NewListenerPtr = NewListener.Listener.Get())
	{
		if (const UAISenseConfig_Sight* SenseConfig = Cast<const UAISenseConfig_Sight>(NewListenerPtr->GetSenseConfig(GetSenseID())))
		{
			const FDigestedSightProperties PropertyDigest(*SenseConfig);
			DigestedProperties.Add(NewListener.GetListenerID(), PropertyDigest);

			GenerateQueriesForListener(NewListener, PropertyDigest);
		}
	}
}

void UReadyOrNotAISense_Sight::GenerateQueriesForListener(const FPerceptionListener& Listener, const FDigestedSightProperties& PropertyDigest, const TFunction<void(FAISightQuery&)>& OnAddedFunc/*= nullptr */)
{
	bool bNewQueriesAdded = false;
	const IGenericTeamAgentInterface* ListenersTeamAgent = Listener.GetTeamAgent();
	const AActor* Avatar = Listener.GetBodyActor();

	// create sight queries with all legal targets
	for (FTargetsContainer::TConstIterator ItTarget(ObservedTargets); ItTarget; ++ItTarget)
	{
		const AActor* TargetActor = ItTarget->Value.Target.Get();
		if (TargetActor == NULL || TargetActor == Avatar)
		{
			continue;
		}

		if (FAISenseAffiliationFilter::ShouldSenseTeam(ListenersTeamAgent, *TargetActor, PropertyDigest.AffiliationFlags))
		{
			// create a sight query		
			const float Importance = CalcQueryImportance(Listener, ItTarget->Value.GetLocation(), PropertyDigest.SightRadiusSq);
			const bool bInRange = Importance > 0.0f;
			if (!bInRange)
			{
				bSightQueriesOutOfRangeDirty = true;
			}
			FAISightQuery& AddedQuery = bInRange ? SightQueriesInRange.AddDefaulted_GetRef() : SightQueriesOutOfRange.AddDefaulted_GetRef();
			AddedQuery.ObserverId = Listener.GetListenerID();
			AddedQuery.TargetId = ItTarget->Key;
			AddedQuery.Importance = Importance;

			if (OnAddedFunc)
			{
				OnAddedFunc(AddedQuery);
			}
			bNewQueriesAdded = true;
		}
	}

	// sort Sight Queries
	if (bNewQueriesAdded)
	{
		RequestImmediateUpdate();
	}
}

void UReadyOrNotAISense_Sight::OnListenerUpdateImpl(const FPerceptionListener& UpdatedListener)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Sense_Sight_ListenerUpdate);

	// first, naive implementation:
	// 1. remove all queries by this listener
	// 2. proceed as if it was a new listener

	// see if this listener is a Target as well
	const FReadyOrNotAISightTarget::FReadyOrNotTargetId AsTargetId = UpdatedListener.GetBodyActorUniqueID();
	FReadyOrNotAISightTarget* AsTarget = ObservedTargets.Find(AsTargetId);
	if (AsTarget != NULL)
	{
		if (AsTarget->Target.IsValid())
		{
			// if still a valid target then backup list of observers for which the listener was visible to restore in the newly created queries
			TSet<FPerceptionListenerID> LastVisibleObservers;
			RemoveAllQueriesToTarget(AsTargetId, [&LastVisibleObservers](const FAISightQuery& Query)
			{
				if (Query.FrameInfo.bLastResult)
				{
					LastVisibleObservers.Add(Query.ObserverId);
				}
			});

			RegisterTarget(*(AsTarget->Target.Get()), [&LastVisibleObservers](FAISightQuery& Query)
			{
				Query.FrameInfo.bLastResult = LastVisibleObservers.Contains(Query.ObserverId);
			});
		}
		else
		{
			RemoveAllQueriesToTarget(AsTargetId);
		}
	}

	const FPerceptionListenerID ListenerID = UpdatedListener.GetListenerID();

	if (UpdatedListener.HasSense(GetSenseID()))
	{
		// if still a valid sense then backup list of targets that were visible by the listener to restore in the newly created queries
		TSet<FReadyOrNotAISightTarget::FReadyOrNotTargetId> LastVisibleTargets;
		RemoveAllQueriesByListener(UpdatedListener, [&LastVisibleTargets](const FAISightQuery& Query)
		{
			if (Query.FrameInfo.bLastResult)
			{
				LastVisibleTargets.Add(Query.TargetId);
			}			
		});

		const UAISenseConfig_Sight* SenseConfig = Cast<const UAISenseConfig_Sight>(UpdatedListener.Listener->GetSenseConfig(GetSenseID()));
		check(SenseConfig);
		FDigestedSightProperties& PropertiesDigest = DigestedProperties.FindOrAdd(ListenerID);
		PropertiesDigest = FDigestedSightProperties(*SenseConfig);

		GenerateQueriesForListener(UpdatedListener, PropertiesDigest, [&LastVisibleTargets](FAISightQuery& Query)
		{
			Query.FrameInfo.bLastResult = LastVisibleTargets.Contains(Query.TargetId);
		});
	}
	else
	{
		// remove all queries
		RemoveAllQueriesByListener(UpdatedListener);

		DigestedProperties.Remove(ListenerID);
	}
}

void UReadyOrNotAISense_Sight::OnListenerConfigUpdated(const FPerceptionListener& UpdatedListener)
{
	bool bSkipListenerUpdate = false;
	const FPerceptionListenerID ListenerID = UpdatedListener.GetListenerID();

	FDigestedSightProperties* PropertiesDigest = DigestedProperties.Find(ListenerID);
	if (PropertiesDigest)
	{
		// The only parameter we need to rebuild all the queries for this listener is if the affiliation mask changed, otherwise there is nothing to update.
		const UAISenseConfig_Sight* SenseConfig = CastChecked<const UAISenseConfig_Sight>(UpdatedListener.Listener->GetSenseConfig(GetSenseID()));
		FDigestedSightProperties NewPropertiesDigest(*SenseConfig);
		bSkipListenerUpdate = NewPropertiesDigest.AffiliationFlags == PropertiesDigest->AffiliationFlags;
		*PropertiesDigest = NewPropertiesDigest;
	}

	if (!bSkipListenerUpdate)
	{
		Super::OnListenerConfigUpdated(UpdatedListener);
	}
}

void UReadyOrNotAISense_Sight::OnListenerRemovedImpl(const FPerceptionListener& RemovedListener)
{
	RemoveAllQueriesByListener(RemovedListener);

	DigestedProperties.FindAndRemoveChecked(RemovedListener.GetListenerID());

	// note: there use to be code to remove all queries _to_ listener here as well
	// but that was wrong - the fact that a listener gets unregistered doesn't have to
	// mean it's being removed from the game altogether.
}

void UReadyOrNotAISense_Sight::RemoveAllQueriesByListener(const FPerceptionListener& Listener, const TFunction<void(const FAISightQuery&)>& OnRemoveFunc/*= nullptr */)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Sense_Sight_RemoveByListener);

	if ((SightQueriesInRange.Num() + SightQueriesOutOfRange.Num()) == 0)
	{
		return;
	}

	const uint32 ListenerId = Listener.GetListenerID();
	
	auto RemoveQuery = [&ListenerId, &OnRemoveFunc](TArray<FAISightQuery>& SightQueries, const int32 QueryIndex)->EReverseForEachResult
	{
		const FAISightQuery& SightQuery = SightQueries[QueryIndex];

		if (SightQuery.ObserverId == ListenerId)
		{
			if (OnRemoveFunc)
			{
				OnRemoveFunc(SightQuery);
			}
			SightQueries.RemoveAtSwap(QueryIndex, 1, /*bAllowShrinking=*/false);
			return EReverseForEachResult::Modified;
		}
		return EReverseForEachResult::UnTouched;
	};
	ReverseForEach(SightQueriesInRange, RemoveQuery);
	if(ReverseForEach(SightQueriesOutOfRange, RemoveQuery) == EReverseForEachResult::Modified)
	{
		bSightQueriesOutOfRangeDirty = true;
	}
}

void UReadyOrNotAISense_Sight::RemoveAllQueriesToTarget(const FReadyOrNotAISightTarget::FReadyOrNotTargetId& TargetId, const TFunction<void(const FAISightQuery&)>& OnRemoveFunc/*= nullptr */)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Sense_Sight_RemoveToTarget);

	auto RemoveQuery = [&TargetId, &OnRemoveFunc](TArray<FAISightQuery>& SightQueries, const int32 QueryIndex)->EReverseForEachResult
	{
		const FAISightQuery& SightQuery = SightQueries[QueryIndex];

		if (SightQuery.TargetId == TargetId)
		{
			if (OnRemoveFunc)
			{
				OnRemoveFunc(SightQuery);
			}
			SightQueries.RemoveAtSwap(QueryIndex, 1, /*bAllowShrinking=*/false);
			return EReverseForEachResult::Modified;
		}
		return EReverseForEachResult::UnTouched;
	};
	ReverseForEach(SightQueriesInRange, RemoveQuery);
	if (ReverseForEach(SightQueriesOutOfRange, RemoveQuery) == EReverseForEachResult::Modified)
	{
		bSightQueriesOutOfRangeDirty = true;
	}
}

void UReadyOrNotAISense_Sight::OnListenerForgetsActor(const FPerceptionListener& Listener, AActor& ActorToForget)
{
	const uint32 ListenerId = Listener.GetListenerID();
	const uint32 TargetId = ActorToForget.GetUniqueID();
	
	auto ForgetPreviousResult = [&ListenerId, &TargetId](FAISightQuery& SightQuery)->EForEachResult
	{
		if (SightQuery.ObserverId == ListenerId && SightQuery.TargetId == TargetId)
		{
			// assuming one query per observer-target pair
			SightQuery.ForgetPreviousResult();
			return EForEachResult::Break;
		}
		return EForEachResult::Continue;
	};

	if (ForEach(SightQueriesInRange, ForgetPreviousResult) == EForEachResult::Continue)
	{
		ForEach(SightQueriesOutOfRange, ForgetPreviousResult);
	}
}

void UReadyOrNotAISense_Sight::OnListenerForgetsAll(const FPerceptionListener& Listener)
{
	const uint32 ListenerId = Listener.GetListenerID();

	auto ForgetPreviousResult = [&ListenerId](FAISightQuery& SightQuery)->EForEachResult
	{
		if (SightQuery.ObserverId == ListenerId)
		{
			SightQuery.ForgetPreviousResult();
		}
		return EForEachResult::Continue;
	};

	ForEach(SightQueriesInRange, ForgetPreviousResult);
	ForEach(SightQueriesOutOfRange, ForgetPreviousResult);
}

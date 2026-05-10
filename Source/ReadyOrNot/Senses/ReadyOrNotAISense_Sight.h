// Void Interactive, 2020

#pragma once

#include "Perception/AISense_Sight.h"
#include "ReadyOrNotAISense_Sight.generated.h"

struct FReadyOrNotAISightTarget
{
	typedef uint32 FReadyOrNotTargetId;
	static const FReadyOrNotTargetId InvalidTargetId;

	TWeakObjectPtr<AActor> Target;
	IAISightTargetInterface* SightTargetInterface;
	FGenericTeamId TeamId;
	FReadyOrNotTargetId TargetId;

	FReadyOrNotAISightTarget(AActor* InTarget = NULL, FGenericTeamId InTeamId = FGenericTeamId::NoTeam);

	FORCEINLINE FVector GetLocation() const
	{
		const AActor* TargetPtr = Target.Get();
		return TargetPtr ? TargetPtr->GetActorLocation() : FVector::ZeroVector;
	}
};

UCLASS(ClassGroup=AI, config=Game)
class READYORNOT_API UReadyOrNotAISense_Sight final : public UAISense
{
	GENERATED_BODY()

public:
	explicit UReadyOrNotAISense_Sight(const FObjectInitializer& ObjectInitializer);
	
	struct FDigestedSightProperties
	{
		FDigestedSightProperties();
		explicit FDigestedSightProperties(const UAISenseConfig_Sight& SenseConfig);
		
		float PeripheralVisionAngleCos;
		float SightRadiusSq;
		float AutoSuccessRangeSqFromLastSeenLocation;
		float LoseSightRadiusSq;
		uint8 AffiliationFlags;
	};	
	
	typedef TMap<FReadyOrNotAISightTarget::FReadyOrNotTargetId, FReadyOrNotAISightTarget> FTargetsContainer;
	FTargetsContainer ObservedTargets;
	
	TMap<FPerceptionListenerID, FDigestedSightProperties> DigestedProperties;

	/** The SightQueries are a n^2 problem and to reduce the sort time, they are now split between in range and out of range */
	/** Since the out of range queries only age as the distance component of the score is always 0, there is few need to sort them */
	/** In the majority of the cases most of the queries are out of range, so the sort time is greatly reduced as we only sort the in range queries */
	int32 NextOutOfRangeIndex = 0;
	bool bSightQueriesOutOfRangeDirty = true;
	TArray<FAISightQuery> SightQueriesOutOfRange;
	TArray<FAISightQuery> SightQueriesInRange;
	
	virtual void RegisterSource(AActor& SourceActors) override;
	virtual void UnregisterSource(AActor& SourceActor) override;
	
	virtual void OnListenerForgetsActor(const FPerceptionListener& Listener, AActor& ActorToForget) override;
	virtual void OnListenerForgetsAll(const FPerceptionListener& Listener) override;

protected:
	virtual float Update() override;

	bool ShouldAutomaticallySeeTarget(const FDigestedSightProperties& PropDigest, FAISightQuery* SightQuery, FPerceptionListener& Listener, AActor* TargetActor, float& OutStimulusStrength) const;

	void OnNewListenerImpl(const FPerceptionListener& NewListener);
	void OnListenerUpdateImpl(const FPerceptionListener& UpdatedListener);
	void OnListenerRemovedImpl(const FPerceptionListener& RemovedListener);
	virtual void OnListenerConfigUpdated(const FPerceptionListener& UpdatedListener) override;
	
	void GenerateQueriesForListener(const FPerceptionListener& Listener, const FDigestedSightProperties& PropertyDigest, const TFunction<void(FAISightQuery&)>& OnAddedFunc = nullptr);

	void RemoveAllQueriesByListener(const FPerceptionListener& Listener, const TFunction<void(const FAISightQuery&)>& OnRemoveFunc = nullptr);
	void RemoveAllQueriesToTarget(const FReadyOrNotAISightTarget::FReadyOrNotTargetId& TargetId, const TFunction<void(const FAISightQuery&)>& OnRemoveFunc = nullptr);

	// returns information whether new LOS queries have been added
	bool RegisterTarget(AActor& TargetActor, const TFunction<void(FAISightQuery&)>& OnAddedFunc = nullptr);

	float CalcQueryImportance(const FPerceptionListener& Listener, const FVector& TargetLocation, const float SightRadiusSq) const;
};

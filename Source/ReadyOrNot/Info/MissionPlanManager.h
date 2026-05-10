// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "MissionPlanManager.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FPlanningMarkerDelegate, const struct FPlanningMarker&);
DECLARE_MULTICAST_DELEGATE_OneParam(FPlanningLineDelegate, const struct FPlanningLine&)
DECLARE_MULTICAST_DELEGATE_OneParam(FPlanningDeleteDelegate, int32 /** Replication ID */);

USTRUCT(BlueprintType)
struct FPlanningMarker : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 PlayerNumber = -1;

	UPROPERTY(BlueprintReadWrite)
	FVector2D Position;

	UPROPERTY(BlueprintReadWrite)
	int32 Floor;
	
	UPROPERTY(BlueprintReadWrite)
	FName Style;

	UPROPERTY(BlueprintReadWrite)
	float Rotation;

	void PreReplicatedRemove(const struct FPlanningMarkerArray& InArraySerializer);
	void PostReplicatedAdd(const struct FPlanningMarkerArray& InArraySerializer);
};

USTRUCT()
struct FPlanningMarkerArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FPlanningMarker> Items;

	FPlanningMarkerDelegate OnPlanningMarkerAdded;
	FPlanningDeleteDelegate OnPlanningMarkerRemoved;
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FPlanningMarker, FPlanningMarkerArray>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FPlanningMarkerArray> : public TStructOpsTypeTraitsBase2<FPlanningMarkerArray>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};

UENUM(BlueprintType)
enum class EPlanningLineTeam : uint8
{
	PLT_Gold,
	PLT_Red,
	PLT_Blue
};

USTRUCT(BlueprintType)
struct FPlanningLine : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 PlayerNumber = -1;
	
	UPROPERTY(BlueprintReadWrite)
	TArray<FVector2D> Points;

	UPROPERTY(BlueprintReadWrite)
	int32 Floor;
	
	UPROPERTY(BlueprintReadWrite)
	EPlanningLineTeam PlanningLineTeam;

	void PreReplicatedRemove(const struct FPlanningLineArray& InArraySerializer);
	void PostReplicatedAdd(const struct FPlanningLineArray& InArraySerializer);
};

USTRUCT()
struct FPlanningLineArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FPlanningLine> Items;

	FPlanningLineDelegate OnPlanningLineAdded;
	FPlanningDeleteDelegate OnPlanningLineRemoved;
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FPlanningLine, FPlanningLineArray>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FPlanningLineArray> : public TStructOpsTypeTraitsBase2<FPlanningLineArray>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};

USTRUCT()
struct FPlanningDrawingUpdate
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FVector2D> NewPoints;
};

USTRUCT()
struct FPlanningDrawing : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FVector2D> Points;

	UPROPERTY()
	int32 Floor = 0;

	UPROPERTY(NotReplicated)
	float Time = 0.0f;
};

USTRUCT()
struct FPlanningDrawingArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FPlanningDrawing> Items;
	
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FPlanningDrawing, FPlanningDrawingArray>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FPlanningDrawingArray> : public TStructOpsTypeTraitsBase2<FPlanningDrawingArray>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};

/**
 * 
 */
UCLASS()
class READYORNOT_API AMissionPlanManager : public AInfo
{
	GENERATED_BODY()

public:
	AMissionPlanManager();

	virtual void BeginPlay() override;

	void ClearPlan();
	
	static void AddPlayer(AReadyOrNotGameState* GameState, AReadyOrNotPlayerState* PlayerState);

	UFUNCTION()
	void OnMissionChanged() { ClearPlan(); }
	
	UFUNCTION(BlueprintPure, meta = (WorldContext = "WorldContextObject"))
	static AReadyOrNotPlayerState* GetPlayerStateFromPlanningNumber(const UObject* WorldContextObject, int32 Number);
	
	UPROPERTY(Replicated)
	FPlanningMarkerArray MarkerArray;

	UPROPERTY(Replicated)
	FPlanningLineArray LineArray;
};

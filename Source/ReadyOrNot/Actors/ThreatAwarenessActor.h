// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "ThreatAwarenessActor.generated.h"

UENUM(BlueprintType)
enum class EThreatLevel : uint8
{
	TL_None,
    TL_Low,
    TL_Medium,
    TL_High,
    TL_Extreme,
    TL_Stairs
};

USTRUCT(BlueprintType)
struct FExitRoute
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere)
	TArray<class ADoor*> Doors;

	// threats on the route (past the first door)
	UPROPERTY(VisibleAnywhere)
	TArray<class AThreatAwarenessActor*> ThreatsOnRoute;

	UPROPERTY(VisibleAnywhere)
	float PathCost = -1.0f;
	
	UPROPERTY(VisibleAnywhere)
	FVector Location;

	FExitRoute()
	{
		Doors = {};
		ThreatsOnRoute = {};
		Location = FVector::ZeroVector;
	}
	
	bool IsValidRoute() const;
	
	friend bool operator==(const FExitRoute& LHS, const FExitRoute& RHS)
	{
		return LHS.Doors == RHS.Doors;
	}
};

USTRUCT(BlueprintType)
struct FExitData
{
	GENERATED_USTRUCT_BODY()

	// used to determine how many unique exits out of a room
	UPROPERTY(VisibleAnywhere)
	TArray<FExitRoute> PossibleRoutes;

	UPROPERTY(VisibleAnywhere)
	FVector Location;
	
	FExitData()
	{
		PossibleRoutes = {};
		Location = FVector::ZeroVector;
	}

	bool IsAnyRouteValid()
	{
		for (FExitRoute r : PossibleRoutes)
		{
			if (r.IsValidRoute())
				return true;
		}
		return false;
	}

	friend bool operator==(const FExitData& LHS, const FExitData& RHS)
	{
		return LHS.PossibleRoutes == RHS.PossibleRoutes && LHS.Location == RHS.Location;
	}
};

USTRUCT(BlueprintType)
struct FLookAtPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntVector Location = FIntVector::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ADoor* LinkedDoor = nullptr;

	friend bool operator==(const FLookAtPoint& Lhs, const FLookAtPoint& Rhs)
	{
		return Lhs.Location == Rhs.Location;
	};
};

DECLARE_STATS_GROUP(TEXT("ThreatAwarenesActor"), STATGROUP_ThreatAwarenesActor, STATCAT_Advanced);

UCLASS(BlueprintType, NotBlueprintable, NotPlaceable, HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API AThreatAwarenessActor final : public AActor
{
	GENERATED_BODY()
	
public:	
	AThreatAwarenessActor();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneComponent* DefaultScene = nullptr;
	
	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBillboardComponent* BillboardComponent = nullptr;
	#endif
	
	void GenerateUniqueExits();
	void GeneratePossibleRoutes();

	UFUNCTION(CallInEditor, Category = "Tools")
	void RemoveAnyVisibleExits();
	
	bool UniqueExitsMatch(AThreatAwarenessActor* OtherThreat);

	UFUNCTION(BlueprintPure)
	bool GetUniqueExtis(TArray<ADoor*>& OutDoors) const { OutDoors = UniqueExits; OutDoors.Remove(nullptr); return OutDoors.Num() > 0; }

	void SetThreatLevel(EThreatLevel tl);
	
	void SetSpriteFromThreatLevel();
	
	void CheckIsOutside();
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE EThreatLevel GetThreatLevel() const { return ThreatLevel; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsDoorThreat() const { return DoorThreat != nullptr; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE ADoor* GetAttachedDoor() const { return DoorThreat; }

	UFUNCTION(CallInEditor, Category = "Tools")
	void GenerateLookAtPoints();
	
	void CalculateExits();

	UFUNCTION(BlueprintPure)
	bool HasSpecificExitDoor(ADoor* Door) const { return UniqueExits.Contains(Door); }

	UFUNCTION(BlueprintPure)
	bool GetRandomExitDoor(ADoor*& Door) const;
	
	UFUNCTION(BlueprintPure)
	bool HasExit() const;

	void TryDestroyIfInvalid();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	#if WITH_EDITOR
	virtual void PostInitializeComponents() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostActorCreated() override;
	virtual bool ShouldTickIfViewportsOnly() const override;
	void EditorTick(float DeltaTime);
	#endif
	
	UPROPERTY(VisibleInstanceOnly, Category = "Generated")
	EThreatLevel ThreatLevel = EThreatLevel::TL_Low;

public:
	UPROPERTY(VisibleAnywhere, Category = "Generated")
	TArray<FExitData> Exits;

	// how many doors etc are unique exits.. if 1 then this only has one way in and one way out
	UPROPERTY(VisibleAnywhere, Category = "Generated")
	TArray<ADoor*> UniqueExits = {};

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Generated")
	FName OwningRoom = NAME_None;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Generated", meta = (EditCondition = "DoorThreat != nullptr", EditConditionHides = true))
	ADoor* DoorThreat = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Generated", meta = (EditCondition = "DoorThreat != nullptr", EditConditionHides = true))
	bool bFrontDoorThreat = false;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Generated")
	bool bIsOutside = false;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Generated")
	TArray<AThreatAwarenessActor*> PathableThreatAwarenessActors;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Generated")
	TArray<FLookAtPoint> SwatLookAtPoints;
};

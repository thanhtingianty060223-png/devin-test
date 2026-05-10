// Copyright Void Interactive, 2023

#pragma once

#include "PatrolPoint.h"
#include "Structs.h"
#include "ThreatAwarenessActor.h"
#include "Door.h"
#include "WorldDataGenerator.generated.h"

enum class EThreatLevel : unsigned char;

class AThreatAwarenessActor;
// class ADoor;

USTRUCT()
struct FSavedSwatLookAtPoint
{
	GENERATED_BODY();

	UPROPERTY()
	FIntVector Location = FIntVector::ZeroValue;

	UPROPERTY()
	FVector LinkedDoorID = FVector::ZeroVector;
};

USTRUCT()
struct FSavedExitRoute
{
	GENERATED_BODY();

	UPROPERTY()
	FVector Location = FVector::ZeroVector;
	
	UPROPERTY()
	float PathCost = -1.0f;
	
	UPROPERTY()
	TArray<FVector> Doors;
	UPROPERTY()
	TArray<FVector> ThreatsOnRoute;
};

USTRUCT()
struct FSavedExitData
{
	GENERATED_BODY();

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	TArray<FSavedExitRoute> PossibleRoutes;
};

USTRUCT()
struct FSavedThreatAwarenessActor
{
	GENERATED_BODY();

	UPROPERTY()
	FVector UniqueID = FVector::ZeroVector;
	UPROPERTY()
	FVector Location = FVector::ZeroVector;
	UPROPERTY()
	EThreatLevel ThreatLevel = EThreatLevel::TL_None;

	UPROPERTY()
	TArray<FSavedExitData> Exits;
	UPROPERTY()
	TArray<FVector> UniqueExits;

	UPROPERTY()
	FString OwningRoom = "";

	UPROPERTY()
	FVector Door = FVector::ZeroVector;
	UPROPERTY()
	bool bFrontDoorThreat = false;

	UPROPERTY()
	bool bIsOutside = false;
	
	UPROPERTY()
	TArray<FSavedSwatLookAtPoint> SwatLookAtPoints;
	UPROPERTY()
	TArray<FVector> PathableThreats;
};

USTRUCT()
struct FSavedClearPoint
{
	GENERATED_BODY();

	UPROPERTY()
	FVector Location_Relative = FVector::ZeroVector;
	
	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	uint8 Stage = 0;
	
	UPROPERTY()
	EClearDirection Direction = EClearDirection::None;
	
	UPROPERTY()
	TArray<FString> CoverLandmarks;

	UPROPERTY()
	bool bHasLineOfSightToDoor = false;
};

USTRUCT()
struct FSavedStackUpActor
{
	GENERATED_BODY();

	UPROPERTY()
	FVector Location = FVector::ZeroVector;
	
	UPROPERTY()
	uint8 Depth = 0;

	UPROPERTY()
	ESquadPosition StackUpPosition = ESquadPosition::SP_NONE;

	UPROPERTY()
	FVector LinkedDoorID = FVector::ZeroVector;
};

USTRUCT()
struct FSavedDoorActor
{
	GENERATED_BODY();
	
	UPROPERTY()
	FVector UniqueID = FVector::ZeroVector;

	UPROPERTY()
	FSavedThreatAwarenessActor FrontDoorThreat = {};
	UPROPERTY()
	FSavedThreatAwarenessActor BackDoorThreat = {};
	
	UPROPERTY()
	TArray<FSavedClearPoint> FrontLeftClearPoints = {};
	UPROPERTY()
	TArray<FSavedClearPoint> FrontRightClearPoints = {};
	UPROPERTY()
	TArray<FSavedClearPoint> BackLeftClearPoints = {};
	UPROPERTY()
	TArray<FSavedClearPoint> BackRightClearPoints = {};
	
	UPROPERTY()
	TArray<FSavedThreatAwarenessActor> FrontThreats = {};
	UPROPERTY()
	TArray<FSavedThreatAwarenessActor> BackThreats = {};
	
	UPROPERTY()
	TArray<FSavedStackUpActor> FrontLeftStackups = {};
	UPROPERTY()
	TArray<FSavedStackUpActor> FrontRightStackups = {};
	UPROPERTY()
	TArray<FSavedStackUpActor> BackLeftStackups = {};
	UPROPERTY()
	TArray<FSavedStackUpActor> BackRightStackups = {};

	UPROPERTY()
	EDoorRoomPosition FrontRoomPosition = EDoorRoomPosition::Center;
	UPROPERTY()
	EDoorRoomPosition BackRoomPosition = EDoorRoomPosition::Center;

	UPROPERTY()
	bool bHasFrame = true;
	UPROPERTY()
	bool bCanIssueOrdersOnFrontSide = true;
	UPROPERTY()
	bool bCanIssueOrdersOnBackSide = true;
};

USTRUCT()
struct FSavedCoverActor
{
	GENERATED_BODY();

	UPROPERTY()
	FString Name = "";
	
	UPROPERTY()
	FTransform Transform = FTransform::Identity;
	
	UPROPERTY()
	FString CoverObjectName = "";
	
	UPROPERTY()
	FCoverRail CoverRail;
	
	UPROPERTY()
	EStandCoverType StandCoverType = EStandCoverType::Wall;
	UPROPERTY()
	ECrouchCoverType CrouchCoverType = ECrouchCoverType::Wall;
	
	UPROPERTY()
	FCoverDirection StandCoverDirection;
	UPROPERTY()
	FCoverDirection CrouchCoverDirection;
	
	UPROPERTY()
	int32 Index = 0;
	
	UPROPERTY()
	bool bIsCrouchOnlyCover = false;
	UPROPERTY()
	bool bOverrideCoverType = false;
};

USTRUCT()
struct FSavedRoomData	
{
	GENERATED_BODY();

	UPROPERTY()
	FName Name = NAME_None;
	
	UPROPERTY()
	FIntVector Location = FIntVector::ZeroValue;
	
	UPROPERTY()
	FVector RootDoorID = FVector::ZeroVector;

	UPROPERTY()
	TArray<FVector> Threats;

	UPROPERTY()
	TArray<FVector> AdditionalRootDoors;
	
	UPROPERTY()
	TArray<FName> ConnectingRooms;
};

UCLASS()
class READYORNOT_API UWorldGenSave final : public USaveGame
{
	GENERATED_BODY()

public:
	UWorldGenSave() {}

	UPROPERTY()
	TArray<FSavedThreatAwarenessActor> SavedThreatAwarenessActors;
	
	UPROPERTY()
	TArray<FSavedDoorActor> SavedDoorActors;
	
	UPROPERTY()
	TArray<FSavedCoverActor> SavedCoverActors;
	
	UPROPERTY()
	TArray<FSavedRoomData> SavedRooms;
};

UCLASS(BlueprintType, NotBlueprintable, HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API AWorldDataGenerator final : public AInfo
{
	GENERATED_BODY()

public:
	AWorldDataGenerator();

	#if WITH_EDITOR
	virtual void CheckForErrors() override;
	#endif
	
	static AWorldDataGenerator* Get(UWorld* World);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UPROPERTY()
	USceneComponent* SceneComponent = nullptr;

	UFUNCTION(BlueprintCallable, Exec)
	void GenerateWorld();

	UFUNCTION(CallInEditor, Category = "Room System")
	void GenerateRooms();
	
	UPROPERTY()
	TArray<ADoor*> VisitedDoors;

	uint8 RoomIndex = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status (Read Only)")
	bool bHasWorldEverBeenGenerated = false;
	
	//UFUNCTION(CallInEditor)
	void GenerateWebbedBreachPoints();

	void GetAllReachableDoors(FVector Location, TArray<ADoor*>& OutDoors);

	//UFUNCTION(CallInEditor, Category = "Data Generation")
	void GenerateSpawnHidingSpots();

	void GetPathableThreatAwarenessPoints(AThreatAwarenessActor* Threat, TArray<AThreatAwarenessActor*>& PathableThreatAwarenessActors);
	void GetVisibleThreatAwarenessPoints(AThreatAwarenessActor* Threat, TArray<AThreatAwarenessActor*>& VisibleThreatAwarenessActors);

	//UFUNCTION(CallInEditor, Category = "Data Generation")
	void GenerateSwatLookAtPointsForEachThreat();

	void GenerateSwatLookAtPoint(AThreatAwarenessActor* A);

	AThreatAwarenessActor* GetSwatLookAtThreats(AThreatAwarenessActor* Threat, TArray<AThreatAwarenessActor*> NotLookAtThese);

	UFUNCTION(CallInEditor, Category = "Verification")
	void ClearNullReferences();

	UFUNCTION(CallInEditor, Category = "Cover Generation")
	void GenerateCoverPoints();
	
	UFUNCTION(CallInEditor, Category = "Cover Generation")
	void DestroyCoverPoints();

	UFUNCTION(CallInEditor, Category = "Threat Generation")
	void CalculateAllExits();

	UPROPERTY()
	TArray<AThreatAwarenessActor*> IgnoredExitThreats;

	bool bIsGenerating = false;

	void GetOrCreateIgnoredExitThreats(TArray<AThreatAwarenessActor*>& OutThreats);

	float GetPathLength(FVector StartLocation, FVector EndLocation);

	TArray<class ANavMeshBoundsVolume*> GetNavMeshBounds();

	int32 GetThreatAwarenessActorCount();
	AThreatAwarenessActor* GetNearestThreat(FVector Location, bool bRequirePath = true, bool bExcludeDoorThreats = false);

	UFUNCTION(CallInEditor, Category = "Threat Generation")
	void GenerateWorldThreatAwarenessActors();
	UFUNCTION(CallInEditor, Category = "Threat Generation")
	void DestroyAllThreats();
	//UFUNCTION(CallInEditor)
	void CleanUpOverlappingThreats();
	
	void GenerateDoorThreatAwarenessActors(ADoor* Door); 
	bool IsSuitableSpawnLocationForThreatAwareness(FVector Location);

	UFUNCTION(CallInEditor, Category = "Threat Generation")
	void GenerateAllDoorThreatAwarenessActors();

	void GenerateDoorPositions(ADoor* Door);
	
	void GenerateDoorClearPointsV2(ADoor* Door);

	TArray<FClearPoint> FindClearPointsInDirection(const TArray<FClearPoint>& ClearPoints, FVector DoorLocation, FVector ForwardDirection, FVector RightDirection, float MaxHorizontalDistance = FLT_MAX, FVector2D MaxRightDotRange = FVector2D(-0.25f, 0.5f), EDoorRoomPosition RoomPosition = EDoorRoomPosition::Center) const;
	TArray<FClearPoint> IncreaseClearPointResolution(const TArray<FClearPoint>& ClearPoints) const;
	void LinkNearbyCoverLandmarksToClearPoints(ADoor* Door, TArray<FClearPoint>* RightClearPoints, TArray<FClearPoint>* LeftClearPoints);

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Status (Read Only)")
	bool bDoorwaysBlocked = false;

	UFUNCTION(CallInEditor, Category = "Clear Point Generation")
	void GenerateAllDoorClearPoints();
	
	UFUNCTION(CallInEditor, Category = "Doors")
	void GenerateAllDoorPositions();
	UFUNCTION(CallInEditor, Category = "Doors")
	void BlockAllDoorways();
	UFUNCTION(CallInEditor, Category = "Doors")
	void UnblockAllDoorways();

	void GatherAllThreatsBetweenDistance(TArray<AThreatAwarenessActor*> InThreats, float MinDist, float MaxDist, FVector TestLocation, TArray<AThreatAwarenessActor*>& OutThreats);
	AThreatAwarenessActor* GatherLeftMostThreat(TArray<AThreatAwarenessActor*> InThreats, FVector StartLocation, FVector ForwardVector, bool bFront, ADoor* Door, float MaxRightMostDist = 0.0f, AThreatAwarenessActor* PrevClearPoint = nullptr);
	AThreatAwarenessActor* GatherRightMostThreat(TArray<AThreatAwarenessActor*> InThreats, FVector StartLocation, FVector ForwardVector, bool bFront, ADoor* Door, float MaxLeftMostDist = 0.0f, AThreatAwarenessActor* PrevClearPoint = nullptr);
	AThreatAwarenessActor* GatherAnyThreat(TArray<AThreatAwarenessActor*> InThreats, bool bFront, ADoor* Door);

	void TrimFarAwayStages(TArray<AThreatAwarenessActor*> InClearPoints, TArray<AThreatAwarenessActor*>& OutClearPoints);
	AThreatAwarenessActor* GetLastValidThreatInArray(TArray<AThreatAwarenessActor*> InThreats);

	UFUNCTION(CallInEditor, Category = "Stack Up Generation")
	void GenerateStackUpPoints();

	UFUNCTION(CallInEditor, Category = "Stack Up Generation")
	void DestroyAllStackups();

	void GenerateDoorStackUps(ADoor* Door);
	void DeleteDoorStackUps(ADoor* Door);
	void GenerateDoorStackUpsV2(ADoor* Door);
	
	void GenerateDoorStackUps_Center(ADoor* Door, bool bFront, bool bLeft, float ForwardOffset);
	void GenerateDoorStackUps_Hallway(ADoor* Door, bool bFront, bool bLeft, float ForwardOffset);
	void GenerateDoorStackUps_Corner(ADoor* Door, bool bFront, bool bLeft, float ForwardOffset);

	void TryGenerateStackUpLocations(ADoor* Door, bool bFront, bool bLeft,TArray<FVector>& OutLocations);

	FVector FindStackupPositionFromLocationArray(TArray<FVector> InLocations, ESquadPosition SquadPosition, ADoor* Door, bool bFront);

	void RemoveStackupsWithinDist(TArray<FVector> InLocations, FVector TestLocation, float Dist, TArray<FVector>& OutLocation);

	UPROPERTY(VisibleInstanceOnly, Category = "Room System", meta = (TitleProperty = "Name"))
	FRoomGenData RoomData;

	// find and link main doors and subdoors
	//UFUNCTION(CallInEditor)
	void LinkSubdoors();

	// if turned on we will loosen the restrictions to allow better gen in tight areas
	int32 LoosenedStackupRestrictionAmount = 0.0f;
	bool IsSuitablePositionForStackUpPoint(FVector Location, ADoor* Door);
	class AStackUpActor* SpawnStackUpActorAtLocation(FVector Location, ADoor* Door, ESquadPosition SquadPosition, bool bFront, bool bLeft, uint8 Depth);

	void AddAnyThreatsThatContainOurs(AThreatAwarenessActor* Threat, TArray<AThreatAwarenessActor*>& VisibleThreatAwarenessActors);
	
	//UFUNCTION(CallInEditor)
	void SortAndTrimMaxVisibleThreats(const TArray<AThreatAwarenessActor*>& InThreats);

	int32 ThreatIndex = 0;

	UFUNCTION(CallInEditor, Category = "Verification")
	void ReportAllUnreachableSpawns();

	void PlacePatrolPointsOnAllThreats(const TArray<AThreatAwarenessActor*>& InThreats);

	void RemoveAllOverlappingThreats(TArray<AThreatAwarenessActor*> InThreats);
	
	TArray<FPatrolPoint> AllPatrolPoints;

	bool WriteGenerationToFile(bool bForce = false);
	void LoadGenerationFromFile();

private:
	bool Internal_ProjectPointToNav(FVector Point, FVector& OutLocation, FVector Extent = FVector(40.0f, 40.0f, 150.0f)) const;
	bool Internal_PointHasLineOfSightToDoor(FVector Point, ADoor* Door, bool bFront) const;
	bool Internal_PointHasLineOfSightTo(FVector Point, FVector OtherPoint) const;
	bool Internal_FindPath(FVector From, FVector To, float MaxPathLength = 500.0f, TArray<FNavPathPoint>* OutPathPoints = nullptr) const;
	bool Internal_FindPath_RoomGen(FVector From, FVector To) const;
	
	ADoor* FindDoorFromID(FVector ID) const;
	AThreatAwarenessActor* FindThreatFromID(FVector ID) const;
	AActor* FindActorFromName(FString Name) const;
	AStackUpActor* FindStackUpActorFromName(FString Name) const;
	ACoverLandmark* FindLandmarkActorFromName(FString Name) const;
	class ACoverPoint* FindCoverActorFromName(FString Name) const;
	
	FClearPoint MakeClearPointFromSavedClearPoint(const FSavedClearPoint& s) const;
};

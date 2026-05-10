 // Copyright Void Interactive, 2021

#pragma once

#include "GameFramework/Actor.h"
#include "Data/AIData.h"
#include "AISpawn.generated.h"

USTRUCT(BlueprintType)
struct FActivityRoute
{
	GENERATED_BODY()

	// The amount of time, in seconds, the AI will perform this world building activity for (excluding entry/exit time)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TimeDoingActivity = 15.0f;

	// The world building actor reference for this route
	// Note: Place in a world building placement actor in the level and link it here
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class AWorldBuildingPlacementActor* WorldBuildingPlacementActor = nullptr;

	// If we are a female, should we be allowed to perform this world building activity for this route
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bAllowFemale = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bMoveOnly = false;
};

USTRUCT(BlueprintType)
struct FActivityRouteCollection
{
	GENERATED_BODY()

	FActivityRoute* GetActivityAtIndex(const int32 InIndex)
	{
		if (ActivityRoutes.IsValidIndex(InIndex))
		{
			return &ActivityRoutes[InIndex];
		}
	
		return nullptr;
	}
	
	FActivityRoute* GetNextActivity()
	{
		int32 curIdx = ActivityIdx;
		if (ActivityRoutes.IsValidIndex(++curIdx))
		{
			return &ActivityRoutes[curIdx];
		}

		if (ActivityRoutes.IsValidIndex(0))
		{
			return &ActivityRoutes[0];
		}

		return nullptr;
	}
	
	const FActivityRoute* GetCurrentActivity()
	{
		if (ActivityRoutes.IsValidIndex(ActivityIdx))
		{
			return &ActivityRoutes[ActivityIdx];
		}
	
		return nullptr;
	}
	
	const FActivityRoute* GetLastActivity()
	{
		if (ActivityRoutes.Num() == 0)
			return nullptr;
		
		return &ActivityRoutes.Last();
	}

	// The list of world building routes to be taken
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FActivityRoute> ActivityRoutes;

	// After performing all world building routes, should we return to our original spawn location?
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bReturnToOriginalSpot = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bSpawnAtFirstRoute = false;

	// After performing all routes, start a cooldown with the specified time
	// Set to 0.0 for no cooldown
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0f, ClampMax = 300.0f))
	float Cooldown = 15.0f;

	// The current index of the route
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	uint8 ActivityIdx = 0;
};

USTRUCT(BlueprintType)
struct FSpawnData
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly)
	FName Name = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDataTableRowHandle SpawnedAI;

	// If true, the AI will spawn unarmed. Only applies to suspects
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bForceNoWeapon = false;
	
	// If true, the AI spawns in brain-dead, unable to move or make decisions. Useful for debugging purposes
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDeactivated = false;

	// If specified, will override the weapon given. Only applies to suspects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bForceNoWeapon"))
	TSubclassOf<ABaseWeapon> ForceWeaponOverride;

	// If specified, will override the body armour given. Only applies to suspects
	// Note: The list of available armour options can be found in the SuspectArmourDataTable
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ForceBodyArmourOverride = NAME_None;

	// The list of tags to be spawned in with. Can be used by general gameplay code
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> SpawnWithTags;

	// World building activity route setup settings
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "SpawnInLandmark == nullptr"))
	FActivityRouteCollection ActivityRouteCollection;

	// Should we spawn at the specified landmark at the start of the game?
	// If set, world building will be disabled
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<ACoverLandmark> SpawnInLandmark = nullptr;

	friend bool operator==(const FSpawnData& Lhs, const FSpawnData& Rhs)
	{
		return Lhs.Name == Rhs.Name;
	};
};

UCLASS(BlueprintType, NotBlueprintable, HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API AAISpawn final : public AActor
{
	GENERATED_BODY()
	
public:	
	AAISpawn();
	
	UFUNCTION(BlueprintCallable, Category = "AI Spawner")
	bool DoSpawn();

	UFUNCTION(BlueprintPure, Category = "AI Spawner")
	static ETeamType GetSpawningTeamType(const FSpawnData& Sd);
	
	bool GetCivilianSpawnData(TArray<const FSpawnData*>& OutSpawnData) const;
	bool GetSuspectSpawnData(TArray<const FSpawnData*>& OutSpawnData) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	uint8 bEnabled : 1;
	
	// Can this spawn point ever be considered for random explosive vest suspects/civilians?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup", meta = (EditCondition = "bEnabled"))
	bool bAllowExplosiveVestSpawn = true;

	// The probability that this spawn will be chosen
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup", meta = (EditCondition = "bEnabled", ClampMin = 0.001f, ClampMax = 1.0f))
	float SpawnChance = 1.0f;

	// The Group ID of this spawn. Only relevant when "UseGroupSpawns" is set to true in AILevelData.ini
	//
	// Usage Example: Inside AILevelData.ini
	//
	// UseSpawnGroups = true
	// MinSuspects_Group0 = 2
	// MaxSuspects_Group0 = 5
	// MinSuspects_Group1 = 0
	// MaxSuspects_Group1 = 3
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup", meta = (EditCondition = "bEnabled"))
	uint8 GroupID = 0;

	FSpawnData SpawnData;

	// Array of AI spawn settings that will be randomly chosen
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "bEnabled", TitleProperty = "Name"))
	TArray<FSpawnData> SpawnArray;

	// Override the spawning weapon with this weapon class. Can be left empty
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overrides", meta = (EditCondition = "bEnabled"))
	TSubclassOf<ABaseWeapon> GlobalWeaponOverride;
		
	// Override the default archetype with this one. Can be left empty. Useful for debugging purposes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Overrides", meta = (EditCondition = "bEnabled"))
	UAIArchetypeData* ArchetypeOverride = nullptr;

	// The character that was spawned in
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status (Read Only)")
	ACyberneticCharacter* SpawnedCharacter = nullptr;
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override; 
	
	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	virtual void PostLoad() override;

	void EditorTick(float DeltaTime);
	#endif

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class USceneComponent* DefaultScene = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UArrowComponent* SpawnDirection = nullptr;
	
	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UBillboardComponent* BillboardComponent = nullptr;

	// When moving this actor around the level, make sure it stays within the navigation bounds in order
	// to prevent out of bounds/unreachable spawns.
	// If true, disable nav mesh bounds checking so that when moving around, it isn't confined to the bounds of a nav mesh
	UPROPERTY(EditInstanceOnly, Category = "Tools", meta = (EditCondition = "bEnabled"))
	uint8 bDisableBoundsCheck : 1;
	
	void UpdateTexture();
	#endif
};

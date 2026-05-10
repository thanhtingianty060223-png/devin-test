// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "SpawnGenerator.generated.h"

UCLASS(HideCategories=("Rendering", "Collision", "Input", "Actor", "HLOD", "LOD", "Cooking"))
class READYORNOT_API ASpawnGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpawnGenerator();

	UFUNCTION(BlueprintPure, Category = "Spawn Generator")
    TArray<FVector> GetNodes() const { return Nodes; }
	
	UFUNCTION(BlueprintPure, Category = "Spawn Generator")
	const TArray<APlayerStart*>& GetAllPlayerStarts() const { return PlayerStarts; }
	
	UFUNCTION(BlueprintPure, Category = "Spawn Generator")
	ETeamType GetSpawnTeam() const { return SpawnTeam; }

protected:
	void Tick(float DeltaSeconds) override;
	
	#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	void PostEditUndo() override;
	void K2_DestroyActor() override;
	bool ShouldTickIfViewportsOnly() const override;
	#endif

	UFUNCTION(CallInEditor, Category = "Nodes")
	void RefreshSpawns();

	UFUNCTION(CallInEditor, Category = "Nodes")
	void UpdatePlayerStartTags();

	UFUNCTION(CallInEditor, Category = "Nodes")
	void SelectAll();
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Spawn Generator")
	USceneComponent* SceneComponent = nullptr;

	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, Category = "Spawn Generator")
	class UBillboardComponent* BillboardComponent = nullptr;
	#else
	UClass* BillboardComponent = nullptr;
	#endif

	UPROPERTY(EditInstanceOnly, Category = "Nodes")
	ETeamType SpawnTeam = ETeamType::TT_NONE;
	
	UPROPERTY(EditInstanceOnly, Category = "Nodes", meta = (ClampMin = 1, ClampMax = 100))
	uint16 Rows = 5;

	UPROPERTY(EditInstanceOnly, Category = "Nodes", meta = (ClampMin = 1, ClampMax = 100))
	uint16 Columns = 5;

	UPROPERTY(EditInstanceOnly, Category = "Nodes")
	float RowSpacing = 100.0f;

	UPROPERTY(EditInstanceOnly, Category = "Nodes")
	float ColumnSpacing = 100.0f;

	UPROPERTY(EditInstanceOnly, Category = "Nodes")
	uint8 bShowNodes : 1;

private:
	void CreateNodes();
	void ClearNodes();
	void UpdatePlayerStartLocations();
	void SpawnPlayerStartActors();
	void DestroyAllPlayerStartActors();
	
	TArray<FVector> Nodes;

	UPROPERTY()
	TArray<APlayerStart*> PlayerStarts;
	
	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObjectsInContainer
	TArray<APlayerStart*> PlayerStarts_NonUproperty; // Needed for Undo functionality to work
};

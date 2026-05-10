// Void Interactive, 2020

#pragma once

#include "Engine/TriggerBox.h"
#include "BuildingTrigger.generated.h"

USTRUCT(BlueprintType)
struct FBuildingFloor
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data", meta = (ClampMin = 0))
    int32 Number = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data", meta = (ClampMin = 0.0f))
	float Height = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
    FVector Location = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	FVector Extent = FVector::OneVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	FText Name = FText::FromString("Floor");
};

/**
 * Used to define a building with data
 */
UCLASS(HideCategories=("Rendering", "Collision", "Input", "Actor", "HLOD", "LOD", "Cooking", "Physics", "Navigation", "Mobile", "Asset User Data"))
class READYORNOT_API ABuildingTrigger : public ATriggerBox
{
	GENERATED_BODY()

public:
	ABuildingTrigger();

	UFUNCTION(BlueprintPure, Category = "Building Trigger")
	bool IsActorOnFloor(AActor* Actor, int32 FloorNumber) const;

	UFUNCTION(BlueprintPure, Category = "Building Trigger")
    FVector GetFloorLocation(int32 FloorNumber) const;

	UFUNCTION(BlueprintPure, Category = "Building Trigger")
    int32 GetFloorNumberFromActorLocation(AActor* Actor) const;

protected:
	void BeginPlay() override;
	void Tick(float DeltaTime) override;

	#if WITH_EDITOR
	void PostEditMove(bool bFinished) override;
	bool ShouldTickIfViewportsOnly() const override;
	#endif
	
	UFUNCTION(BlueprintNativeEvent, Category = "Events")
			void OnBuildingEnter(AActor* OverlappedActor, AActor* OtherActor);
    virtual void OnBuildingEnter_Implementation(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION(BlueprintNativeEvent, Category = "Events")
			void OnBuildingExit(AActor* OverlappedActor, AActor* OtherActor);
    virtual void OnBuildingExit_Implementation(AActor* OverlappedActor, AActor* OtherActor);
	
	UFUNCTION(CallInEditor, Category = "Setup")
	void GenerateFloors();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (ClampMin = 0))
	int32 NumberOfFloors = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	uint8 bAuto : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (EditCondition = "!bAuto"))
	uint8 bUniformFloorSpacing : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (ClampMin = 0.0f, EditCondition = "!bAuto && bUniformFloorSpacing"))
	float SpacingBetweenFloors = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (ClampMin = 0.0f, EditCondition = "!bAuto && !bUniformFloorSpacing"))
    TArray<float> SpacingPerFloor;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FText BuildingName = FText::FromString("Building");
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TMap<int32, FString> FloorNumberToFloorName;

	#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	uint8 bVisualizeFloors : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	uint8 bVisualizeFloorMidPoints : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	uint8 bVisualizeMinMaxExtents : 1;
	#endif

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Debug")
	TArray<FBuildingFloor> GeneratedFloors;
};

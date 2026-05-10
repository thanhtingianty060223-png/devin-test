// Copyright Void Interactive, 2023

#pragma once

#include "GameFramework/Actor.h"
#include "RoomVisualizer.generated.h"

UENUM(BlueprintType)
enum class ERoomSize : uint8
{
	Small,
	Medium,
	Large,
	Corridor
};

UCLASS(BlueprintType, NotBlueprintable, NotPlaceable, HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API ARoomVisualizer final : public AActor
{
	GENERATED_BODY()
	
public:	
	ARoomVisualizer();
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneComponent* DefaultScene = nullptr;
	
	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBillboardComponent* BillboardComponent = nullptr;
	#endif

	FORCEINLINE ERoomSize GetRoomSize() const { return Size; }

	UPROPERTY(VisibleInstanceOnly)
	FName OwningRoom = NAME_None;
	
	UPROPERTY(VisibleInstanceOnly)
	TArray<ADoor*> Doors;
	
	UPROPERTY(VisibleInstanceOnly)
	TArray<AThreatAwarenessActor*> Threats;
	
	UPROPERTY(VisibleInstanceOnly, EditFixedSize)
	TArray<ARoomVisualizer*> ConnectingRooms;

	void SetRoomSize(const ERoomSize& InSize);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
	void EditorTick(float DeltaTime);
	#endif

	UPROPERTY(VisibleInstanceOnly)
	ERoomSize Size = ERoomSize::Small;

private:
	#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UTexture2D* SmallRoomIcon = nullptr;
	UPROPERTY()
	UTexture2D* MediumRoomIcon = nullptr;
	UPROPERTY()
	UTexture2D* LargeRoomIcon = nullptr;
	UPROPERTY()
	UTexture2D* CorridorRoomIcon = nullptr;
	#endif
};

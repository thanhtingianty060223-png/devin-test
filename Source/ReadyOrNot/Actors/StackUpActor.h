// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "StackUpActor.generated.h"

UCLASS(BlueprintType, NotBlueprintable, NotPlaceable, HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API AStackUpActor final : public AActor
{
	GENERATED_BODY()
	
public:
	AStackUpActor();
	
	void SetSquadPosition(ADoor* NewLinkedDoor, ESquadPosition SquadPosition);
	
	FORCEINLINE ESquadPosition GetSquadPosition() const { return StackUpPosition; }
	FORCEINLINE ADoor* GetLinkedDoor() const { return LinkedDoor; }
	
	FColor GetDebugColor() const;

	void EnableNavBlocker();
	void DisableNavBlocker();
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneComponent* DefaultScene = nullptr;
	
	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UBillboardComponent* BillboardComponent = nullptr;
	#endif
	
	UPROPERTY(VisibleAnywhere, Category = "Generated")
	uint8 Depth = 0;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Status (Read Only)")
	AController* OccupiedBy = nullptr;

protected:
	virtual void Tick(float DeltaTime) override;

	#if WITH_EDITOR
	void EditorTick(float DeltaTime);
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	#endif

	UPROPERTY(VisibleAnywhere, Category = "Generated")
	ESquadPosition StackUpPosition = ESquadPosition::SP_NONE;

	UPROPERTY(VisibleAnywhere, Category = "Generated")
	ADoor* LinkedDoor = nullptr;
};

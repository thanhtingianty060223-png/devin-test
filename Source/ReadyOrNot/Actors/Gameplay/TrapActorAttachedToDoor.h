// � Void Interactive, 2021

#pragma once

#include "TrapActor.h"
#include "TrapActorAttachedToDoor.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API ATrapActorAttachedToDoor : public ATrapActor
{
	GENERATED_BODY()

public:
	ATrapActorAttachedToDoor();
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Replicated, Category = "Trap Attached To Door")
	class ADoor* AttachedToDoor = nullptr;

	virtual bool CanCutWire() const override;

	virtual void OnTrapDisarmed_Implementation(AReadyOrNotCharacter* DisarmedBy = nullptr) override;
	virtual void OnTrapTriggered_Implementation(AReadyOrNotCharacter* TriggeredBy) override;

	virtual bool CanDisarmTrap() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;

	#if WITH_EDITOR
	virtual void PostEditUndo() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif

	virtual bool ShouldConsiderTrapCollisionFor(AReadyOrNotCharacter* InCharacter) override;

	void GenerateCableMesh();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Cable")
	float WireYPosition = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trap|Cable")
	FTransform CableTransform;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Trap|Cable")
	TArray<class USplineMeshComponent*> CableMeshComponents;
	
	UPROPERTY(BlueprintReadOnly, Category = "Trap|Cable")
	float CurveStrength = 100.0f;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Trap|Cable")
	float MappedSplineLocation = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Trap|Cable")
	uint8 bChunk1Destroyed : 1;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Trap|Cable")
	uint8 bChunk2Destroyed : 1;
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Trap|Cable")
	uint8 bSubdoorChunk1Destroyed : 1;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Trap|Cable")
	uint8 bSubdoorChunk2Destroyed : 1;
};
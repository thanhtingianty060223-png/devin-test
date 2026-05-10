// Void Interactive, 2020

#pragma once

#include "ActorSpawnPoint.h"
#include "EvidenceSpawnPoint.generated.h"

UCLASS(HideCategories=("Rendering", "Collision", "Input", "Actor", "HLOD", "LOD", "Cooking"))
class READYORNOT_API AEvidenceSpawnPoint : public AActorSpawnPoint
{
	GENERATED_BODY()
	
public:
	AEvidenceSpawnPoint();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<AEvidenceActor> EvidenceActorClass = nullptr;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup")
	class ASplineTrigger_Incrimination* EvidenceSearchArea = nullptr;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup")
	class ABuildingTrigger_Incrimination* EvidenceBuilding = nullptr;

protected:
	void BeginPlay() override;
};

// Void Interactive, 2020

#pragma once

#include "Actors/EvidenceExtractionDevice.h"
#include "EvidenceExtractionDevice_Incrim.generated.h"

UCLASS()
class READYORNOT_API AEvidenceExtractionDevice_Incrim : public AEvidenceExtractionDevice
{
	GENERATED_BODY()

public:
	AEvidenceExtractionDevice_Incrim();

	bool CanStartExtraction() const override;
	bool CanCollectEvidence() const override;
	bool IsExtracting() const override;
	bool HasEvidenceToExtract() const override;

protected:
	void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UObjectiveMarkerComponent* ObjectiveMarkerComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
    class UObjectiveMarkerComponent* ObjectiveMarkerComponent_WayPoint = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
    class UMapActorComponent* MapActorComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup", meta = (DisplayAfter = "EvidenceExtractionTime"))
	FString MapSectionName;
};

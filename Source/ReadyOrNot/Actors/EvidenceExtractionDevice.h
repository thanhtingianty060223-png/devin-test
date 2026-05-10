// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "EvidenceExtractionDevice.generated.h"

UCLASS()
class READYORNOT_API AEvidenceExtractionDevice : public AActor, public IUseabilityInterface
{
	GENERATED_BODY()
	
public:	
	AEvidenceExtractionDevice();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Evidence Extraction Device")
	void TryExtractEvidence(APlayerCharacter* EvidencePossessor);
	
	UFUNCTION(BlueprintPure, Category = "Evidence Extraction Device")
	virtual bool CanStartExtraction() const;
	
	UFUNCTION(BlueprintPure, Category = "Evidence Extraction Device")
	virtual bool CanCollectEvidence() const;
	
	UFUNCTION(BlueprintPure, Category = "Evidence Extraction Device")
	virtual bool IsExtracting() const;
	
	UFUNCTION(BlueprintPure, Category = "Evidence Extraction Device")
	virtual bool HasEvidenceToExtract() const;

protected:
	void Tick(float DeltaTime) override;

	virtual void TryExtractEvidence_Implementation(APlayerCharacter* EvidencePossessor);
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* StaticMeshComponent = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	class UInteractableComponent* InteractableComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	float EvidenceExtractionTime = 30.0f;

	// Begin IUseabilityInterface
	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	// End IUseabilityInterface
};

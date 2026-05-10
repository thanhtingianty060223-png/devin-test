// Copyright Void Interactive, 2023

#pragma once

#include "GameFramework/Actor.h"
#include "Components/InteractableComponent.h"
#include "Interfaces/Reportable.h"
#include "Interfaces/Securable.h"
#include "ReportableActor.generated.h"

UENUM(BlueprintType)
enum EVolumeShape
{
	Box,
	Sphere,
	Capsule
};

UCLASS(Blueprintable, HideCategories = ("Replication", "Input", "LOD", "Cooking", "Rendering", "Collision"), AutoExpandCategories=("Actor"))
class READYORNOT_API AReportableActor : public AActor, public IGetFriendlyName, public IUseabilityInterface, public IReportable, public ISecurable
{
	GENERATED_BODY()

public:
	AReportableActor();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PostInitializeComponents() override;
	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(ReplicatedUsing=OnRep_bReportableEnabled, EditAnywhere, Category = "Reportable")
	bool bReportableEnabled = true;

	UPROPERTY(EditDefaultsOnly, Category = "Reportable")
	bool bHasBeenReported = false;

	/** If no report objective exists with the same tag as this reportable on beginplay, disable this reportable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reportable")
	bool bDisableOnNoMatchingObjective = true;

	/** If the objective with matching tag to this reportable is hidden, this reportable will be disabled. Reportable is enabled when objective is unhidden */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reportable")
	bool bDisableWhileMatchingObjectiveHidden = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reportable")
	FText ReportableName = FText::FromString("Reportable");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reportable")
	FString ReportVoiceLine = "[CALL]CollectEvidence";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reportable")
	bool bTocResponseOnReport = true;	

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReported, AReportableActor*, ReportableActor);
	UPROPERTY(BlueprintAssignable, Category = Events)
	FOnReported OnReported;

	/** Implementation Note:
	 *	Not inheriting from AVolume or using a brush component as it makes it difficult to derive from in blueprints.
	 *	Mimicking a brush component by swapping between a box/sphere/capsule component when changed in details panel.
	 *	Bonus of doing it this way is that the shape can be modified at runtime as well.
	**/
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UShapeComponent* ShapeComponent;

	UPROPERTY(EditAnywhere, Category = "Reportable")
	TEnumAsByte<EVolumeShape> Shape = Box;

	UPROPERTY(EditAnywhere, Category = "Reportable", meta = (EditCondition = "Shape == EVolumeShape::Box", EditConditionHides))
	FVector BoxExtents = FVector(200.f, 200.f, 200.f);

	UPROPERTY(EditAnywhere, Category = "Reportable", meta = (EditCondition = "Shape == EVolumeShape::Sphere || Shape == EVolumeShape::Capsule", EditConditionHides))
	float Radius = 50.f;

	UPROPERTY(EditAnywhere, Category = "Reportable", meta = (EditCondition = "Shape == EVolumeShape::Capsule", EditConditionHides))
	float ZHeight = 200.f;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneComponent;
	

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UInteractableComponent* InteractableComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UScoringComponent* ScoringComponent;

	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
	virtual bool CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const override;

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
	void DisableReportableIfNoMatchingObjective();

	UFUNCTION()
	void SetReportableEnabled(bool bEnable);

	// IReportable implementation
	virtual bool CanReportNow_Implementation() override;
	virtual FString GetSpeechTypeForReport_Implementation() override;
	virtual void ReportToTOC_Implementation(class AReadyOrNotCharacter* Reporter, bool bPlayAnimation = true) override;

	virtual bool CanBeSecured_Implementation() const override;
	virtual bool CanBeSecuredByTrailers_Implementation() const override;
	virtual bool IsSecured_Implementation() const override;
	virtual void Secure_Implementation(AReadyOrNotCharacter* InInstigator) override;
	virtual FVector GetLocation_Implementation() const override;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBillboardComponent* BillboardComponent;
#endif

private:
	UFUNCTION()
	void OnRep_bReportableEnabled();
};

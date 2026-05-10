// Copyright Void Interactive, 2023

#pragma once

#include "Enums.h"
#include "Components/SkeletalMeshComponent.h"
#include "ItemVisualizationComponent.generated.h"

UCLASS()
class READYORNOT_API UItemVisualizationComponent final : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	UItemVisualizationComponent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EItemVisualizationType VisualizationType = EItemVisualizationType::IVT_None;
	
	void DisableItemVisualizationComponent();
	void UpdateItemVisualizationComponent();

	FORCEINLINE class AReadyOrNotCharacter* GetOwningCharacter() const { return Cast<AReadyOrNotCharacter>(GetOwner()); }
	
protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	ABaseItem* BasedOfItem = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	UStaticMeshComponent* MagazineComp = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attachments")
	class USkeletalMeshComponent* ScopeAttachment = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attachments")
	class USkeletalMeshComponent* MuzzleAttachment = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attachments")
	class USkeletalMeshComponent* UnderbarrelAttachment = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Attachments")
	class USkeletalMeshComponent* OverbarrelAttachment = nullptr;

	void CreateAttachmentFromClass(USkeletalMeshComponent*& SkeletalMeshComponent, UWeaponAttachment* WeaponAttachment);
};
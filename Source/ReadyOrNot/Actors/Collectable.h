// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Collectable.generated.h"

UCLASS(Blueprintable, Abstract)
class READYORNOT_API ACollectable : public AActor, public IUseabilityInterface
{
	GENERATED_BODY()

public:
	ACollectable();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	static TArray<ACollectable*> GetAllCollectables();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true))
	FText ItemDescription;

	UPROPERTY(EditAnywhere)
	TArray<FName> RequiredTags;

	UPROPERTY(EditAnywhere, Instanced)
	TArray<class UProgressionRequirement*> RequiredProgression;

protected:
#if WITH_EDITOR
	// Center the children of this collectable by their bounds rather than their pivot
	UFUNCTION(CallInEditor, Category="Collectable")
	void CenterRootChildren();
#endif
};
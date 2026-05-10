// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "CoverLandmarkProxy.generated.h"

enum class ECoverLandmarkAnimDirection : uint8;

UCLASS(HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API ACoverLandmarkProxy : public AActor
{
	GENERATED_BODY()

public:
	ACoverLandmarkProxy();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Data")
	class ACoverLandmark* LandmarkOwner = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Data")
	ECoverLandmarkAnimDirection EntryDirection;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Data")
	ECoverLandmarkAnimDirection ExitDirection;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Events")
	void OnProxyUse(bool bIsActive);
	
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Events")
	void OnProxyEnd(bool bSuccess);
	
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Events")
	void EnableProxyInteraction();
	
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Events")
	void DisableProxyInteraction();
	
protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneComponent = nullptr;

	#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UBillboardComponent* BillboardComponent = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "Components")
	UArrowComponent* ArrowComponent = nullptr;
	#endif
	
	virtual void BeginPlay() override;
};

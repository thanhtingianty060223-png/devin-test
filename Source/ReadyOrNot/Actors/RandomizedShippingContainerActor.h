// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RandomizedShippingContainerActor.generated.h"

UCLASS()
class READYORNOT_API ARandomizedShippingContainerActor : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ContainerMeshComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ContainerDecalsComp;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* LeftDoorDecalsComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* RightDoorDecalsComp;
	
public:	
	// Sets default values for this actor's properties
	ARandomizedShippingContainerActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(CallInEditor)
	void RandomizeContainer();

	UPROPERTY(EditAnywhere)
	TArray<UMaterialInterface*> ContainerMaterials;

	UPROPERTY(EditAnywhere)
	TArray<UStaticMesh*> ContainerDecals;

	UPROPERTY(EditAnywhere)
	TArray<UStaticMesh*> LeftDoorDecals;
	
	UPROPERTY(EditAnywhere)
	TArray<UStaticMesh*> RightDoorDecals;

};

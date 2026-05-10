// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PremissionStreetView.generated.h"

UCLASS()
class READYORNOT_API APremissionStreetView : public AActor
{
	GENERATED_BODY()


	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* LeftBuildingMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* RightBuildingMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* LeftTrafficLight;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* RightTrafficLight;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UArrowComponent* Direction;

	
public:	
	// Sets default values for this actor's properties
	APremissionStreetView();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
	TArray<UStaticMesh*> Buildings;

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<AActor>> TrafficLights;

	FVector OriginalActorLocation;
	
	UPROPERTY(EditAnywhere)
	float InterpConstantSpeed = 100.0f;

	UPROPERTY(EditAnywhere)
	float TimeUntilReset = 5.0f;

};

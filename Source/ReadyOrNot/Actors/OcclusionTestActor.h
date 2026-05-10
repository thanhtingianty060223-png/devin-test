// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OcclusionTestActor.generated.h"

UCLASS()
class READYORNOT_API AOcclusionTestActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AOcclusionTestActor();

	// Sound occlusion parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion")
	float OcclusionMultiplier = 1.0f;

	// Depth to fully occlude gunshots (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion")
	float FullOcclusionDepth = 150.0f;

	// Depth to fully occlude gunshots (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion")
	float TickInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion")
	bool GunshotOrFootstep = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion | Gunshot")
	UFMODEvent* GunshotSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion | Gunshot")
	bool bIsOutside = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion | Footstep")
	UFMODEvent* FootstepSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion | Footstep")
	UFMODEvent* FoleySound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion | Footstep")
	bool bHeavyArmor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion | Footstep")
	bool bIsCrouching = false;
	


	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UFMODAudioPropagationComponent* FMODAudioPropagationComp = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UFMODAudioComponent *AudioComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void GetFMODFootstepParameters(int32& Stance, int32& Speed, int32& Surface);
};

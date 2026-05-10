// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_SpawnLight.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAnimNotifyState_SpawnLight : public UAnimNotifyState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere)
	float StartIntensity = 100.0f;

	UPROPERTY(EditAnywhere)
	float MiddleIntensity = 100.0f;

	UPROPERTY(EditAnywhere)
	float EndIntensity = 100.0f;
	
	UPROPERTY(EditAnywhere)
	float InterpSpeed = 1.0f;

	UPROPERTY()
	APointLight* PointLight;
	
	UPROPERTY()
	float MaxDuration = 0.0f;

	UPROPERTY()
	float CurrentDuration = 0.0f;

	public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	FName SocketName;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	
};

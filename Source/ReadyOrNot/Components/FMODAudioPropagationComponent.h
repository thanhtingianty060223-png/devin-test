// Copyright Void Interactive, 2021

#pragma once

#include "FMODAudioComponent.h"
#include "FMODAudioPropagationComponent.generated.h"

USTRUCT(BlueprintType)
struct FMODParam
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = Audio)
	FName paramName;

	UPROPERTY(EditAnywhere, Category = Audio)
	float paramVal;

	//UFUNCTION(BlueprintCallable)
	void SetValues(FName name, float val);
};

UCLASS()
class READYORNOT_API UFMODAudioPropagationComponent final : public UFMODAudioComponent
{
	GENERATED_BODY()

	UFMODAudioPropagationComponent();
	
	void OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath);

	FVector AudioLocation;
	FTimerHandle UpdateAudioPropagation_Handle;

protected:
	virtual void BeginPlay() override;
	//virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector AudioPlayLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float volumeToSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float silentDistance = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OcclusionAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float minDistance = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPlayPropagation = true;
	
public:
	bool bSearchingPath = false;
	
	//UFUNCTION(BlueprintCallable)
	//bool UpdateAudioPropagation();

	float GetOcclusionAmount();

	static float GetDepthMaterialOcclusionAmount(UWorld* World, TArray<AActor*> ActorsToIgnore, FVector SoundLocation, FVector ListenerLocation, float DefaultOcclusionDepth, float OcclusionMultiplier);

	UFUNCTION(BlueprintCallable)
	FFMODEventInstance PlayEvent(UFMODEvent* EventToPlay, FVector Origin, TArray<FMODParam> Params);

	UFUNCTION(BlueprintCallable)
	void PlayEventAttached(UFMODEvent* EventToPlay, USceneComponent* CompToAttach, FName AttachPoint, TArray<FMODParam> Params);
	
	void PlayEventAttached(UFMODEvent* EventToPlay, USceneComponent* CompToAttach, FName AttachPoint, TArray<FMODParam> Params, bool bCheckOcclusion);
	void PlayEventAttached(UFMODAudioComponent* AudioComp, const TArray<FMODParam>& Params, bool bCheckOcclusion = true);

	UFUNCTION(BlueprintCallable)
	bool CheckOcclusion();
};


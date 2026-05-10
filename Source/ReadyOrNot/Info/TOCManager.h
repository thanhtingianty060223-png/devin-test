// Void Interactive, 2020

#pragma once

#include "Data/TOCData.h"
#include "TOCManager.generated.h"

UCLASS(Transient, HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API ATOCManager final : public AActor
{
	GENERATED_BODY()

public:
	ATOCManager();

	UFUNCTION(BlueprintPure, DisplayName = "Get TOC Manager")
	static ATOCManager* Get();

	UFUNCTION(BlueprintCallable)
	void StartTOCResponse(FString Line, bool bIsNetworked = true, ETOCPriority Priority = ETOCPriority::ETP_MediumPriority);

	UFUNCTION(BlueprintCallable)
	void StopTOCAudio(bool bClearQueue = true);
	
	UFUNCTION(BlueprintPure)
	bool IsTOCSpeaking() const;
	
	UFUNCTION(BlueprintPure)
	bool IsTOCSpeakingLine(FString Line) const;

	bool bCanPlayPrefix = true;

	float ROEViolateTOCResponseDelay = 1.15f;

	FORCEINLINE int32 GetNumQueuedResponses() const { return QueuedTOCData.Num(); }
	
protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(VisibleDefaultsOnly)
	USceneComponent* SceneComponent = nullptr;
	
	UPROPERTY(Transient)
	USoundSource* VoiceSoundSource;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Audio")
	UFMODEvent* TocEvent = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Audio")
	FTOCData CurrentTOCData;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Audio")
	TArray<FTOCData> QueuedTOCData;
	
	#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Debug")
	FString DebugTOCLine = "";
	#endif
	
	UFUNCTION(NetMulticast, Reliable)
    void Multicast_PlayTOCSound2D(const FString& FileName);
	void Multicast_PlayTOCSound2D_Implementation(const FString& FileName);

	bool AddDataToQueue(const FTOCData& TOCData);
	bool RemoveDataFromQueue(const FTOCData& TOCData);
	
	void PlayTOCLine(const FString& VoiceLine, bool bIsNetworked);
	
	void LogSpeechData(const FString& VoiceLine, const FString Filename);
	
private:
	void PlayTOCSound_Internal(const FString& FilePath, const FString& FileName);

	#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "Debug")
	void DebugStartTOCResponse();	
	#endif
	
	UFUNCTION()
    void IterateTOCQueue();
};

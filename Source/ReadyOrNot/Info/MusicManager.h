// Void Interactive, 2020

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "MusicManager.generated.h"

UCLASS(BlueprintType, NotBlueprintable)
class READYORNOT_API UMusicManager final : public UWorldSubsystem
{
	GENERATED_BODY()

	UMusicManager();
	
public:
	static UMusicManager* Get(UObject* Context);

	UFUNCTION(BlueprintPure)
	TArray<FString> GetMusicParameters() const;

	UFUNCTION(BlueprintCallable)
	void StopTheMusic(bool bGoHome = true);
	
	UFUNCTION(BlueprintCallable)
	void ResumeMusicParametersUpdate();

	UFUNCTION(BlueprintCallable)
	void StartMusicParametersUpdate();

	UFUNCTION(BlueprintCallable)
	void PauseMusicParametersUpdate();
	
	UFUNCTION(BlueprintCallable)
	void StopMusicParametersUpdate();
	
	UFUNCTION(BlueprintCallable)
	void SetMusicParameterValue(FString ParamName, float ParamValue);

	static float GetCombatMusicIntensity();
	
protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

	UPROPERTY(BlueprintReadOnly)
	UFMODEvent* MusicEvent;

	UPROPERTY(BlueprintReadOnly)
	FFMODEventInstance MusicEventInst;

	UPROPERTY(BlueprintReadOnly)
	FTimerHandle TH_UpdateMusicParameters;
	
	UFUNCTION()
	void UpdateMusicParameters();

	FVector GetLocalPlayerLocation() const;
	float GetClosestDistanceToDoor() const;
};

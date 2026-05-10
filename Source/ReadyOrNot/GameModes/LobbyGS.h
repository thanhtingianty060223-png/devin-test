// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "ReadyOrNotGameState.h"
#include "LobbyGS.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ALobbyGS : public AReadyOrNotGameState
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void LoadStartupWidgetsAfterLoadingScreen() override {};

	void SetPreviousMissionGrade(const FString& Grade);
	
private:
	UPROPERTY(EditAnywhere)
	UFMODEvent* LobbyMusicEvent;

	FFMODEventInstance LobbyEventInst;

	UPROPERTY(Replicated)
	float MissionGradeMusic = 0.0f;
	
	float LobbyMusicMorale = -1.0f;

	void PlayLobbyMusic();
	void StopLobbyMusic();

	void SetLoadoutMusic(float Value);
	void SetCommanderMusic(float Value);

	bool bFinishedLoading = false;
};

// Void Interactive, 2020

#pragma once

#include "ReadyOrNotGameState.h"
#include "CaptureTheFlagGS.generated.h"

DECLARE_STATS_GROUP(TEXT("RON CTF GS"), STATGROUP_RONCTFGS, STATCAT_Advanced);

/**
 * 
 */
UCLASS()
class READYORNOT_API ACaptureTheFlagGS final : public AReadyOrNotGameState
{
	GENERATED_BODY()

public:
	ACaptureTheFlagGS();
	
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Capture The Flag|Game State|Data")
	class ACTF_Flag* Flag = nullptr;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Capture The Flag|Game State|Data")
	APlayerCharacter* FlagBearer = nullptr;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Capture The Flag|Game State|Data")
	ETeamType FlagBearerTeam = ETeamType::TT_NONE;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_FlagStatus, Category = "Capture The Flag|Game State|Data")
	uint8 bFlagCaptured : 1;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Capture The Flag|Game State|Data")
	uint8 bGameWon : 1;

protected:
	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
private:
	UFUNCTION()
	void OnRep_FlagStatus();
};

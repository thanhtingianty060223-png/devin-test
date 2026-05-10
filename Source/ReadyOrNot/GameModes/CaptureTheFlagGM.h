// Void Interactive, 2020

#pragma once

#include "ReadyOrNotGameMode_PVP.h"
#include "CaptureTheFlagGM.generated.h"

DECLARE_STATS_GROUP(TEXT("RON CTF GM"), STATGROUP_RONCTFGM, STATCAT_Advanced);

/**
 * 
 */
UCLASS()
class READYORNOT_API ACaptureTheFlagGM final : public AReadyOrNotGameMode_PVP
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Capture The Flag|Game Mode")
    void CaptureFlag(class ACTF_Flag* CapturedFlag, APlayerCharacter* NewFlagBearer);

	UFUNCTION(BlueprintCallable, Category = "Capture The Flag|Game Mode")
    void DropFlag();
	
	UFUNCTION(BlueprintCallable, Category = "Capture The Flag|Game Mode")
	class ACTF_FlagSpawnPoint* ChooseFlagSpawnPoint();

	void SpawnFlag();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFlagCapturedSignature, APlayerCharacter*, CapturedByCharacter, ETeamType, CpaturedByTeam);
	UPROPERTY(BlueprintAssignable, Category = "Capture The Flag|Game Mode|Events")
	FOnFlagCapturedSignature OnFlagCaptured;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFlagDroppedSignature, APlayerCharacter*, DroppedByCharacter, ETeamType, DroppedByTeam);
	UPROPERTY(BlueprintAssignable, Category = "Capture The Flag|Game Mode|Events")
	FOnFlagDroppedSignature OnFlagDropped;

	void RoundWonTeam(ETeamType WinningTeam) override;

protected:
	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;

	AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	void TimeLimitVictoryConditions_Implementation() override;
	
	bool ShouldCountDownTimelimitNow() override;

	void StartMatch() override;
	void NextRound() override;

	void RoundEnd() override;

	UPROPERTY(EditDefaultsOnly, Category = "Settings")
	TSubclassOf<class ACTF_Flag> FlagClassToSpawn = nullptr;
	
private:
	UPROPERTY()
	class ACTF_Flag* Flag = nullptr;
	
	// Called when the flag bearer has been killed
	UFUNCTION()
    void OnFlagBearerKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter);

	void BroadcastFlagCaptured();
	void BroadcastFlagDropped();

	uint8 bHasOnFlagCapturedEventBroadcasted : 1;
	uint8 bHasOnFlagDroppedEventBroadcasted : 1;
	
	void DestroyAllCTFFlags();

	void DestroyFlag();
};

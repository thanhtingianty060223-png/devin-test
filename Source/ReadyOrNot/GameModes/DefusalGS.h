// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "ReadyOrNotGameState.h"
#include "DefusalGS.generated.h"


class ADefusalGM;
UENUM(BlueprintType)
enum class EDefusalMatchSate : uint8
{
	DMS_Warmup,
	DMS_PreRoundTimer,
	DMS_Playing,
	DMS_HalfTime,
	DMS_MatchFinished
};


/**
 * 
 */
UCLASS()
class READYORNOT_API ADefusalGS : public AReadyOrNotGameState
{
	
	GENERATED_BODY()
	
	ADefusalGS();

	virtual void BeginPlay() override;

protected:
	UPROPERTY(Replicated, BlueprintReadOnly)
	EDefusalMatchSate DefusalMatchState;

	UPROPERTY()
	UUserWidget* DefusalHudInst;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> DefusalHudClass;
	
	UPROPERTY()
	class ALoadoutPortal* LoadoutPortal;

public:
	void ChangeDefusalMatchState(EDefusalMatchSate NewMatchState);

	EDefusalMatchSate GetDefusalMatchstate() { return DefusalMatchState; }
	void OpenBuyMenu(APlayerController* Controller);

	UPROPERTY(Replicated)
	int32 CountdownUntilMatchStarts;

	UPROPERTY(Replicated)
	int32 ElapsedRoundTime;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 BombTimeRemaining;

	
};

// Copyright Void Interactive, 2017

#pragma once

#include "Engine/DataAsset.h"
#include "Sound/SoundCue.h"
#include "FMODBlueprintStatics.h"
#include "TocSpeechData.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UTocSpeechData : public UDataAsset
{
	GENERATED_BODY()

public:
	// CO-OP lines
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CO-OP")
		USoundCue* CivilianIncapacitated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CO-OP")
		USoundCue* CivilianDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CO-OP")
		USoundCue* CivilianRestrained;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CO-OP")
		USoundCue* SuspectIncapacitated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CO-OP")
		USoundCue* SuspectDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CO-OP")
		USoundCue* SuspectRestrained;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CO-OP")
		USoundCue* OfficerDown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CO-OP")
		USoundCue* DOA;

	// PvP lines (note, for this we are using FMOD)
	UPROPERTY(EditAnywhere, BLueprintReadWrite, Category = "PvP")
		UFMODEvent* SWATVictory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PvP")
		UFMODEvent* SuspectVictory;

	UPROPERTY(EditAnywhere, BLueprintReadWrite, Category = "PvP")
		UFMODEvent* SWATInCustody;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PvP")
		UFMODEvent* SuspectInCustody_MP;

	UPROPERTY(EditAnywhere, BLueprintReadWrite, Category = "PvP")
		UFMODEvent* SWATReinforcements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PvP")
		UFMODEvent* SuspectReinforcements;

	UPROPERTY(EditAnywhere, BLueprintReadWrite, Category = "PvP")
		UFMODEvent* BothTeamsReinforcements;

	UPROPERTY(EditAnywhere, BLueprintReadWrite, Category = "PvP")
		UFMODEvent* SWATFriendlyFire;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PvP")
		UFMODEvent* SuspectFriendlyFire;

	// The VIP has reached the safe zone (SWAT victory)
	UPROPERTY(EditAnywhere, BLueprintReadWrite, Category = "PvP")
		UFMODEvent* VIPEscorted;

	// The VIP has been executed (Suspect victory)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PvP")
		UFMODEvent* VIPExecuted;

	// The VIP was killed by SWAT (Suspect victory)
	UPROPERTY(EditAnywhere, BLueprintReadWrite, Category = "PvP")
		UFMODEvent* SWATKilledVIP;

	// The VIP was killed by the suspects at an inappropriate time (SWAT victory)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PvP")
		UFMODEvent* SuspectKilledVIP;

	// The VIP has been taken into custody
	UPROPERTY(EditAnywhere, BLueprintReadWrite, Category = "PvP")
		UFMODEvent* VIPInCustody;

	// The VIP has been released
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PvP")
		UFMODEvent* VIPReleased;
};

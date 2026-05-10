// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ReadyOrNotCoverData.generated.h"


USTRUCT(BlueprintType)
struct FCoverDirectionalTrans
{
	GENERATED_USTRUCT_BODY()

	/* the transition to play when facing directly*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* CoverTrans_0;

	/* the distance offset when facing directly to predict cover positioning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector DistanceOffsetCover_0;

	/* the transition to play when facing 90 degrees*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* CoverTrans_90;

	/* the distance offset when facing 90 degrees to predict cover positioning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector DistanceOffsetCover_90;
};

USTRUCT(BlueprintType)
struct FCoverTrans
{
	GENERATED_USTRUCT_BODY()

	/* the transition to play when entering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* CoverEnterTrans;

	/* the transition to play when exiting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* CoverExitTrans;
};


USTRUCT(BlueprintType)
struct FCoverDataMain
{
	GENERATED_USTRUCT_BODY()


	/* basic cover pose when entered cover */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoverMain)
	UAnimSequence* CoverIdlePose;

	/* if this cover set has the ability to perform vertical exposure like over the cover itself */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoverMain)
	bool bHasVerticalExposure;

	/* the initial transition going into cover*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoverMain)
	FCoverDirectionalTrans CoverEnterTrans;

	/* the initial transition going out of cover*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoverMain)
	FCoverDirectionalTrans CoverExitTrans;

	/* the transition played when we switch sides and to a next coverset*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoverMain)
	UAnimMontage* SideSwitchTrans;


	// ====================================================================
	// ====================================================================
	// ====================================================================


	/* HORIZONTAL DATA */

	/* the transition we play when we enter/exit horizontal aiming */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoverHorizontal)
	FCoverTrans AimingHTrans;

	/* the transition we play when we enter/exit horizontal blindfiring*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoverHorizontal)
	FCoverTrans BlindFireHTrans;

	/* The horizontal blindfire pose */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoverHorizontal)
	UAnimSequence* BlindFireHIdlePose;

	/* The horizontal aiming pose */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoverHorizontal)
	UAnimSequence* AimingHIdlePose;


	// ====================================================================
	// ====================================================================
	// ====================================================================


	/* VERTICAL DATA: */

	/* the transition we play when we enter/exit horizontal aiming */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoverVertical)
	FCoverTrans AimingVTrans;

	/* the transition we play when we enter/exit horizontal blindfiring*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoverVertical)
	FCoverTrans BlindFireVTrans;

	/* The vertical blindfire pose */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoverVertical)
	UAnimSequence* BlindFireVIdlePose;

	/* The vertical aiming pose */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CoverVertical)
	UAnimSequence* AimingVIdlePose;
};

/**
 * 
 */
UCLASS(BlueprintType)
class READYORNOT_API UReadyOrNotCoverData : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	FCoverDataMain CoverData;
};

// Copyright Void Interactive, 2021

#pragma once

#include "Engine/DataTable.h"
#include "Enums.h"
#include "Actors/BaseItem.h"
#include "AnimationDataTable.generated.h"

USTRUCT(BlueprintType)
struct FAnimSectionData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly)
	float SectionStart = 0.0f;

	UPROPERTY(EditDefaultsOnly)
	float SectionEnd = 0.0f;
};

USTRUCT(BlueprintType)
struct FAnimWeaponData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly)
	TArray<UAnimMontage*> AnimMontages;
};

USTRUCT(BlueprintType)
struct FAnimStanceData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly)
	FAnimWeaponData StandingAnimData;
	
	UPROPERTY(EditDefaultsOnly)
	FAnimWeaponData CrouchedAnimData;
};

/**
 * 
 */
USTRUCT(BlueprintType)
struct FAnimationDataTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TMap<EAnimWeaponType, FAnimStanceData> AnimData;
	
	UPROPERTY(EditDefaultsOnly)
	bool bRestartIfAlreadyPlaying = false;
	
	UPROPERTY(EditDefaultsOnly)
	float MaxRandomDelay = 0.0f;
	
	UPROPERTY(EditDefaultsOnly)
	float Cooldown = 0.0f;
	
	UPROPERTY(EditDefaultsOnly)
	bool bNoCanPlayWhileStrafing = false;

	UPROPERTY(EditDefaultsOnly)
	bool bNoCanPlayWhileNotStrafing = false;
	
	UPROPERTY(EditDefaultsOnly)
	bool bCanQueue = true;

	UPROPERTY(EditDefaultsOnly)
	bool bCanAnimationBeInterupted = true;

	UPROPERTY(EditDefaultsOnly)
	TMap<TSubclassOf<ABaseItem>, FString> OverrideAnimation;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "!bCanAnimationBeInterupted"))
	TArray<FString> CanOnlyBeInteruptedBy;
};

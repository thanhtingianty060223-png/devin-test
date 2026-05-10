// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "ProgressionData.generated.h"

UCLASS(Abstract, EditInlineNew, CollapseCategories)
class READYORNOT_API UProgressionRequirement : public UObject
{
	GENERATED_BODY()

public:
	virtual bool IsLocked(const TSet<FName>& InProgressionTags);
};

UENUM()
enum class ELevelGrade
{
	S,
	APlus,
	A,
	B,
	C,
	D,
	E,
	F
};

UCLASS()
class READYORNOT_API ULevelCompleteRequirement : public UProgressionRequirement
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, meta=(GetOptions="GetLevelOptions"))
	FName RequiredLevel;

	UPROPERTY(EditAnywhere)
	ELevelGrade RequiredGrade;
	
	virtual bool IsLocked(const TSet<FName>& InProgressionTags) override;

protected:
	UFUNCTION()
	TArray<FName> GetLevelOptions() const;
};


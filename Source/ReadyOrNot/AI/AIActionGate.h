// Copyright Void Interactive, 2022

#pragma once

#include "AI/AIActionData.h"
#include "AIActionGate.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, Const)
class READYORNOT_API UAIActionGate : public UObject
{
	GENERATED_BODY()

public:
	virtual UWorld* GetWorld() const override final;
	
	UFUNCTION(BlueprintNativeEvent)
	bool CanOpen(const FAIActionDecisionContext& Context) const;

protected:
	virtual bool CanOpen_Implementation(const FAIActionDecisionContext& Context) const { return true; }
};

UCLASS(BlueprintType, EditInlineNew, Const, DisplayName = "Valid Target")
class READYORNOT_API UAGValidTarget final : public UAIActionGate
{
	GENERATED_BODY()

public:
	virtual bool CanOpen_Implementation(const FAIActionDecisionContext& Context) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	bool bAllowFriendly = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	bool bAllowEnemy = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	bool bAllowNeutral = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	bool bAllowLastTrackedTarget = false;
};

UCLASS(BlueprintType, EditInlineNew, Const, DisplayName = "Number Of Nearby AI")
class READYORNOT_API UAGAnyNearbyAI final : public UAIActionGate
{
	GENERATED_BODY()

public:
	virtual bool CanOpen_Implementation(const FAIActionDecisionContext& Context) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Options", meta = (ExposeOnSpawn = true))
	ETeamType TeamType = ETeamType::TT_SUSPECT; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Options", meta = (ExposeOnSpawn = true))
	float SearchRange = 1000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Options", meta = (ExposeOnSpawn = true))
	bool bMustBeVisible = false;
};

UENUM(BlueprintType)
enum class EComparisonOp : uint8
{
	Equals,
	NotEquals,
	GreaterThan,
	GreaterThanEquals,
	LessThan,
	LessThanEquals,
};

UCLASS(BlueprintType, EditInlineNew, Const, DisplayName = "Num Active AI")
class READYORNOT_API UAGNumActiveAI final : public UAIActionGate
{
	GENERATED_BODY()

public:
	virtual bool CanOpen_Implementation(const FAIActionDecisionContext& Context) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Options", meta = (ExposeOnSpawn = true))
	ETeamType Team = ETeamType::TT_SUSPECT;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Options", meta = (ExposeOnSpawn = true))
	uint8 NumAI = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Options", meta = (ExposeOnSpawn = true))
	EComparisonOp ComparisonType = EComparisonOp::Equals;
};

UCLASS(BlueprintType, EditInlineNew, Const, DisplayName = "Has Equipped Item")
class READYORNOT_API UAG_HasEquippedItem final : public UAIActionGate
{
	GENERATED_BODY()

public:
	virtual bool CanOpen_Implementation(const FAIActionDecisionContext& Context) const override;
};

UCLASS(BlueprintType, EditInlineNew, Const, DisplayName = "Has Equipped Weapon")
class READYORNOT_API UAG_HasEquippedWeapon final : public UAIActionGate
{
	GENERATED_BODY()

public:
	virtual bool CanOpen_Implementation(const FAIActionDecisionContext& Context) const override;
};

UCLASS(BlueprintType, EditInlineNew, Const, DisplayName = "Is Surrendered")
class READYORNOT_API UAG_IsSurrendered final : public UAIActionGate
{
	GENERATED_BODY()

public:
	virtual bool CanOpen_Implementation(const FAIActionDecisionContext& Context) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Options", meta = (ExposeOnSpawn = true))
	float Time = 0.0f;
};

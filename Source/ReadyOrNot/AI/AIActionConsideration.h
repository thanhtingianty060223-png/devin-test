// Copyright Void Interactive, 2022

#pragma once

#include "AIActionConsideration.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, Const)
class READYORNOT_API UAIActionConsideration : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	float Score(const FAIActionDecisionContext& Context, bool& bSuccess) const;

	UFUNCTION(BlueprintPure)
	float EvaluateResponseCurve(float Score);
	
protected:
	virtual float Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const { return 0.0f; }
	
	UFUNCTION(BlueprintNativeEvent)
	float CalculateCurve(float X) const;
	virtual float CalculateCurve_Implementation(const float X) const { return X; }

	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif
	
	#if WITH_EDITORONLY_DATA
	virtual void Serialize(FArchive& Ar) override;
	#endif

	// TODO: experiment with this
	//FAlphaBlend CurveType;

	virtual UWorld* GetWorld() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	uint8 bManualCurveEdit : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "!bManualCurveEdit"))
	uint8 bCustomCurveFunction : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "!bManualCurveEdit && !bCustomCurveFunction"))
	TEnumAsByte<EEasingFunc::Type> CurveType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "!bManualCurveEdit"))
	uint8 bInverseX : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "!bManualCurveEdit"))
	uint8 bInverseY : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	uint8 bClamp : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Response Curve")
	FRuntimeFloatCurve Curve;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Response Curve")
	uint8 bCustomRange : 1;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Response Curve", meta = (EditCondition = "bCustomRange"))
	float MinRange = 0.0f;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Response Curve", meta = (EditCondition = "bCustomRange"))
	float MaxRange = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Response Curve")
	float OffsetX = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Response Curve")
	float OffsetY = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Response Curve")
	float Exponent = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Response Curve", meta = (ClampMin = 1, ClampMax = 100))
	int32 SubStep = 20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Response Curve")
	TEnumAsByte<ERichCurveInterpMode> InterpMode = RCIM_Linear;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Response Curve")
	TEnumAsByte<ERichCurveTangentMode> TangentMode = RCTM_Auto;
	
private:
	void UpdateCurve();
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, Const)
class READYORNOT_API UACNumberOfNearbyAI final : public UAIActionConsideration
{
	GENERATED_BODY()

protected:
	virtual float Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Options", meta = (ExposeOnSpawn = true))
	ETeamType TeamType = ETeamType::TT_SUSPECT;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Options", meta = (ExposeOnSpawn = true))
	float SearchRange = 1000.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Options", meta = (ExposeOnSpawn = true))
	bool bMustBeVisible = false;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, Const, DisplayName = "Health")
class READYORNOT_API UAC_Health final : public UAIActionConsideration
{
	GENERATED_BODY()

protected:
	virtual float Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, Const, DisplayName = "Target Health")
class READYORNOT_API UAC_TargetHealth final : public UAIActionConsideration
{
	GENERATED_BODY()

protected:
	virtual float Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, Const, DisplayName = "Incapacitation Health")
class READYORNOT_API UAC_IncapacitationHealth final : public UAIActionConsideration
{
	GENERATED_BODY()

protected:
	virtual float Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, Const, DisplayName = "Ammo")
class READYORNOT_API UAC_Ammo final : public UAIActionConsideration
{
	GENERATED_BODY()

protected:
	virtual float Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, Const, DisplayName = "Armor")
class READYORNOT_API UAC_Armor final : public UAIActionConsideration
{
	GENERATED_BODY()

protected:
	virtual float Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, Const, DisplayName = "Morale")
class READYORNOT_API UAC_Morale final : public UAIActionConsideration
{
	GENERATED_BODY()

protected:
	virtual float Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const override;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, Const, DisplayName = "Stress")
class READYORNOT_API UAC_Stress final : public UAIActionConsideration
{
	GENERATED_BODY()

protected:
	virtual float Score_Implementation(const FAIActionDecisionContext& Context, bool& bSuccess) const override;
};

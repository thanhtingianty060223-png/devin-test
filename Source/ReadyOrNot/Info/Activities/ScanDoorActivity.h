// Copyright Void Interactive, 2023

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "ScanDoorActivity.generated.h"

UENUM(BlueprintType)
enum class EDoorScanMethod : uint8
{
	None,
	Slide,
	Slice,
	Snap,
	CenterCheck
};

UCLASS()
class READYORNOT_API UScanDoorActivity final : public UBaseActivity
{
	GENERATED_BODY()

public:
	UScanDoorActivity();
	
	UPROPERTY(BlueprintReadOnly)
	ADoor* Door = nullptr;

	FVector CommandLocation = FVector::ZeroVector;

	EDoorScanMethod ScanMethod = EDoorScanMethod::Slide;
	
	FIntVector OppositeStackUpLocation = FIntVector::ZeroValue;

	FORCEINLINE uint8 GetSliceStage() const { return SliceStage; }

protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual bool ShouldForceStrafe() const override;
	virtual bool ShouldForceNoStrafe() const override;
	virtual void PerformActivity(float DeltaTime) override;
	#if !UE_BUILD_SHIPPING
	virtual void PerformActivity_Debug(float DeltaTime) override;
	#endif
	virtual void FinishedActivity(bool bSuccess) override;
	virtual bool CanFinishActivity() const override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual bool GetOverrideMovementSpeed(float& OutMovementSpeed) const override;
	virtual void ActivityOverriden(UBaseActivity* OverridingActivity) override;
	virtual void ResumeActivity() override;
	virtual void ResetData() override;
	virtual bool CanBePushed() const override;

	virtual bool GetLeanOverride(float& LeanOverride) const override;

	UFUNCTION()
	void EnterMoveToStage();
	UFUNCTION()
	void TickMoveToStage(float DeltaTime, float Uptime);
	UFUNCTION()
	bool CanStartScanning() const;
	
	UFUNCTION()
	void EnterScanStage();
	UFUNCTION()
	void TickScanStage(float DeltaTime, float Uptime);
	UFUNCTION()
	bool IsScanComplete() const;
	
	UFUNCTION()
	void EnterCompleteStage();
	UFUNCTION()
	void TickCompleteStage(float DeltaTime, float Uptime);

	bool CanScanForThreats() const;
	
	UPROPERTY()
	TArray<ACyberneticCharacter*> SpottedCharacters;

	UPROPERTY()
	class ATrapActorAttachedToDoor* SpottedTrap = nullptr;

	FVector StartFocalPoint = FVector::ZeroVector;
	FIntVector LocationBeforeReturn = FIntVector::ZeroValue;
	FIntVector OriginalLocation = FIntVector::ZeroValue;
	FVector RightDoorVector = FVector::ZeroVector;
	uint8 SliceStage = 0;

	bool bPeekedDoor = false;

private:
	FVector SliceFocalPoint() const;
	FVector SlideFocalPoint() const;
	FVector SnapFocalPoint() const;
};

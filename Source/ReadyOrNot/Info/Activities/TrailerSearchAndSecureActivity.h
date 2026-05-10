// Copyright Void Interactive, 2023

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "TrailerSearchAndSecureActivity.generated.h"

UCLASS()
class READYORNOT_API UTrailerSearchAndSecureActivity : public UBaseActivity
{
	GENERATED_BODY()
	
public:
	UTrailerSearchAndSecureActivity();
	
	UPROPERTY()
	TArray<AActor*> AllSecurables;
	
	FVector SpawnLocation = FVector::ZeroVector;
	FVector CommandLocation = FVector::ZeroVector;
	
	FRoom* SearchingRoom = nullptr;

	virtual void ResetData() override;

protected:
	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	virtual bool CanFinishActivity() const override;

	void TrySecure();
	
	UPROPERTY()
	AActor* ClosestSecurable = nullptr;
};

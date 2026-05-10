// Copyright Void Interactive, 2022

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "TraverseHoleActivity.generated.h"

UCLASS()
class READYORNOT_API UTraverseHoleActivity : public UBaseActivity
{
	GENERATED_BODY()

public:
	UTraverseHoleActivity();

	virtual void StartActivity(AAIController* Owner) override;
	virtual void PerformActivity(float DeltaTime) override;
	#if !UE_BUILD_SHIPPING
	virtual void PerformActivity_Debug(float DeltaTime) override;
	#endif
	virtual void FinishedActivity(bool bSuccess) override;
	virtual void FinishedActivity_NoOwner(bool bSuccess) override;
	virtual bool CanFinishActivity() const override;
	virtual bool CanShoot() const override;
	virtual bool ShouldForceStrafe() const override;
	virtual bool ShouldForceNoStrafe() const override;
	virtual bool CanOverrideActivity() const override;
	virtual bool OverrideFocalPoint(FVector& FocalPoint) override;
	virtual float GetDestinationTolerance() const override;
	virtual void ResetData() override;

	virtual void OnIncapacitated(AReadyOrNotCharacter* IncapacitatedCharacter, AReadyOrNotCharacter* InstigatorCharacter) override;
	virtual void OnKilled(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* KilledCharacter) override;
	
	// If null, find nearest hole
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	class AWallHoleTraversal* WallHoleTraversalActor = nullptr;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	uint8 bIgnoreCooldown : 1;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	uint8 bFromNavLink : 1;

protected:
	bool HasReachedEntryLocation(float Tolerance) const;

	void TeleportWeapon();
	
	void AddIgnoredActors();
	void RemoveIgnoredActors();

	UFUNCTION()
	void Enter_MoveToHole_State();
	UFUNCTION()
	void Tick_MoveToHole_State(float DeltaTime, float Uptime);
	UFUNCTION()
	bool CanEnterHole() const;
	
	UFUNCTION()
	void Enter_EnterHole_State();
	UFUNCTION()
	void Tick_EnterHole_State(float DeltaTime, float Uptime);
	UFUNCTION()
	bool ShouldMove() const;
	
	UFUNCTION()
	void Enter_Move_State();
	UFUNCTION()
	void Tick_Move_State(float DeltaTime, float Uptime);
	UFUNCTION()
	bool CanExitHole() const;
	
	UFUNCTION()
	void Enter_ExitHole_State();
	UFUNCTION()
	void Tick_ExitHole_State(float DeltaTime, float Uptime);
	
	UPROPERTY(VisibleInstanceOnly)
	UAnimMontage* EntryAnim = nullptr;
	UPROPERTY(VisibleInstanceOnly)
	UAnimMontage* ExitAnim = nullptr;
	UPROPERTY(VisibleInstanceOnly)
	UAnimMontage* LoopAnim = nullptr;

	float TimeEnteringHole = 0.0f;
	float TimeExitingHole = 0.0f;
	float EntryAnimTime = 0.0f;
	float ExitAnimTime = 0.0f;

	float TraversalProgress = 0.0f;

	FTransform ChosenEntryTransform;
	FTransform ChosenExitTransform;
	
	uint8 bIsInverseEntry : 1;
};

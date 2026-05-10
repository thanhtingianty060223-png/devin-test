// Void Interactive, 2020

#pragma once

#include "Info/Activities/BaseActivity.h"
#include "BaseCombatMoveActivity.generated.h"

UENUM(BlueprintType)
enum class ENavType : uint8
{
	NT_SinglePoint,
	NT_Circle,
	NT_Grid
};

UENUM(BlueprintType)
enum class ENavLOS : uint8
{
	NL_Any,
	NL_BreakLOS,
	NL_RequireLOS,
	NL_RequireLOSToMyself
};

USTRUCT(BlueprintType)
struct FNavGenerationParameters
{
	GENERATED_USTRUCT_BODY()

	FNavGenerationParameters()
	{
		NavType = ENavType::NT_SinglePoint;
		LOSType = ENavLOS::NL_Any;
		Radius = 500.0f;
		MaxDistance = 0;
	}

	FNavGenerationParameters(const ENavType& InNavType, const ENavLOS& InLOSType, const float InRadius, const float InMaxDistance = 0.0f)
	{
		NavType = InNavType;
		LOSType = InLOSType;
		Radius = InRadius;
		MaxDistance = InMaxDistance;
	}
	
	ENavType NavType = ENavType::NT_SinglePoint;
	ENavLOS LOSType = ENavLOS::NL_Any;

	float Radius = 500.0f;

	int32 MaxDistance = 0;
};

UCLASS(Abstract, BlueprintType, Blueprintable)
class READYORNOT_API UBaseCombatMoveActivity : public UBaseActivity
{
	GENERATED_BODY()
	
public:
	UBaseCombatMoveActivity();

	virtual void StartActivity(AAIController* Owner) override;

	virtual void OnMoveInterrupted(UBaseActivity* Activity);
	virtual void OnMoveResumed();
	
	// Use RequestCombatMove instead
	virtual void PerformActivity(float DeltaTime) override final;
	virtual bool CanFinishActivity() const override final;

	#if !UE_BUILD_SHIPPING
	virtual void GatherDebugString(FString& OutString) override;
	FString UnableToCombatReason = "";
	#endif

	bool IsActive() const;

	UFUNCTION(BlueprintPure)
	FORCEINLINE UBaseActivity* GetInterruptActivity() const { return InterruptActivity; }

	UFUNCTION(BlueprintPure)
	bool HasLOSToEnemy() const;
	
	UFUNCTION(BlueprintPure)
	bool HasAnyOtherCombatMoveGotLocation(const FVector& TestLocation, float Tolerance = 10.0f) const;

	UFUNCTION(BlueprintCallable)
	void GenerateNavigablePoints(const FVector& GenAroundLoc, const FNavGenerationParameters& NavGenerationParameters, TArray<FVector>& OutLocations);

	UPROPERTY(BlueprintReadOnly)
	class UBaseCombatActivity* OwningCombatActivity = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	int32 FailureCount = 0;

	UPROPERTY(BlueprintReadOnly)
	float LastSuccessTime = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float LastFailTime = 0.f;
	
protected:
	virtual void RequestCombatMove(const float DeltaTime)
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "";
		#endif
		
		RequestCombatMove_Blueprint(DeltaTime);
	}
	
	virtual bool ShouldPerformCombatMove() const;
	
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Request Combat Move")
	void RequestCombatMove_Blueprint(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void FinishCombatMove(bool bSuccess = true);

	UPROPERTY()
	UBaseActivity* InterruptActivity = nullptr;
	
	FVector LocationBeforeInterrupt = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bRequireMagazineWeapon = false;
};


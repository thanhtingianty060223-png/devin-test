// Void Interactive, 2020

#pragma once

#include "Components/ResourceComponent.h"
#include "MoraleComponent.generated.h"

USTRUCT(BlueprintType)
struct FMoraleDamageTraceParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<AActor*> IgnoredActors;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UPrimitiveComponent*> IgnoredComponents;
};

USTRUCT(BlueprintType)
struct FMoraleChangeInfo
{
	GENERATED_BODY()

	float Delta = 0.0f;
	float TimeSinceChange = 0.0f;
};

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent), HideCategories = ("Activation", "Cooking", "AssetUserData", "Collision"))
class READYORNOT_API UMoraleComponent final : public UResourceComponent
{
	GENERATED_BODY()

public:
	UMoraleComponent();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	ACyberneticCharacter* OwnerCharacter = nullptr;

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetMorale() const { return Resource; }
	
	UFUNCTION(BlueprintPure)
	FORCEINLINE TMap<FName, FMoraleChangeInfo>& GetMoraleDamageHistory() { return MoraleDamageHistory; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE TMap<FName, FMoraleChangeInfo>& GetMoraleGainHistory() { return MoraleGainHistory; }
	
	UFUNCTION(BlueprintCallable)
	static void IncreaseMoraleOnCharacter(ACyberneticCharacter* Character, float MoraleValue, FName Reason = NAME_None);
	UFUNCTION(BlueprintCallable)
	static void LowerMoraleOnCharacter(ACyberneticCharacter* Character, float MoraleValue, FName Reason = NAME_None);
	UFUNCTION(BlueprintCallable)
	static void ResetMoraleOnCharacter(ACyberneticCharacter* Character);

	UFUNCTION(BlueprintCallable)
	static void ApplyRadialMoraleDamage(const UObject* WorldContextObject, FVector Location, float Damage, float Radius, FMoraleDamageTraceParameters LOSParameters, TArray<ETeamType> Teams, FName Reason = NAME_None);
	UFUNCTION(BlueprintCallable)
	static void ApplyRadialMoraleDamageWithFalloff(const UObject* WorldContextObject, FVector Location, float Damage, float InnerRadius, float OuterRadius, FMoraleDamageTraceParameters LOSParameters, TArray<ETeamType> Teams, EEasingFunc::Type FalloffCurve = EEasingFunc::Linear, FName Reason = NAME_None);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Morale")
	float StartingMorale = 1.0f;
	
protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	TMap<FName, FMoraleChangeInfo> MoraleDamageHistory;
	TMap<FName, FMoraleChangeInfo> MoraleGainHistory;
};

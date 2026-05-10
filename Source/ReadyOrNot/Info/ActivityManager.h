// Void Interactive, 2020

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "ActivityManager.generated.h"

UCLASS(BlueprintType, NotBlueprintable, Transient)
class READYORNOT_API UActivityManager final : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStartActivity, UBaseActivity*, Activity, ACyberneticController*, Controller);
	UPROPERTY(BlueprintAssignable)
	FOnStartActivity OnStartActivity;
	
	static UActivityManager* Get(UObject* ObjectContext);

	virtual void Deinitialize() override;

	template<typename T>
	static T* CreateActivity(UObject* ContextObject, TSubclassOf<T> InActivityClass = T::StaticClass(), const FText& InActivityName = FText::GetEmpty(), float InActivityStartDelay = 0.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Activity Manager", meta = (DefaultToSelf = "ContextObject", HidePin = "ContextObject", DeterminesOutputType = "InActivityClass"))
	static UBaseActivity* CreateActivity(UObject* ContextObject, TSubclassOf<UBaseActivity> InActivityClass, const FText& InActivityName = FText::GetEmpty(), float InActivityStartDelay = 0.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Activity Manager")
	static bool GiveActivityTo(UBaseActivity* InActivity, class ACyberneticCharacter* InCharacter, bool bOverrideCurrentActivity = true, bool bClearActivityList = false);

	template<typename T>
	static bool AnyAIHasActivity(TFunctionRef<bool(const T*)> Predicate = [](){ return true; });
	
	template<typename T>
	static void IterateAllActivitiesOfType(TFunctionRef<bool(T*)> Predicate = [](){ return true; });

	UPROPERTY()
	TArray<UBaseActivity*> AllActivities;
	
	UPROPERTY(BlueprintReadOnly)
	TMap<TSubclassOf<class UBaseActivity>, float> ActivityClassGlobalCooldownMap;

	UFUNCTION(BlueprintPure)
	bool IsActivityClassOnCooldown(TSubclassOf<class UBaseActivity> Class) const;
	
	UFUNCTION(BlueprintPure)
	bool IsActivityClassOnCooldown_WithTimeRemaining(TSubclassOf<class UBaseActivity> Class, float& TimeRemaining) const;

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override;
};

template<typename T>
T* UActivityManager::CreateActivity(UObject* ContextObject, TSubclassOf<T> InActivityClass, const FText& InActivityName, float InActivityStartDelay)
{
	static_assert(TIsDerivedFrom<T, UBaseActivity>::Value, "T must be derived from UBaseActivity");

	if (InActivityClass == nullptr)
	{
		InActivityClass = T::StaticClass();
	}
	
	T* NewActivity = NewObject<T>(ContextObject, InActivityClass);

	if (!InActivityName.IsEmpty())
		NewActivity->ActivityName = InActivityName;
	
	NewActivity->ActivityStartDelay = InActivityStartDelay;

	Get(ContextObject)->AllActivities.Add(NewActivity);

	return NewActivity;
}


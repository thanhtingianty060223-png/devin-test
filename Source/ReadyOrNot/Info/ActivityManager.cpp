// Void Interactive, 2020

#include "ActivityManager.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

UActivityManager* UActivityManager::Get(UObject* ContextObject)
{
	return ContextObject->GetWorld()->GetSubsystem<UActivityManager>();
}

void UActivityManager::Deinitialize()
{
	Super::Deinitialize();

	AllActivities.Empty();
	ActivityClassGlobalCooldownMap.Empty();
}

UBaseActivity* UActivityManager::CreateActivity(UObject* ContextObject, const TSubclassOf<UBaseActivity> InActivityClass, const FText& InActivityName, const float InActivityStartDelay)
{
	return CreateActivity<UBaseActivity>(ContextObject, InActivityClass, InActivityName, InActivityStartDelay);
}

bool UActivityManager::GiveActivityTo(UBaseActivity* InActivity, ACyberneticCharacter* InCharacter, const bool bOverrideCurrentActivity, const bool bClearActivityList)
{
	if (!InActivity || !InCharacter)
		return false;

	if (!InCharacter->GetCyberneticsController())
		return false;

	if (bClearActivityList)
	{
		InCharacter->GetCyberneticsController()->ClearActivityList();
	}

	return InCharacter->GetCyberneticsController()->AddActivity(InActivity, bOverrideCurrentActivity);
}

bool UActivityManager::IsActivityClassOnCooldown(TSubclassOf<UBaseActivity> Class) const
{
	if (const float* Time = ActivityClassGlobalCooldownMap.Find(Class))
	{
		return *Time > 0.0f;
	}

	return false;
}

bool UActivityManager::IsActivityClassOnCooldown_WithTimeRemaining(TSubclassOf<UBaseActivity> Class, float& TimeRemaining) const
{
	if (const float* Time = ActivityClassGlobalCooldownMap.Find(Class))
	{
		TimeRemaining = *Time;
		return *Time > 0.0f;
	}

	TimeRemaining = 0.0f;
	return false;
}

void UActivityManager::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (TMap<TSubclassOf<UBaseActivity>, float>::TIterator It = ActivityClassGlobalCooldownMap.CreateIterator(); It; ++It)
	{
		It.Value() -= DeltaSeconds;
		if (It.Value() <= 0.0f)
		{
			It.RemoveCurrent();
		}
	}
	
	AllActivities.RemoveAll([](UBaseActivity* Activity)
	{
		return !IsValid(Activity) ||
				!Activity->GetOwningController() ||
				!Activity->GetOwningController()->GetCharacter() ||
				Activity->GetOwningController()->GetCharacter()->IsDeadOrUnconscious() ||
				Activity->GetOwningController()->GetCharacter()->IsIncapacitated() ||
				Activity->GetOwningController()->GetCharacter()->IsArrested();
	});
}

TStatId UActivityManager::GetStatId() const
{
	return GetStatID();
}

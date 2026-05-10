// Copyright Void Interactive, 2021

#include "ReadyOrNotGameplayEffect.h"

#include "Engine/World.h"
#include "Log.h"

UWorld* UReadyOrNotGameplayEffect::GetWorld() const
{
	if (HasAllFlags(RF_ClassDefaultObject))
		return nullptr;
	
	return GetOuter() ? GetOuter()->GetWorld() : nullptr;
}

void UReadyOrNotGameplayEffect::Initialize_Implementation(AActor* InActor)
{
	Actor = InActor;
	World = GetWorld();

	if (World)
		TimerManager = &World->GetTimerManager();
}

void UReadyOrNotGameplayEffect::ApplyEffect_Implementation()
{
#if !UE_BUILD_SHIPPING
	ULog::Info(GetName() + " | Applied");
#endif
}

void UReadyOrNotGameplayEffect::Multicast_Initialize_Implementation(AActor* InActor)
{
	Initialize(InActor);
}

void UReadyOrNotGameplayEffect::Multicast_ApplyEffect_Implementation()
{
	ApplyEffect();
}

void UReadyOrNotGameplayEffect::Multicast_ApplyEffectFor_Implementation(const float Seconds)
{
	ApplyEffectFor(Seconds);
}

void UReadyOrNotGameplayEffect::Multicast_ResetEffect_Implementation()
{
	ResetEffect();
}

void UReadyOrNotGameplayEffect::ApplyEffectFor(const float Seconds)
{
	ApplyEffect();

	if (Seconds > 0.0f)
	{
		if (!TimerManager->IsTimerActive(TH_EffectExpiry))
			TimerManager->SetTimer(TH_EffectExpiry, this, &UReadyOrNotGameplayEffect::OnEffectExpired, Seconds);
	}
}

void UReadyOrNotGameplayEffect::ResetEffect_Implementation()
{
#if !UE_BUILD_SHIPPING
	ULog::Info(GetName() + " | Reset");
#endif
}

void UReadyOrNotGameplayEffect::OnEffectExpired_Implementation()
{
	ResetEffect();

	OnGameplayEffectExpired.Broadcast(GetClass());
}

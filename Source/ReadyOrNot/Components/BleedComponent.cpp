// Void Interactive, 2020

#include "BleedComponent.h"

#include "CharacterHealthComponent.h"
#include "PlayerPostProcessing.h"
#include "DamageTypes/BleedDamageType.h"
#include "GameplayEffects/PlayerEffect_ModifyRecoil.h"

#include "HUD/Widgets/HumanCharacterHUD_V2.h"

#include "lib/ReadyOrNotFunctionLibrary.h"
#include <CommonInputSubsystem.h>

#include "Actors/Items/BallisticsShield.h"

UBleedComponent::UBleedComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0167f;

	if(BleedEvent == nullptr)
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> FMODEvent(TEXT("FMODEvent'/Game/FMOD/Events/Character/Special/Bleed_Out.Bleed_Out'"));
        
		if(FMODEvent.Object != nullptr)
			BleedEvent = FMODEvent.Object;
	}

	// Quick hack to remove  the first two heals so we just stop the bleeding
	HealCount = 3;
}

void UBleedComponent::DestroyComponent(bool bPromoteChildren)
{
	Super::DestroyComponent(bPromoteChildren);
	
}

void UBleedComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UBleedComponent, bIsBleeding);
	DOREPLIFETIME(UBleedComponent, HealCount);
	DOREPLIFETIME(UBleedComponent, bTempStopBleeding);
}

APlayerCharacter* UBleedComponent::GetOwningCharacter() const
{
	return Cast<APlayerCharacter>(GetOwner());
}

void UBleedComponent::SetBreathingCompBleedEvent(bool bBleed)
{
	if(BleedEvent == nullptr || GetOwningCharacter() == nullptr || GetOwningCharacter()->IsLocalPlayer() == false)
		return;

	GetOwningCharacter()->GetFMODBreathingComp()->Stop();
	GetOwningCharacter()->GetFMODBreathingComp()->Release();
	GetOwningCharacter()->GetFMODBreathingComp()->Event = bBleed ? BleedEvent : GetOwningCharacter()->BreathingBaseEvent;
	GetOwningCharacter()->GetFMODBreathingComp()->Play();
}

void UBleedComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwningCharacter() && GetOwningCharacter()->IsPlayerControlled())
	{
		#if !UE_BUILD_SHIPPING
		if (GetOwningCharacter()->HasGodMode() || GetOwningCharacter()->GetHealthComponent()->IsUnlimitedResourceEnabled())
		{
			bIsBleeding = false;
			BleedTime = 0.0f;
			return;
		}
		#endif
		
		if (!GetOwningCharacter()->IsDeadOrUnconscious())
		{
			if (GetOwningCharacter()->HumanCharacterWidget_V2)
			{
				if (CanHeal() && !bTempStopBleeding)
				{
					if (!bShownActionPrompt || !GetOwningCharacter()->HumanCharacterWidget_V2->IsActionSlotInUse_Reserved(0))
					{
						bShownActionPrompt = true;

						const bool bUsingGamepad = UReadyOrNotFunctionLibrary::IsUsingGamepad(GetOwningCharacter()->GetRONPlayerController());
						
						GetOwningCharacter()->HumanCharacterWidget_V2->AssignPlayerActionPrompt_Reserved(0, IE_Repeat, NSLOCTEXT("BleedComponent", "Heal Self", "Stop Bleeding"), "Red", true, false);
					}
				}
				else
				{
					if (bShownActionPrompt)
					{
						bShownActionPrompt = false;
						GetOwningCharacter()->HumanCharacterWidget_V2->RemovePlayerActionPrompt_Reserved(0);
					}
				}
			}
		
			if (bIsBleeding)
			{
				if (!bTempStopBleeding)
				{
					BleedTime += DeltaTime;
					if (GetOwningCharacter()->HasAuthority())
						GetOwningCharacter()->Server_TakeDamage(3.0f * DeltaTime, FPointDamageEvent(3.0f * DeltaTime, FHitResult(), FVector::ZeroVector, UBleedDamageType::StaticClass()), GetOwningCharacter()->GetController(), GetOwningCharacter());
				}
			
				if (!bHasStartedPostProcess)
				{
					GetOwningCharacter()->GetPlayerPostProcessing()->StartBleedingEffect();
					bHasStartedPostProcess = true;
					SetBreathingCompBleedEvent(true);
				}
			}
			else
			{
				BleedTime = 0.0f;
				if (bHasStartedPostProcess)
				{
					GetOwningCharacter()->GetPlayerPostProcessing()->StopBleedingEffect();
					bHasStartedPostProcess = false;
					SetBreathingCompBleedEvent(false);
				}
			}
		}
	}
	else
	{
		GetOwningCharacter()->GetFMODBreathingComp()->Stop();
	}
		
}


void UBleedComponent::CompleteHeal()
{
	GetOwningCharacter()->GetPlayerPostProcessing()->StopHealingEffect();

	GetOwningCharacter()->Client_ResetPlayerEffect(UPlayerEffect_ModifyRecoil::StaticClass());

	bHasStartedHealing = false;
	
	GetOwningCharacter()->GetInventoryComponent()->EquipHolsteredItem();

	
	// First heal, Reset back to full health
	if (HealCount <= 1)
	{
		GetOwningCharacter()->GetHealthComponent()->Server_ResetResource();
	}
	// Second heal, if we're less than half health, reset back to half health (50%)
	else if (HealCount >= 2)
	{
		const float HalfHealth = GetOwningCharacter()->GetHealthComponent()->GetHalfResource();
		if (GetOwningCharacter()->GetCurrentHealth() < HalfHealth)
		{
			GetOwningCharacter()->GetHealthComponent()->Server_SetResource(HalfHealth);
		}
	}
	
	bIsBleeding = false;
	bTempStopBleeding = false;

	// Inform the server we've finished (otherwise bIsBleeding never gets updated on server, thus if we start bleeding again we get no replication update and cannot heal)
	if (GetOwningCharacter() && !GetOwningCharacter()->HasAuthority())
	{
		GetOwningCharacter()->Server_FinishHealing();
	}
}

bool UBleedComponent::CanHeal() const
{
	return bIsBleeding || (HealCount == 0 && GetOwningCharacter()->GetCurrentHealth() < 100.0f) || (HealCount == 1 && GetOwningCharacter()->GetCurrentHealth() < 50.0f);
}

void UBleedComponent::StartBleeding()
{
	bIsBleeding = true;
	bTempStopBleeding = false;
}

void UBleedComponent::StopBleeding()
{
	bIsBleeding = false;
}

void UBleedComponent::PrepareHeal()
{
	bTempStopBleeding = true;
	
	ABaseItem* EquippedItem = GetOwningCharacter()->GetEquippedItem();
    if (EquippedItem && EquippedItem->AnimationData)
    {    		
   		GetOwningCharacter()->GetInventoryComponent()->HolsterEquippedItem(false);
    	
    	if (EquippedItem->AnimationData->Holster.Body_FP)
    	{
    		float TimerLength;

    		// If holding a shield, take longer to start the heal because 1) more cumbersome, 2) shield freaks out if starting heal anim too early
    		if(Cast<ABallisticsShield>(EquippedItem))
    		{
    			TimerLength = (EquippedItem->AnimationData->Holster.Body_FP->GetPlayLength());
    		}
            else
            {
            	TimerLength = (EquippedItem->AnimationData->Holster.Body_FP->GetPlayLength() / 2.0f) * 0.7f;
            }
			GetWorld()->GetTimerManager().SetTimer(TH_DoHeal, this, &UBleedComponent::DoHeal, TimerLength, false);
    	}
    	else
    	{
    		DoHeal();
    	}
    }
}

void UBleedComponent::DoHeal()
{
	
	if (HealCount == 0)
	{
		GetOwningCharacter()->PlayMontageFromTable("fp_heal_first");
		GetOwningCharacter()->PlayMontageFromTable("tp_heal_first");
	}
	else if (HealCount == 1)
	{
		GetOwningCharacter()->PlayMontageFromTable("fp_heal_second");
		GetOwningCharacter()->PlayMontageFromTable("tp_heal_second");
	}
	else
	{
		GetOwningCharacter()->PlayMontageFromTable("fp_heal_bandage");
		GetOwningCharacter()->PlayMontageFromTable("tp_heal_bandage");
	}

	bHasStartedHealing = true;
	HealCount++;
}


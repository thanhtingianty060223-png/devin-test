// Copyright Void Interactive, 2021

#include "CharacterHealthComponent.h"

FLimbHealthData FLimbHealthData::Invalid;

UCharacterHealthComponent::UCharacterHealthComponent()
    : Super()
{
    HealthStatus = EPlayerHealthStatus::HS_Healthy;
}

void UCharacterHealthComponent::OnPawnLeavingGame()
{
    Resource = -1.0f;
}

void UCharacterHealthComponent::Server_InitResource_Implementation()
{
    Super::Server_InitResource_Implementation();

    Server_SetCurrentLimbHealthToMax(ELimbType::LT_RightLeg);
    Server_SetCurrentLimbHealthToMax(ELimbType::LT_LeftLeg);
    Server_SetCurrentLimbHealthToMax(ELimbType::LT_RightArm);
    Server_SetCurrentLimbHealthToMax(ELimbType::LT_LeftArm);
    Server_SetCurrentLimbHealthToMax(ELimbType::LT_Head);

    RemainingRevives = MaxRevives;
    RemainingReviveTime = ReviveTime;
    RemainingReviveHealth = MaxReviveHealth;

    HealthStatus = EPlayerHealthStatus::HS_Healthy;
}

void UCharacterHealthComponent::Server_SetMaxResource_Implementation(const float NewMaxResource)
{
    if (bEnableIncapacitation && !FMath::IsNearlyZero(IncapacitationHealthMultiplier, 0.0001f))
    {
        if (NewMaxResource <= 0.0f)
            return;

        MaxResource = FMath::Clamp(NewMaxResource * IncapacitationHealthMultiplier, 0.0f, MaxResourceLimit);
        OriginalMaxResource = MaxResource;

        LowResource = (MaxResource/IncapacitationHealthMultiplier) * LowResourceThreshold;
        return;
    }
        
    Super::Server_SetMaxResource_Implementation(NewMaxResource);
}

void UCharacterHealthComponent::OnComponentCreated()
{
    SetIsReplicated(true);
}

void UCharacterHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UCharacterHealthComponent, HealthStatus);

    // Limbs
    DOREPLIFETIME(UCharacterHealthComponent, RightLeg);
    DOREPLIFETIME(UCharacterHealthComponent, LeftLeg);
    DOREPLIFETIME(UCharacterHealthComponent, RightArm);
    DOREPLIFETIME(UCharacterHealthComponent, LeftArm);
    DOREPLIFETIME(UCharacterHealthComponent, Head);

    // Revive system
    DOREPLIFETIME(UCharacterHealthComponent, bUnlimitedRevives);
    DOREPLIFETIME(UCharacterHealthComponent, MaxRevives);
    DOREPLIFETIME(UCharacterHealthComponent, ReviveTime);
    DOREPLIFETIME(UCharacterHealthComponent, ReviveTimeDecrement);
    DOREPLIFETIME(UCharacterHealthComponent, ReviveOperatingTime);
    DOREPLIFETIME(UCharacterHealthComponent, MaxReviveHealth);
    DOREPLIFETIME(UCharacterHealthComponent, RemainingRevives);
    DOREPLIFETIME(UCharacterHealthComponent, RemainingReviveTime);
    DOREPLIFETIME(UCharacterHealthComponent, RemainingReviveHealth);    
}

const FLimbHealthData& UCharacterHealthComponent::GetLimb_Const(const ELimbType& Limb) const
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        return RightLeg;
        
        case ELimbType::LT_LeftLeg:
        return LeftLeg;
        
        case ELimbType::LT_RightArm:
        return RightArm;
        
        case ELimbType::LT_LeftArm:
        return LeftArm;
          
        case ELimbType::LT_Head:
        return Head;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to get limb data (read-only version). The given Limb type does not exist!");
        #endif
        return FLimbHealthData::Invalid;
    }    
}

FLimbHealthData& UCharacterHealthComponent::GetLimb(const ELimbType& Limb)
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        return RightLeg;
        
        case ELimbType::LT_LeftLeg:
        return LeftLeg;
        
        case ELimbType::LT_RightArm:
        return RightArm;
        
        case ELimbType::LT_LeftArm:
        return LeftArm;
          
        case ELimbType::LT_Head:
        return Head;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to get limb data (reference version). The given Limb type does not exist!");
        #endif
        return FLimbHealthData::Invalid;
    }    
}

FLimbHealthData UCharacterHealthComponent::GetLimb_Copy(const ELimbType& Limb) const
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        return RightLeg;
        
        case ELimbType::LT_LeftLeg:
        return LeftLeg;
        
        case ELimbType::LT_RightArm:
        return RightArm;
        
        case ELimbType::LT_LeftArm:
        return LeftArm;
          
        case ELimbType::LT_Head:
        return Head;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to get limb data (copy version). The given Limb type does not exist!");
        #endif
        return FLimbHealthData::Invalid;
    }
}

bool UCharacterHealthComponent::IsLimbBroken(const ELimbType& Limb) const
{
    switch (Limb)
    {
        case ELimbType::LT_None:
        return false;
        
        case ELimbType::LT_RightLeg:
        return RightLeg.HasNoTickets();
        
        case ELimbType::LT_LeftLeg:
        return LeftLeg.HasNoTickets();
        
        case ELimbType::LT_RightArm:
        return RightArm.HasNoTickets();
        
        case ELimbType::LT_LeftArm:
        return LeftArm.HasNoTickets();

        case ELimbType::LT_Head:
        return Head.HasNoTickets();
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error(CUR_CLASS_FUNC + " | The given Limb type does not exist!");
        #endif
        return false;
    }
}

TArray<ELimbType> UCharacterHealthComponent::GetBrokenLimbs() const
{
    TArray<ELimbType> BrokenLimbs;

    if (RightLeg.IsHealthDepleted() || RightLeg.HasNoTickets())
        BrokenLimbs.Add(ELimbType::LT_RightLeg);

    if (LeftLeg.IsHealthDepleted() || LeftLeg.HasNoTickets())
        BrokenLimbs.Add(ELimbType::LT_LeftLeg);
    
    if (RightArm.IsHealthDepleted() || RightArm.HasNoTickets())
        BrokenLimbs.Add(ELimbType::LT_RightArm);
    
    if (LeftArm.IsHealthDepleted() || LeftArm.HasNoTickets())
        BrokenLimbs.Add(ELimbType::LT_LeftArm);
    
    if (Head.IsHealthDepleted() || Head.HasNoTickets())
        BrokenLimbs.Add(ELimbType::LT_Head);

    return BrokenLimbs;
}

bool UCharacterHealthComponent::IsLimbFullHealth(const ELimbType& Limb) const
{
    switch (Limb)
    {
        case ELimbType::LT_None:
        return false;
        
        case ELimbType::LT_RightLeg:
        return RightLeg.IsFullHealth();
        
        case ELimbType::LT_LeftLeg:
        return LeftLeg.IsFullHealth();
        
        case ELimbType::LT_RightArm:
        return RightArm.IsFullHealth();
        
        case ELimbType::LT_LeftArm:
        return LeftArm.IsFullHealth();

        case ELimbType::LT_Head:
        return Head.IsFullHealth();
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error(CUR_CLASS_FUNC + " | The given Limb type does not exist!");
        #endif
        return false;
    }
}

bool UCharacterHealthComponent::IsLimbNoHealth(const ELimbType& Limb) const
{
    switch (Limb)
    {
        case ELimbType::LT_None:
        return false;
        
        case ELimbType::LT_RightLeg:
        return RightLeg.IsHealthDepleted();
        
        case ELimbType::LT_LeftLeg:
        return LeftLeg.IsHealthDepleted();
        
        case ELimbType::LT_RightArm:
        return RightArm.IsHealthDepleted();
        
        case ELimbType::LT_LeftArm:
        return LeftArm.IsHealthDepleted();

        case ELimbType::LT_Head:
        return Head.IsHealthDepleted();
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error(CUR_CLASS_FUNC + " | The given Limb type does not exist!");
        #endif
        return false;
    }
}

bool UCharacterHealthComponent::IsLimbLowHealth(const ELimbType& Limb) const
{
    switch (Limb)
    {
        case ELimbType::LT_None:
        return false;
        
        case ELimbType::LT_RightLeg:
        return RightLeg.IsLowHealth();
        
        case ELimbType::LT_LeftLeg:
        return LeftLeg.IsLowHealth();
        
        case ELimbType::LT_RightArm:
        return RightArm.IsLowHealth();
        
        case ELimbType::LT_LeftArm:
        return LeftArm.IsLowHealth();
        
        case ELimbType::LT_Head:
        return Head.IsLowHealth();
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error(CUR_CLASS_FUNC + " | The given Limb type does not exist!");
        #endif
        return false;
    }
}

bool UCharacterHealthComponent::IsLimbHealthAt(const ELimbType& Limb, const float HealthValue) const
{
    switch (Limb)
    {
        case ELimbType::LT_None:
        return false;

        case ELimbType::LT_RightLeg:
        return RightLeg.IsHealthAt(HealthValue);
        
        case ELimbType::LT_LeftLeg:
        return LeftLeg.IsHealthAt(HealthValue);
        
        case ELimbType::LT_RightArm:
        return RightArm.IsHealthAt(HealthValue);
        
        case ELimbType::LT_LeftArm:
        return LeftArm.IsHealthAt(HealthValue);
        
        case ELimbType::LT_Head:
        return Head.IsHealthAt(HealthValue);
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error(CUR_CLASS_FUNC + " | The given Limb type does not exist!");
        #endif
        return false;
    }
}

bool UCharacterHealthComponent::IsLimbHealthBelow(const ELimbType& Limb, const float HealthValue) const
{
    switch (Limb)
    {
        case ELimbType::LT_None:
        return false;
        
        case ELimbType::LT_RightLeg:
        return RightLeg.IsHealthBelow(HealthValue);
        
        case ELimbType::LT_LeftLeg:
        return LeftLeg.IsHealthBelow(HealthValue);
        
        case ELimbType::LT_RightArm:
        return RightArm.IsHealthBelow(HealthValue);
        
        case ELimbType::LT_LeftArm:
        return LeftArm.IsHealthBelow(HealthValue);
        
        case ELimbType::LT_Head:
        return Head.IsHealthBelow(HealthValue);
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error(CUR_CLASS_FUNC + " | The given Limb type does not exist!");
        #endif
        return false;
    }
}

bool UCharacterHealthComponent::IsLimbHealthAbove(const ELimbType& Limb, const float HealthValue) const
{
    switch (Limb)
    {
        case ELimbType::LT_None:
        return false;
        
        case ELimbType::LT_RightLeg:
        return RightLeg.IsHealthAbove(HealthValue);
        
        case ELimbType::LT_LeftLeg:
        return LeftLeg.IsHealthAbove(HealthValue);
        
        case ELimbType::LT_RightArm:
        return RightArm.IsHealthAbove(HealthValue);
        
        case ELimbType::LT_LeftArm:
        return LeftArm.IsHealthAbove(HealthValue);
        
        case ELimbType::LT_Head:
        return Head.IsHealthAbove(HealthValue);
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error(CUR_CLASS_FUNC + " | The given Limb type does not exist!");
        #endif
        return false;
    }
}

bool UCharacterHealthComponent::IsLimbHealthAtOrBelow(const ELimbType& Limb, const float HealthValue) const
{
    switch (Limb)
    {
        case ELimbType::LT_None:
        return false;
        
        case ELimbType::LT_RightLeg:
        return RightLeg.IsHealthAtOrBelow(HealthValue);
        
        case ELimbType::LT_LeftLeg:
        return LeftLeg.IsHealthAtOrBelow(HealthValue);
        
        case ELimbType::LT_RightArm:
        return RightArm.IsHealthAtOrBelow(HealthValue);
        
        case ELimbType::LT_LeftArm:
        return LeftArm.IsHealthAtOrBelow(HealthValue);
        
        case ELimbType::LT_Head:
        return Head.IsHealthAtOrBelow(HealthValue);
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error(CUR_CLASS_FUNC + " | The given Limb type does not exist!");
        #endif
        return false;
    }
}

bool UCharacterHealthComponent::IsLimbHealthAtOrAbove(const ELimbType& Limb, const float HealthValue) const
{
    switch (Limb)
    {
        case ELimbType::LT_None:
        return false;
        
        case ELimbType::LT_RightLeg:
        return RightLeg.IsHealthAtOrAbove(HealthValue);
        
        case ELimbType::LT_LeftLeg:
        return LeftLeg.IsHealthAtOrAbove(HealthValue);
        
        case ELimbType::LT_RightArm:
        return RightArm.IsHealthAtOrAbove(HealthValue);
        
        case ELimbType::LT_LeftArm:
        return LeftArm.IsHealthAtOrAbove(HealthValue);
        
        case ELimbType::LT_Head:
        return Head.IsHealthAtOrAbove(HealthValue);
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error(CUR_CLASS_FUNC + " | The given Limb type does not exist!");
        #endif
        return false;
    }
}

bool UCharacterHealthComponent::IsAnyLimbBelowFullHealth(ELimbType& OutLimbType) const
{
    if (RightArm.IsHealthBelow(RightArm.GetMaxHealth()))
    {
        OutLimbType = ELimbType::LT_RightArm;
        return true;
    }
        
    if (LeftArm.IsHealthBelow(LeftArm.GetMaxHealth()))
    {
        OutLimbType = ELimbType::LT_LeftArm;
        return true;
    }
        
    if (RightLeg.IsHealthBelow(RightLeg.GetMaxHealth()))
    {
        OutLimbType = ELimbType::LT_RightLeg;
        return true;
    }
        
    if (LeftLeg.IsHealthBelow(LeftLeg.GetMaxHealth()))
    {
        OutLimbType = ELimbType::LT_LeftLeg;
        return true;
    }

    if (Head.IsHealthBelow(Head.GetMaxHealth()))
    {
        OutLimbType = ELimbType::LT_Head;
        return true;
    }

    OutLimbType = ELimbType::LT_None;
    return false;  
}

bool UCharacterHealthComponent::IsAnyLimbAtNoHealth(ELimbType& OutLimbType) const
{
    if (RightArm.IsHealthDepleted())
    {
        OutLimbType = ELimbType::LT_RightArm;
        return true;
    }

    if (LeftArm.IsHealthDepleted())
    {
        OutLimbType = ELimbType::LT_LeftArm;
        return true;
    }

    if (RightLeg.IsHealthDepleted())
    {
        OutLimbType = ELimbType::LT_RightLeg;
        return true;
    }

    if (LeftLeg.IsHealthDepleted())
    {
        OutLimbType = ELimbType::LT_LeftLeg;
        return true;
    }
    
    if (Head.IsHealthDepleted())
    {
        OutLimbType = ELimbType::LT_Head;
        return true;
    }

    OutLimbType = ELimbType::LT_None;
    return false;  
}

bool UCharacterHealthComponent::IsAnyLimbBroken(ELimbType& OutLimbType) const
{
    if (RightArm.HasNoTickets())
    {
        OutLimbType = ELimbType::LT_RightArm;
        return true;
    }

    if (LeftArm.HasNoTickets())
    {
        OutLimbType = ELimbType::LT_LeftArm;
        return true;
    }

    if (RightLeg.HasNoTickets())
    {
        OutLimbType = ELimbType::LT_RightLeg;
        return true;
    }

    if (LeftLeg.HasNoTickets())
    {
        OutLimbType = ELimbType::LT_LeftLeg;
        return true;
    }
    
    if (Head.HasNoTickets())
    {
        OutLimbType = ELimbType::LT_Head;
        return true;
    }

    OutLimbType = ELimbType::LT_None;
    return false;    
}

bool UCharacterHealthComponent::HalfMaxLimbHealth(const ELimbType& Limb)
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        if (RightLeg.CurrentLimbHealthHalvings < RightLeg.GetMaxLimbHealthHalving())
        {
            RightLeg.SetMaxHealth(RightLeg.GetHalfHealth());
            RightLeg.CurrentLimbHealthHalvings++;
            BroadcastOnFullLimbHealth(RightLeg, ELimbType::LT_RightLeg);
            return true;
        }
        return false;
        
        case ELimbType::LT_LeftLeg:
        if (LeftLeg.CurrentLimbHealthHalvings < LeftLeg.GetMaxLimbHealthHalving())
        {
            LeftLeg.SetMaxHealth(LeftLeg.GetHalfHealth());
            LeftLeg.CurrentLimbHealthHalvings++;
            BroadcastOnFullLimbHealth(LeftLeg, ELimbType::LT_LeftLeg);
            return true;
        }
        return false;
        
        case ELimbType::LT_RightArm:
        if (RightArm.CurrentLimbHealthHalvings < RightArm.GetMaxLimbHealthHalving())
        {
            RightArm.SetMaxHealth(RightArm.GetHalfHealth());
            RightArm.CurrentLimbHealthHalvings++;
            BroadcastOnFullLimbHealth(RightArm, ELimbType::LT_RightArm);
            return true;
        }
        return false;
        
        case ELimbType::LT_LeftArm:
        if (LeftArm.CurrentLimbHealthHalvings < LeftArm.GetMaxLimbHealthHalving())
        {
            LeftArm.SetMaxHealth(LeftArm.GetHalfHealth());
            LeftArm.CurrentLimbHealthHalvings++;
            BroadcastOnFullLimbHealth(LeftArm, ELimbType::LT_LeftArm);
            return true;
        }
        return false;

        case ELimbType::LT_Head:
        if (Head.CurrentLimbHealthHalvings < Head.GetMaxLimbHealthHalving())
        {
            Head.SetMaxHealth(Head.GetHalfHealth());
            Head.CurrentLimbHealthHalvings++;
            BroadcastOnFullLimbHealth(Head, ELimbType::LT_Head);
            return true;
        }
        return false;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to set a new max health to limb. The given Limb type does not exist!");
        #endif
        return false;
    }   
}

void UCharacterHealthComponent::Server_SetMaxLimbHealth_Implementation(const ELimbType& Limb, const float NewMaxHealth)
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        RightLeg.SetMaxHealth(NewMaxHealth);
        BroadcastOnFullLimbHealth(RightLeg, ELimbType::LT_RightLeg);
        break;
        
        case ELimbType::LT_LeftLeg:
        LeftLeg.SetMaxHealth(NewMaxHealth);
        BroadcastOnFullLimbHealth(LeftLeg, ELimbType::LT_LeftLeg);
        break;
        
        case ELimbType::LT_RightArm:
        RightArm.SetMaxHealth(NewMaxHealth);
        BroadcastOnFullLimbHealth(RightArm, ELimbType::LT_RightArm);
        break;
        
        case ELimbType::LT_LeftArm:
        LeftArm.SetMaxHealth(NewMaxHealth);
        BroadcastOnFullLimbHealth(LeftArm, ELimbType::LT_LeftArm);
        break;
          
        case ELimbType::LT_Head:
        Head.SetMaxHealth(NewMaxHealth);
        BroadcastOnFullLimbHealth(Head, ELimbType::LT_Head);
        break;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to set max health to limb. The given Limb type does not exist!");
        #endif
        break;
    }
}

void UCharacterHealthComponent::Server_HalfMaxLimbHealth_Implementation(const ELimbType& Limb)
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        if (RightLeg.CurrentLimbHealthHalvings < RightLeg.GetMaxLimbHealthHalving())
        {
            RightLeg.SetMaxHealth(RightLeg.GetHalfHealth());
            RightLeg.CurrentLimbHealthHalvings++;
            BroadcastOnFullLimbHealth(RightLeg, ELimbType::LT_RightLeg);
        }
        break;
        
        case ELimbType::LT_LeftLeg:
        if (LeftLeg.CurrentLimbHealthHalvings < LeftLeg.GetMaxLimbHealthHalving())
        {
            LeftLeg.SetMaxHealth(LeftLeg.GetHalfHealth());
            LeftLeg.CurrentLimbHealthHalvings++;
            BroadcastOnFullLimbHealth(LeftLeg, ELimbType::LT_LeftLeg);
        }
        break;
        
        case ELimbType::LT_RightArm:
        if (RightArm.CurrentLimbHealthHalvings < RightArm.GetMaxLimbHealthHalving())
        {
            RightArm.SetMaxHealth(RightArm.GetHalfHealth());
            RightArm.CurrentLimbHealthHalvings++;
            BroadcastOnFullLimbHealth(RightArm, ELimbType::LT_RightArm);
        }
        break;
        
        case ELimbType::LT_LeftArm:
        if (LeftArm.CurrentLimbHealthHalvings < LeftArm.GetMaxLimbHealthHalving())
        {
            LeftArm.SetMaxHealth(LeftArm.GetHalfHealth());
            LeftArm.CurrentLimbHealthHalvings++;
            BroadcastOnFullLimbHealth(LeftArm, ELimbType::LT_LeftArm);
        }
        break;

        case ELimbType::LT_Head:
        if (Head.CurrentLimbHealthHalvings < Head.GetMaxLimbHealthHalving())
        {
            Head.SetMaxHealth(Head.GetHalfHealth());
            Head.CurrentLimbHealthHalvings++;
            BroadcastOnFullLimbHealth(Head, ELimbType::LT_Head);
        }
        break;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to set a new max health to limb. The given Limb type does not exist!");
        #endif
        break;
    }
}

void UCharacterHealthComponent::Server_SetLimbHealth_Implementation(const ELimbType& Limb, const float NewHealthAmount)
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        RightLeg.SetHealth(NewHealthAmount);
        BroadcastOnFullLimbHealth(RightLeg, ELimbType::LT_RightLeg);
        BroadcastOnLowLimbHealth(RightLeg, ELimbType::LT_RightLeg, RightLeg.GetCurrentHealth());
        BroadcastOnNoLimbHealth(RightLeg, ELimbType::LT_RightLeg);
        BroadcastOnLimbBroken(RightLeg, ELimbType::LT_RightLeg);
        break;
        
        case ELimbType::LT_LeftLeg:
        LeftLeg.SetHealth(NewHealthAmount);
        BroadcastOnFullLimbHealth(LeftLeg, ELimbType::LT_LeftLeg);
        BroadcastOnLowLimbHealth(LeftLeg, ELimbType::LT_LeftLeg, LeftLeg.GetCurrentHealth());
        BroadcastOnNoLimbHealth(LeftLeg, ELimbType::LT_LeftLeg);
        BroadcastOnLimbBroken(LeftLeg, ELimbType::LT_LeftLeg);
        break;
        
        case ELimbType::LT_RightArm:
        RightArm.SetHealth(NewHealthAmount);
        BroadcastOnFullLimbHealth(RightArm, ELimbType::LT_RightArm);
        BroadcastOnLowLimbHealth(RightArm, ELimbType::LT_RightArm, RightArm.GetCurrentHealth());
        BroadcastOnNoLimbHealth(RightArm, ELimbType::LT_RightArm);
        BroadcastOnLimbBroken(RightArm, ELimbType::LT_RightArm);
        break;
        
        case ELimbType::LT_LeftArm:
        LeftArm.SetHealth(NewHealthAmount);
        BroadcastOnFullLimbHealth(LeftArm, ELimbType::LT_LeftArm);
        BroadcastOnLowLimbHealth(LeftArm, ELimbType::LT_LeftArm, LeftArm.GetCurrentHealth());
        BroadcastOnNoLimbHealth(LeftArm, ELimbType::LT_LeftArm);
        BroadcastOnLimbBroken(LeftArm, ELimbType::LT_LeftArm);
        break;

        case ELimbType::LT_Head:
        Head.SetHealth(NewHealthAmount);
        BroadcastOnFullLimbHealth(Head, ELimbType::LT_Head);
        BroadcastOnLowLimbHealth(Head, ELimbType::LT_Head, Head.GetCurrentHealth());
        BroadcastOnNoLimbHealth(Head, ELimbType::LT_Head);
        BroadcastOnLimbBroken(Head, ELimbType::LT_Head);
        break;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to set health to limb. The given Limb type does not exist!");
        #endif
        break;
    }
}

void UCharacterHealthComponent::Server_SetCurrentLimbHealthToMax_Implementation(const ELimbType& Limb)
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        RightLeg.SetHealthToMax();
        BroadcastOnFullLimbHealth(RightLeg, ELimbType::LT_RightLeg);
        break;
        
        case ELimbType::LT_LeftLeg:
        LeftLeg.SetHealthToMax();
        BroadcastOnFullLimbHealth(LeftLeg, ELimbType::LT_LeftLeg);
        break;
        
        case ELimbType::LT_RightArm:
        RightArm.SetHealthToMax();
        BroadcastOnFullLimbHealth(RightArm, ELimbType::LT_RightArm);
        break;
        
        case ELimbType::LT_LeftArm:
        LeftArm.SetHealthToMax();
        BroadcastOnFullLimbHealth(LeftArm, ELimbType::LT_LeftArm);
        break;

        case ELimbType::LT_Head:
        Head.SetHealthToMax();
        BroadcastOnFullLimbHealth(Head, ELimbType::LT_Head);
        break;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to set health to limb. The given Limb type does not exist!");
        #endif
        break;
    }
}

void UCharacterHealthComponent::Server_UpdatePreviousLimbHealth_Implementation(const ELimbType& Limb)
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        RightLeg.UpdatePreviousHealth();
        break;
        
        case ELimbType::LT_LeftLeg:
        LeftLeg.UpdatePreviousHealth();
        break;
        
        case ELimbType::LT_RightArm:
        RightArm.UpdatePreviousHealth();
        break;
        
        case ELimbType::LT_LeftArm:
        LeftArm.UpdatePreviousHealth();
        break;

        case ELimbType::LT_Head:
        Head.UpdatePreviousHealth();
        break;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to update previous health to limb. The given Limb type does not exist!");
        #endif
        break;
    }
}

void UCharacterHealthComponent::Server_IncreaseLimbHealth_Implementation(const ELimbType& Limb, const float Amount)
{    
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        RightLeg.IncreaseHealth(Amount);
        BroadcastOnFullLimbHealth(RightLeg, ELimbType::LT_RightLeg);
        break;
        
        case ELimbType::LT_LeftLeg:
        LeftLeg.IncreaseHealth(Amount);
        BroadcastOnFullLimbHealth(LeftLeg, ELimbType::LT_LeftLeg);
        break;
        
        case ELimbType::LT_RightArm:
        RightArm.IncreaseHealth(Amount);
        BroadcastOnFullLimbHealth(RightArm, ELimbType::LT_RightArm);
        break;
        
        case ELimbType::LT_LeftArm:
        LeftArm.IncreaseHealth(Amount);
        BroadcastOnFullLimbHealth(LeftArm, ELimbType::LT_LeftArm);
        break;
        
        case ELimbType::LT_Head:
        Head.IncreaseHealth(Amount);
        BroadcastOnFullLimbHealth(Head, ELimbType::LT_Head);
        break;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to increase health to limb. The given Limb type does not exist!");
        #endif
        break;
    }
}

void UCharacterHealthComponent::Server_DecreaseLimbHealth_Implementation(const ELimbType& Limb, const float Amount)
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        RightLeg.DecreaseHealth(Amount);
        BroadcastOnLowLimbHealth(RightLeg, ELimbType::LT_RightLeg, RightLeg.GetCurrentHealth());
        BroadcastOnNoLimbHealth(RightLeg, ELimbType::LT_RightLeg);
        BroadcastOnLimbBroken(RightLeg, ELimbType::LT_RightLeg);
        break;
        
        case ELimbType::LT_LeftLeg:
        LeftLeg.DecreaseHealth(Amount);
        BroadcastOnLowLimbHealth(LeftLeg, ELimbType::LT_LeftLeg, LeftLeg.GetCurrentHealth());
        BroadcastOnNoLimbHealth(LeftLeg, ELimbType::LT_LeftLeg);
        BroadcastOnLimbBroken(LeftLeg, ELimbType::LT_LeftLeg);
        break;
        
        case ELimbType::LT_RightArm:
        RightArm.DecreaseHealth(Amount);
        BroadcastOnLowLimbHealth(RightArm, ELimbType::LT_RightArm, RightArm.GetCurrentHealth());
        BroadcastOnNoLimbHealth(RightArm, ELimbType::LT_RightArm);
        BroadcastOnLimbBroken(RightArm, ELimbType::LT_RightArm);
        break;
        
        case ELimbType::LT_LeftArm:
        LeftArm.DecreaseHealth(Amount);
        BroadcastOnLowLimbHealth(LeftArm, ELimbType::LT_LeftArm, LeftArm.GetCurrentHealth());
        BroadcastOnNoLimbHealth(LeftArm, ELimbType::LT_LeftArm);
        BroadcastOnLimbBroken(LeftArm, ELimbType::LT_LeftArm);
        break;

        case ELimbType::LT_Head:
        Head.DecreaseHealth(Amount);
        BroadcastOnLowLimbHealth(Head, ELimbType::LT_Head, Head.GetCurrentHealth());
        BroadcastOnNoLimbHealth(Head, ELimbType::LT_Head);
        BroadcastOnLimbBroken(Head, ELimbType::LT_Head);
        break;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to decrease health to limb. The given Limb type does not exist!");
        #endif
        break;
    }
}

void UCharacterHealthComponent::Server_ResetLimbHealth_Implementation(const ELimbType& Limb)
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        RightLeg.ResetHealth();
        BroadcastOnFullLimbHealth(RightLeg, ELimbType::LT_RightLeg);
        break;
        
        case ELimbType::LT_LeftLeg:
        LeftLeg.ResetHealth();
        BroadcastOnFullLimbHealth(LeftLeg, ELimbType::LT_LeftLeg);
        break;
        
        case ELimbType::LT_RightArm:
        RightArm.ResetHealth();
        BroadcastOnFullLimbHealth(RightArm, ELimbType::LT_RightArm);
        break;
        
        case ELimbType::LT_LeftArm:
        LeftArm.ResetHealth();
        BroadcastOnFullLimbHealth(LeftArm, ELimbType::LT_LeftArm);
        break;

        case ELimbType::LT_Head:
        Head.ResetHealth();
        BroadcastOnFullLimbHealth(Head, ELimbType::LT_Head);
        break;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to reset health to limb. The given Limb type does not exist!");
        #endif
        break;
    }
}

void UCharacterHealthComponent::Server_IncreaseLimbTickets_Implementation(const ELimbType& Limb, const int32 Amount)
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        RightLeg.IncreaseTickets(Amount);
        BroadcastOnFullTickets(RightLeg, ELimbType::LT_RightLeg);
        break;
        
        case ELimbType::LT_LeftLeg:
        LeftLeg.IncreaseTickets(Amount);
        BroadcastOnFullTickets(LeftLeg, ELimbType::LT_LeftLeg);
        break;
        
        case ELimbType::LT_RightArm:
        RightArm.IncreaseTickets(Amount);
        BroadcastOnFullTickets(RightArm, ELimbType::LT_RightArm);
        break;
        
        case ELimbType::LT_LeftArm:
        LeftArm.IncreaseTickets(Amount);
        BroadcastOnFullTickets(LeftArm, ELimbType::LT_LeftArm);
        break;

        case ELimbType::LT_Head:
        Head.IncreaseTickets(Amount);
        BroadcastOnFullTickets(Head, ELimbType::LT_Head);
        break;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to increase tickets to limb. The given Limb type does not exist!");
        #endif
        break;
    } 
}

void UCharacterHealthComponent::Server_DecreaseLimbTickets_Implementation(const ELimbType& Limb, const int32 Amount)
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        RightLeg.DecreaseTickets(Amount);
        BroadcastOnNoTickets(RightLeg, ELimbType::LT_RightLeg);
        break;
        
        case ELimbType::LT_LeftLeg:
        LeftLeg.DecreaseTickets(Amount);
        BroadcastOnNoTickets(LeftLeg, ELimbType::LT_LeftLeg);
        break;
        
        case ELimbType::LT_RightArm:
        RightArm.DecreaseTickets(Amount);
        BroadcastOnNoTickets(RightArm, ELimbType::LT_RightArm);
        break;
        
        case ELimbType::LT_LeftArm:
        LeftArm.DecreaseTickets(Amount);
        BroadcastOnNoTickets(LeftArm, ELimbType::LT_LeftArm);
        break;

        case ELimbType::LT_Head:
        Head.DecreaseTickets(Amount);
        BroadcastOnNoTickets(Head, ELimbType::LT_Head);
        break;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to decrease tickets to limb. The given Limb type does not exist!");
        #endif
        break;
    } 
}

void UCharacterHealthComponent::Server_UseAllRemainingLimbTickets_Implementation(const ELimbType& Limb)
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        RightLeg.EmptyRemainingTickets();
        BroadcastOnNoTickets(RightLeg, ELimbType::LT_RightLeg);
        break;
        
        case ELimbType::LT_LeftLeg:
        LeftLeg.EmptyRemainingTickets();
        BroadcastOnNoTickets(LeftLeg, ELimbType::LT_LeftLeg);
        break;
        
        case ELimbType::LT_RightArm:
        RightArm.EmptyRemainingTickets();
        BroadcastOnNoTickets(RightArm, ELimbType::LT_RightArm);
        break;
        
        case ELimbType::LT_LeftArm:
        LeftArm.EmptyRemainingTickets();
        BroadcastOnNoTickets(LeftArm, ELimbType::LT_LeftArm);
        break;

        case ELimbType::LT_Head:
        Head.EmptyRemainingTickets();
        BroadcastOnNoTickets(Head, ELimbType::LT_Head);
        break;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to remove all remaining tickets to limb. The given Limb type does not exist!");
        #endif
        break;
    }
}

void UCharacterHealthComponent::Server_ResetLimbTickets_Implementation(const ELimbType& Limb)
{
    switch (Limb)
    {
        case ELimbType::LT_RightLeg:
        RightLeg.ResetTickets();
        BroadcastOnFullTickets(RightLeg, ELimbType::LT_RightLeg);
        break;
        
        case ELimbType::LT_LeftLeg:
        LeftLeg.ResetTickets();
        BroadcastOnFullTickets(LeftLeg, ELimbType::LT_LeftLeg);
        break;
        
        case ELimbType::LT_RightArm:
        RightArm.ResetTickets();
        BroadcastOnFullTickets(RightArm, ELimbType::LT_RightArm);
        break;
        
        case ELimbType::LT_LeftArm:
        LeftArm.ResetTickets();
        BroadcastOnFullTickets(LeftArm, ELimbType::LT_LeftArm);
        break;

        case ELimbType::LT_Head:
        Head.ResetTickets();
        BroadcastOnFullTickets(Head, ELimbType::LT_Head);
        break;
        
        default:
        #if !UE_BUILD_SHIPPING
        ULog::Error("Failed to reset tickets to limb. The given Limb type does not exist!");
        #endif
        break;
    } 
}

void UCharacterHealthComponent::Server_ResetAllLimbHealth_Implementation()
{
    RightLeg.ResetHealth();
    LeftLeg.ResetHealth();
    LeftArm.ResetHealth();
    RightArm.ResetHealth();
    Head.ResetHealth();
}

void UCharacterHealthComponent::Server_ResetAllLimbTickets_Implementation()
{
    RightLeg.ResetTickets();
    LeftLeg.ResetTickets();
    LeftArm.ResetTickets();
    RightArm.ResetTickets();
    Head.ResetTickets();
}

void UCharacterHealthComponent::Server_IncreaseRevive_Implementation()
{
    if (bUnlimitedRevives)
        return;
 
    RemainingRevives = FMath::Clamp(RemainingRevives + 1, 0, MaxRevives);
    RemainingReviveTime = FMath::Clamp(RemainingReviveTime + ReviveTimeDecrement, 0.0f, ReviveTime);
}

void UCharacterHealthComponent::Server_DecreaseRevive_Implementation()
{
    if (bUnlimitedRevives)
        return;
    
    RemainingRevives = FMath::Clamp(RemainingRevives - 1, 0, MaxRevives);
    RemainingReviveTime = FMath::Clamp(RemainingReviveTime - ReviveTimeDecrement, 0.0f, ReviveTime);
}

void UCharacterHealthComponent::Server_SetRemainingRevives_Implementation(const int32 NewRemainingRevives)
{
    if (bUnlimitedRevives)
        return;

    RemainingRevives = FMath::Clamp(NewRemainingRevives, 0, MaxRevives);
    RemainingReviveTime = FMath::Clamp(ReviveTimeDecrement * RemainingRevives, 0.0f, ReviveTime);
}

void UCharacterHealthComponent::Server_ResetRevives_Implementation()
{
    if (bUnlimitedRevives)
        return;

    RemainingRevives = MaxRevives;
    RemainingReviveTime = ReviveTime;
}

void UCharacterHealthComponent::Server_IncreaseReviveHealth_Implementation(const float Amount)
{
    RemainingReviveHealth = FMath::Clamp(RemainingReviveHealth + Amount, 0.0f, MaxReviveHealth);
}

void UCharacterHealthComponent::Server_DecreaseReviveHealth_Implementation(const float Amount)
{
    RemainingReviveHealth = FMath::Clamp(RemainingReviveHealth - Amount, 0.0f, MaxReviveHealth);
}

void UCharacterHealthComponent::Server_SetReviveHealth_Implementation(const float NewReviveHealth)
{
    RemainingReviveHealth = FMath::Clamp(NewReviveHealth, 0.0f, MaxReviveHealth);
}

void UCharacterHealthComponent::Server_ResetReviveHealth_Implementation()
{
    RemainingReviveHealth = MaxReviveHealth;
}

void UCharacterHealthComponent::SetHealthStatus(const EPlayerHealthStatus NewHealthStatus)
{
    HealthStatus = NewHealthStatus;
}

void UCharacterHealthComponent::Server_DecreaseResource_Implementation(const float Amount)
{
    Super::Server_DecreaseResource_Implementation(Amount);
    
    if (IsFullResource())
    {
        SetHealthStatus(EPlayerHealthStatus::HS_Healthy);
    }
    else if (bEnableIncapacitation && IsResourceAtOrBelow(MaxResource/IncapacitationHealthMultiplier) && Resource > 0.0f)
    {
        SetHealthStatus(EPlayerHealthStatus::HS_Incapacitated);
    }
    else
    {
        SetHealthStatus(EPlayerHealthStatus::HS_Injured);
    }

    if (IsDepleted())
        SetHealthStatus(EPlayerHealthStatus::HS_Dead);
}

bool UCharacterHealthComponent::CanUseReviveSystem() const
{
    if (!GetWorld())
        return false;
    
    if (!GetOwner())
        return false;
        
    if (AReadyOrNotGameState* RONGS = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState()))
    {
        return RONGS->bRevivesAllowed;
    }

    return false;
}

void UCharacterHealthComponent::BroadcastOnFullLimbHealth(FLimbHealthData& InLimbData, const ELimbType& LimbType)
{
    if (InLimbData.GetCurrentHealth() > InLimbData.GetLowHealth())
    {
        InLimbData.bHasLowHealthEventBeenBroadcasted = false;
        InLimbData.bHasOnBrokenEventBeenBroadcasted = false;
    }
    
    if (InLimbData.IsFullHealth() && !InLimbData.bHasFullHealthEventBeenBroadcasted)
    {
        OnLimbFullHealth.Broadcast(LimbType);
        InLimbData.bHasFullHealthEventBeenBroadcasted = true;
    }
}

void UCharacterHealthComponent::BroadcastOnLowLimbHealth(FLimbHealthData& InLimbData, const ELimbType& LimbType, const float& InCurrentLimbHealth)
{
    if (InLimbData.GetCurrentHealth() < InLimbData.GetMaxHealth())
    {
        InLimbData.bHasFullHealthEventBeenBroadcasted = false;
    }

    if (InLimbData.IsLowHealth() && !InLimbData.bHasLowHealthEventBeenBroadcasted)
    {
        OnLimbLowHealth.Broadcast(LimbType, InCurrentLimbHealth);
        InLimbData.bHasLowHealthEventBeenBroadcasted = true;
    }
}

void UCharacterHealthComponent::BroadcastOnNoLimbHealth(FLimbHealthData& InLimbData, const ELimbType& LimbType)
{
    if (InLimbData.IsHealthDepleted() && !InLimbData.bHasNoHealthEventBeenBroadcasted)
    {
        OnLimbNoHealth.Broadcast(LimbType);
        InLimbData.bHasNoHealthEventBeenBroadcasted = true;
    }
}

void UCharacterHealthComponent::BroadcastOnLimbBroken(FLimbHealthData& InLimbData, const ELimbType& LimbType)
{
    if (InLimbData.HasNoTickets() && !InLimbData.bHasOnBrokenEventBeenBroadcasted)
    {
        OnLimbBroken.Broadcast(LimbType);
        InLimbData.bHasOnBrokenEventBeenBroadcasted = true;
    }
}

void UCharacterHealthComponent::BroadcastOnFullTickets(FLimbHealthData& InLimbData, const ELimbType& LimbType)
{
    if (InLimbData.GetRemainingTickets() > 0)
    {
        InLimbData.bHasOnNoTicketsRemainingEventBeenBroadcasted = false;
    }
    
    if (InLimbData.HasMaxTickets() && !InLimbData.bHasOnFullTicketsEventBeenBroadcasted)
    {
        OnLimbFullTickets.Broadcast(LimbType);
        InLimbData.bHasOnFullTicketsEventBeenBroadcasted = true;
    }
}

void UCharacterHealthComponent::BroadcastOnNoTickets(FLimbHealthData& InLimbData, const ELimbType& LimbType)
{
    if (InLimbData.GetRemainingTickets() < InLimbData.GetMaxTickets())
    {
        InLimbData.bHasOnFullTicketsEventBeenBroadcasted = false;
    }

    if (InLimbData.HasNoTickets() && !InLimbData.bHasOnNoTicketsRemainingEventBeenBroadcasted)
    {
        OnLimbNoTickets.Broadcast(LimbType);
        InLimbData.bHasOnNoTicketsRemainingEventBeenBroadcasted = true;
    }
}

void FLimbHealthData::IncreaseHealth(const float Amount)
{
    if (Amount <= 0.0f)
        return;
	
    PreviousHealth = Health;

    Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
}

void FLimbHealthData::IncreaseTickets(const int32 Amount)
{
    if (Amount <= 0)
        return;
    
    Tickets = FMath::Clamp(Tickets + Amount, 0, MaxTickets);
}

void FLimbHealthData::DecreaseHealth(const float Amount)
{
    if (Amount <= 0.0f)
        return;

    PreviousHealth = Health;

    Health = FMath::Clamp(Health - Amount * LimbDamageMultiplier, 0.0f, MaxHealth);
}

void FLimbHealthData::DecreaseTickets(const int32 Amount)
{
    if (Amount <= 0)
        return;
    
    Tickets = FMath::Clamp(Tickets - 1, 0, MaxTickets);
}

void FLimbHealthData::EmptyRemainingTickets()
{
    Tickets = 0;
}

void FLimbHealthData::ResetTickets()
{
    Tickets = MaxTickets;
}

void FLimbHealthData::SetMaxHealth(const float NewMaxHealth)
{
    if (NewMaxHealth <= 0.0f)
        return;

    MaxHealth = FMath::Clamp(NewMaxHealth, 1.0f, MaxHealthLimit);

    LowHealth = MaxHealth * LowHealthThreshold;
}

void FLimbHealthData::SetHealth(const float Amount)
{
	PreviousHealth = Health;

	Health = FMath::Clamp(Amount, 0.0f, MaxHealth);
}

FLimbHealthData::FLimbHealthData()
    : bHasFullHealthEventBeenBroadcasted(false),
      bHasLowHealthEventBeenBroadcasted(false),
      bHasNoHealthEventBeenBroadcasted(false),
      bHasOnBrokenEventBeenBroadcasted(false),
      bHasOnFullTicketsEventBeenBroadcasted(false),
      bHasOnNoTicketsRemainingEventBeenBroadcasted(false)
{
}

bool FLimbHealthData::IsInvalid() const
{
    return 	GetCurrentHealth() == Invalid.GetCurrentHealth() &&
            GetMaxHealth() == Invalid.GetMaxHealth() &&
            GetLowHealth() == Invalid.GetLowHealth() &&
            GetPreviousHealth() == Invalid.GetPreviousHealth() &&
            GetRemainingTickets() == Invalid.GetRemainingTickets() &&
            GetMaxTickets() == Invalid.GetMaxTickets() &&
            GetLimbDamageMultiplier() == Invalid.GetLimbDamageMultiplier() &&
            GetLowHealthThreshold() == Invalid.GetLowHealthThreshold() &&
            GetMaxHealthLimit() == Invalid.GetMaxHealthLimit() &&
            GetOriginalMaxHealth() == Invalid.GetOriginalMaxHealth() &&
            CurrentLimbHealthHalvings == Invalid.CurrentLimbHealthHalvings &&
            bHasFullHealthEventBeenBroadcasted == Invalid.bHasFullHealthEventBeenBroadcasted &&
            bHasLowHealthEventBeenBroadcasted == Invalid.bHasLowHealthEventBeenBroadcasted &&
            bHasNoHealthEventBeenBroadcasted == Invalid.bHasNoHealthEventBeenBroadcasted &&
            bHasOnBrokenEventBeenBroadcasted == Invalid.bHasOnBrokenEventBeenBroadcasted &&
            bHasOnFullTicketsEventBeenBroadcasted == Invalid.bHasOnFullTicketsEventBeenBroadcasted &&
            bHasOnNoTicketsRemainingEventBeenBroadcasted == Invalid.bHasOnNoTicketsRemainingEventBeenBroadcasted;
}

bool FLimbHealthData::Equals(const FLimbHealthData& OtherLimb) const
{
    return 	GetCurrentHealth() == OtherLimb.GetCurrentHealth() &&
            GetMaxHealth() == OtherLimb.GetMaxHealth() &&
            GetLowHealth() == OtherLimb.GetLowHealth() &&
            GetPreviousHealth() == OtherLimb.GetPreviousHealth() &&
            GetRemainingTickets() == OtherLimb.GetRemainingTickets() &&
            GetMaxTickets() == OtherLimb.GetMaxTickets() &&
            GetLimbDamageMultiplier() == OtherLimb.GetLimbDamageMultiplier() &&
            GetLowHealthThreshold() == OtherLimb.GetLowHealthThreshold() &&
            GetMaxHealthLimit() == OtherLimb.GetMaxHealthLimit() &&
            GetOriginalMaxHealth() == OtherLimb.GetOriginalMaxHealth() &&
            CurrentLimbHealthHalvings == OtherLimb.CurrentLimbHealthHalvings &&
            bHasFullHealthEventBeenBroadcasted == OtherLimb.bHasFullHealthEventBeenBroadcasted &&
            bHasLowHealthEventBeenBroadcasted == OtherLimb.bHasLowHealthEventBeenBroadcasted &&
            bHasNoHealthEventBeenBroadcasted == OtherLimb.bHasNoHealthEventBeenBroadcasted &&
            bHasOnBrokenEventBeenBroadcasted == OtherLimb.bHasOnBrokenEventBeenBroadcasted &&
            bHasOnFullTicketsEventBeenBroadcasted == OtherLimb.bHasOnFullTicketsEventBeenBroadcasted &&
            bHasOnNoTicketsRemainingEventBeenBroadcasted == OtherLimb.bHasOnNoTicketsRemainingEventBeenBroadcasted;
}

void FLimbHealthData::SetHealthToMax()
{
    PreviousHealth = Health;

    Health = FMath::Clamp(MaxHealth, 0.0f, MaxHealthLimit);
    Tickets = MaxTickets;
}

void FLimbHealthData::ResetHealth()
{
    MaxHealth = OriginalMaxHealth;
	Health = MaxHealth;
	PreviousHealth = MaxHealth;
    Tickets = MaxTickets;

	LowHealth = MaxHealth * LowHealthThreshold;
    
    CurrentLimbHealthHalvings = 0;
}

void FLimbHealthData::UpdatePreviousHealth()
{
    PreviousHealth = Health;
}

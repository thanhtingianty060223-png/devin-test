// Copyright Void Interactive, 2021

#include "ArmourResourceComponent.h"

UArmourResourceComponent::UArmourResourceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;

	ResourceName = "Armour";
	MaxResourceLimit = 500.0f;
	RemainingTickets = MaxTickets;
}

void UArmourResourceComponent::SetMaxTickets(const int32 NewMax)
{
	MaxTickets = FMath::Clamp(NewMax, 0, MAX_int32);
	RemainingTickets = MaxTickets;
}

void UArmourResourceComponent::SetResistance(const float NewResistancePercentage)
{
	Resistance = FMath::Clamp(NewResistancePercentage, 0.0f, 100.0f);
}

void UArmourResourceComponent::BeginPlay()
{
	Super::BeginPlay();

	RemainingTickets = MaxTickets;
}

void UArmourResourceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UArmourResourceComponent, MaxTickets);
	DOREPLIFETIME(UArmourResourceComponent, RemainingTickets);
	DOREPLIFETIME(UArmourResourceComponent, Resistance);
}

void UArmourResourceComponent::Server_IncreaseResource_Implementation(const float Amount)
{
	Super::Server_IncreaseResource_Implementation(Amount);
	
	RemainingTickets = FMath::Clamp(RemainingTickets + 1, 0, MaxTickets);
}

void UArmourResourceComponent::Server_DecreaseResource_Implementation(const float Amount)
{
	Super::Server_DecreaseResource_Implementation(Amount);

	RemainingTickets = FMath::Clamp(RemainingTickets - 1, 0, MaxTickets);
}

void UArmourResourceComponent::Server_ResetResource_Implementation()
{
	Super::Server_ResetResource_Implementation();

	RemainingTickets = MaxTickets;
}

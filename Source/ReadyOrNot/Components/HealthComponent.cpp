// Copyright Void Interactive, 2021

#include "HealthComponent.h"

UHealthComponent::UHealthComponent() : Super()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;

	ResourceName = "Health";

	UResourceComponent::Server_InitResource_Implementation();
}
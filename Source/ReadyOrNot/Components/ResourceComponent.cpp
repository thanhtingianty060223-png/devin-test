// Copyright Void Interactive, 2021

#include "ResourceComponent.h"

UResourceComponent::UResourceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;

	UResourceComponent::Server_InitResource_Implementation();
}

void UResourceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UResourceComponent, Resource);
	DOREPLIFETIME(UResourceComponent, ResourceName);
	DOREPLIFETIME(UResourceComponent, PreviousResource);
	DOREPLIFETIME(UResourceComponent, OriginalMaxResource);
	DOREPLIFETIME(UResourceComponent, MaxResource);
	DOREPLIFETIME(UResourceComponent, LowResource);
	DOREPLIFETIME(UResourceComponent, LowResourceThreshold);
	DOREPLIFETIME(UResourceComponent, bUnlimited);
}

void UResourceComponent::SetUnlimitedResource(const bool bEnabled)
{
	bUnlimited = bEnabled;
}

void UResourceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_AutonomousProxy)
		Server_InitResource();
}

void UResourceComponent::OnComponentCreated()
{
	SetIsReplicated(true);
}

#if WITH_EDITOR
void UResourceComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// If default resource value has changed, update current resource
	if (PropertyChangedEvent.GetPropertyName() == "MaxResource")
	{
		if (MaxResource > MaxResourceLimit)
		{
			MaxResource = MaxResourceLimit;
			Resource = MaxResource;
		}

		Resource = MaxResource;
	}
	else if (PropertyChangedEvent.GetPropertyName() == "MaxResourceLimit")
	{
		if (MaxResource > MaxResourceLimit)
		{
			MaxResource = MaxResourceLimit;
			Resource = MaxResource;
		}
	}
}
#endif

void UResourceComponent::Server_InitResource_Implementation()
{
	OriginalMaxResource = MaxResource;
	Resource = MaxResource;
	PreviousResource = MaxResource;

	LowResource = MaxResource * LowResourceThreshold;
}

void UResourceComponent::Server_EnableUnlimitedResource_Implementation()
{
	bUnlimited = true;
}

void UResourceComponent::Server_DisableUnlimitedResource_Implementation()
{
	bUnlimited = false;
}

void UResourceComponent::Server_ToggleUnlimitedResource_Implementation()
{
	bUnlimited = !bUnlimited;
}

void UResourceComponent::Server_SetUnlimitedResource_Implementation(const bool bEnabled)
{
#if !UE_BUILD_SHIPPING
	SetUnlimitedResource(bEnabled);
#endif
}

void UResourceComponent::Server_ResetResource_Implementation()
{
	MaxResource = OriginalMaxResource;
	Resource = OriginalMaxResource;
	PreviousResource = OriginalMaxResource;

	LowResource = MaxResource * LowResourceThreshold;
}

void UResourceComponent::Server_DepleteResource_Implementation()
{
	if (bUnlimited)
		return;
	
	PreviousResource = Resource;
	
	Resource = 0.0f;

	OnDepletedResource.Broadcast();
	bNoResourceEventBroadcasted = true;
}

void UResourceComponent::Server_SetMaxResource_Implementation(const float NewMaxResource)
{
	if (NewMaxResource <= 0.0f)
		return;

	MaxResource = FMath::Clamp(NewMaxResource, 0.0f, MaxResourceLimit);
	OriginalMaxResource = MaxResource;

	LowResource = MaxResource * LowResourceThreshold;
}

void UResourceComponent::Server_SetResource_Implementation(const float NewResourceAmount)
{
	if (NewResourceAmount <= 0.0f || bUnlimited)
		return;
	
	PreviousResource = Resource;

	Resource = FMath::Clamp(NewResourceAmount, -1.0f, MaxResource);

	BroadcastEvents_SetResource();
}

void UResourceComponent::Server_SetCurrentResourceToMax_Implementation()
{
	Server_SetResource_Implementation(MaxResource);
}

void UResourceComponent::Server_UpdatePreviousResource_Implementation()
{
	PreviousResource = Resource;
}

void UResourceComponent::Server_IncreaseResource_Implementation(const float Amount)
{
	if (Amount <= 0.0f || Resource >= MaxResource)
		return;
	
	PreviousResource = Resource;

	Resource = FMath::Clamp(IncreaseResource_Expression(Amount), 0.0f, MaxResource);

	BroadcastEvents_IncreaseResource();
}

void UResourceComponent::Server_DecreaseResource_Implementation(const float Amount)
{
	if (Amount <= 0.0f || Resource <= 0.0f || bUnlimited)
		return;

	PreviousResource = Resource;

	Resource = FMath::Clamp(DecreaseResource_Expression(Amount), 0.0f, MaxResource);

	BroadcastEvents_DecreaseResource();
}

void UResourceComponent::BroadcastEvents_IncreaseResource()
{
	if (Resource >= MaxResource)
	{
		if (!bFullResourceEventBroadcasted && Resource != PreviousResource)
		{
			OnFullResource.Broadcast();
			bFullResourceEventBroadcasted = true;
		}
	}
	else
	{
		if (Resource > LowResource)
		{
			bFullResourceEventBroadcasted = false;
			bLowResourceEventBroadcasted = false;
			bNoResourceEventBroadcasted = false;
		}
	}
}

void UResourceComponent::BroadcastEvents_DecreaseResource()
{
	if (Resource <= 0.0f)
	{
		if (!bNoResourceEventBroadcasted && Resource != PreviousResource)
		{
			OnDepletedResource.Broadcast();
			bNoResourceEventBroadcasted = true;
		}
	}
	else if (Resource <= LowResource)
	{
		if (!bLowResourceEventBroadcasted && Resource != PreviousResource)
		{
			OnLowResource.Broadcast(Resource);
			bLowResourceEventBroadcasted = true;
		}
	}
	else
	{
		bFullResourceEventBroadcasted = false;
		bLowResourceEventBroadcasted = false;
		bNoResourceEventBroadcasted = false;
	}
}

void UResourceComponent::BroadcastEvents_SetResource()
{
	if (Resource >= MaxResource)
	{
		if (!bFullResourceEventBroadcasted && Resource != PreviousResource)
		{
			OnFullResource.Broadcast();
			bFullResourceEventBroadcasted = true;
		}
	}

	if (Resource <= LowResource)
	{
		if (!bLowResourceEventBroadcasted && Resource != PreviousResource)
		{
			OnLowResource.Broadcast(Resource);
			bLowResourceEventBroadcasted = true;
		}
	}

	if (Resource <= 0)
	{
		if (!bNoResourceEventBroadcasted && Resource != PreviousResource)
		{
			OnDepletedResource.Broadcast();
			bNoResourceEventBroadcasted = true;
		}
	}

	if (Resource > LowResource && Resource < MaxResource)
	{
		bFullResourceEventBroadcasted = false;
		bLowResourceEventBroadcasted = false;
		bNoResourceEventBroadcasted = false;
	}
}

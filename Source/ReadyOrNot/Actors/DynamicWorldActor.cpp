#include "DynamicWorldActor.h"
#include "ReadyOrNot.h"

ADynamicWorldActor::ADynamicWorldActor()
{
	bReplicates = true;
}

void ADynamicWorldActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADynamicWorldActor, bReplicateSpawn);
}

void ADynamicWorldActor::BeginPlay()
{
	Super::BeginPlay();

	if (!bReplicateSpawn)
	{
		SetActorHiddenInGame(true);
	}
}

void ADynamicWorldActor::SetDynamicSpawn(bool bSpawned)
{
	bReplicateSpawn = bSpawned;
	if (bSpawned)
	{
		V_LOGM(LogReadyOrNot, "Successfully Spawned Dynamic World Actor %s!", *GetName());
		SetActorHiddenInGame(false);
		OnDynamicallySpawned();
	}
	else
	{
		V_LOGM(LogReadyOrNot, "Destroying Dynamic World Actor %s!", *GetName());
		Destroy();
	}
}

void ADynamicWorldActor::OnRep_ReplicateSpawn()
{
	SetDynamicSpawn(bReplicateSpawn);
}

void ADynamicWorldActor::CheckDynamicSpawn()
{
	AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (!gs)
	{
		SetDynamicSpawn(false);
		return;
	}

	for (int32 i = 0; i < gs->WhitelistedLabels.Num(); i++)
	{
		if (gs->WhitelistedLabels[i] == DynamicLabel)
		{
			SetDynamicSpawn(true);
			return;
		}
	}

	SetDynamicSpawn(false);
}

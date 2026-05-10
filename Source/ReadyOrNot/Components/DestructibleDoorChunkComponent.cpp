// Copyright Void Interactive, 2021

#include "DestructibleDoorChunkComponent.h"

#include "Actors/Door.h"

UDestructibleDoorChunkComponent::UDestructibleDoorChunkComponent()
{
	SetGenerateOverlapEvents(false);
	
	bForceNavigationObstacle = false;
	bAutoRegister = true;

	ComponentTags.AddUnique("NoCover");

	UPrimitiveComponent::SetNotifyRigidBodyCollision(true);
	UPrimitiveComponent::SetCollisionObjectType(ECC_DOOR);
	UPrimitiveComponent::SetLinearDamping(0.1f);
	UPrimitiveComponent::SetAngularDamping(0.5f);
	UPrimitiveComponent::SetAllUseCCD(true);
}

void UDestructibleDoorChunkComponent::DoDamage(float Damage)
{
	Health -= Damage;
	if (Health <= 0.0f)
	{
		OnChunkDestroyed();
	}
}

void UDestructibleDoorChunkComponent::SetIsDoorHandle(bool bNewDoorHandle)
{
	bIsDoorHandle = bNewDoorHandle;
}

void UDestructibleDoorChunkComponent::SetIsHinge(bool bNewIsHinge)
{
	bIsHinge = bNewIsHinge;
}

void UDestructibleDoorChunkComponent::SetCannotKickIfDestroyed(bool bNewCannotKickIfDestroyed)
{
	bCannotKickIfDestroyed = bNewCannotKickIfDestroyed;
}

bool UDestructibleDoorChunkComponent::IsUnkickablePiece()
{
	if (IsDestroyed() && bCannotKickIfDestroyed)
	{
		return true;
	}
	return false;
}

void UDestructibleDoorChunkComponent::AddSupportChunk(class UDestructibleDoorChunkComponent* Chunk)
{
	if (Chunk)
	{
		SupportChunks.AddUnique(Chunk);
	}
}

void UDestructibleDoorChunkComponent::CheckSupportChunks()
{

	// BFS to check if we have a connection to a hinge.
	TArray<UDestructibleDoorChunkComponent*> Queue;
	TArray<UDestructibleDoorChunkComponent*> Visited = {this};

	// Add all chunks to the queue so the BFS can start.
	for(UDestructibleDoorChunkComponent* Chunk : SupportChunks)
	{
		Queue.Add(Chunk);
	}
	
	bool bAnySupportsLeft = false;

	while(Queue.Num() > 0 && !bAnySupportsLeft)
	{
		UDestructibleDoorChunkComponent* Chunk = Queue.Pop();
		Visited.Add(Chunk);
		if(!Chunk->IsDestroyed() && Chunk->IsHinge())
		{
			bAnySupportsLeft = true;
		}
		else if(!Chunk->IsDestroyed())
		{
			for(UDestructibleDoorChunkComponent* SupportChunk : Chunk->SupportChunks)
			{
				if(!Queue.Contains(SupportChunk) && !Visited.Contains(SupportChunk))
				{
					Queue.Add(SupportChunk);
				}
			}
		}
	}

	if (!bAnySupportsLeft && !bIsHinge)
	{
		OnChunkDestroyed();
	}
}

void UDestructibleDoorChunkComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsVisible())
	{
		SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Block);
	}
	else
	{
		SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Ignore);
	}

	UPrimitiveComponent::SetCollisionObjectType(ECC_DOOR);
	UPrimitiveComponent::SetMassOverrideInKg(NAME_None, 75.0f);
}

void UDestructibleDoorChunkComponent::OnVisibilityChanged()
{
	Super::OnVisibilityChanged();

	if (IsVisible())
	{
		SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Block);
	}
	else
	{
		SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Ignore);
	}
}

void UDestructibleDoorChunkComponent::PostLoad()
{
	Super::PostLoad();

	ComponentTags.AddUnique("NoCover");
}

void UDestructibleDoorChunkComponent::Restore()
{
	Health = 1500.0f;
	UPrimitiveComponent::SetCollisionObjectType(ECC_DOOR);
	UPrimitiveComponent::SetMassOverrideInKg(NAME_None, 75.0f);
	SetSimulatePhysics(false);
	SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Block);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	SetCanEverAffectNavigation(true);
}

void UDestructibleDoorChunkComponent::OnChunkDestroyed()
{
	Health = -1.0f;

	if (ADoor* Door = GetOwner<ADoor>())
	{
		if (GetOwnerRole() >= ROLE_Authority)
		{
			Door->DestroyedChunkIdxs.AddUnique(Door->GetChunkComponents().Find(this));
		}
		
		if (bIsDoorHandle)
		{
			Door->BreakDoorHandles();
		}

		UPrimitiveComponent::SetMassOverrideInKg(NAME_None, 75.0f);
		SetGenerateOverlapEvents(false);
		SetNotifyRigidBodyCollision(false);
		SetSimulatePhysics(true);
		SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		SetCanEverAffectNavigation(false);
	}
}

// Copyright Void Interactive, 2021

#include "SpawnStaticMeshAnimNotifyState.h"
#include "ReadyOrNot.h"

void USpawnStaticMeshAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	AStaticMeshActor* sm = MeshComp->GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform());
	if (sm)
	{
		#if WITH_EDITOR
		sm->SetActorLabel("NotifyPropActor " + Animation->GetName());
		#endif
		
		sm->SetOwner(MeshComp->GetOwner());
		sm->GetStaticMeshComponent()->SetOwnerNoSee(bOwnerNoSee);
		sm->GetStaticMeshComponent()->SetOnlyOwnerSee(bOnlyOwnerSee);
		sm->GetStaticMeshComponent()->SetCastShadow(bCastShadow);
		sm->SetMobility(EComponentMobility::Movable);
		sm->SetActorEnableCollision(false);
		sm->GetStaticMeshComponent()->SetStaticMesh(StaticMeshToSpawn);
		sm->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		sm->GetStaticMeshComponent()->SetCanEverAffectNavigation(false);
		sm->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetIncludingScale, BoneToSpawnOn);
		sm->SetReplicates(false);
		sm->Tags.AddUnique("SpawnedFromStaticMeshNotify");
		SpawnedStaticMeshGUID = FGuid::NewGuid();;
		sm->Tags.AddUnique(*SpawnedStaticMeshGUID.ToString());
		if (!bSimulatePhysicsAtEnd && bDestroyAtEnd)
		{
			sm->SetLifeSpan(TotalDuration);
		}
		
		for (TActorIterator<AReadyOrNotLevelScript>It(MeshComp->GetWorld()); It; ++It)
		{
			AReadyOrNotLevelScript* LevelScript = *It;
			if (LevelScript)
			{
				LevelScript->SpawnedFromNotifyActors.AddUnique(sm);
			}
		}
	}
}

void USpawnStaticMeshAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{

}

void USpawnStaticMeshAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	AStaticMeshActor* SpawnedStaticMeshActor = nullptr;
	for (TActorIterator<AReadyOrNotLevelScript>It(MeshComp->GetWorld()); It; ++It)
	{
		AReadyOrNotLevelScript* LevelScript = *It;
		if (LevelScript)
		{
			for (AActor* MeshActor : LevelScript->SpawnedFromNotifyActors)
			{
				if (MeshActor)
				{
					if (MeshActor->Tags.Contains(*SpawnedStaticMeshGUID.ToString()))
					{
						SpawnedStaticMeshActor = (AStaticMeshActor*)MeshActor;
						break;
					}
				}
			}
		}
	}
	if (SpawnedStaticMeshActor)
	{
		if (bSimulatePhysicsAtEnd)
		{
			SpawnedStaticMeshActor->SetActorEnableCollision(true);
			SpawnedStaticMeshActor->GetStaticMeshComponent()->SetSimulatePhysics(true);
			SpawnedStaticMeshActor->GetStaticMeshComponent()->AddImpulse(MeshComp->GetForwardVector() * ForceVector, NAME_None, true);
			SpawnedStaticMeshActor->GetStaticMeshComponent()->AddAngularImpulseInRadians(MeshComp->GetForwardVector() * ForceVector, NAME_None);
			SpawnedStaticMeshActor->GetStaticMeshComponent()->SetCollisionProfileName("Item");
			SpawnedStaticMeshActor->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		}
		if (bDestroyAtEnd)
		{
			for (TActorIterator<AReadyOrNotLevelScript>It(MeshComp->GetWorld()); It; ++It)
			{
				AReadyOrNotLevelScript* LevelScript = *It;
				if (LevelScript)
				{
					LevelScript->SpawnedFromNotifyActors.Remove(SpawnedStaticMeshActor);
				}
			}
			SpawnedStaticMeshActor->Destroy();
		}
	}
}

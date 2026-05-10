// Void Interactive, 2020

#include "SpawnSkeletalMeshAnimNotifyState.h"

void USpawnSkeletalMeshAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	ASkeletalMeshActor* sm = MeshComp->GetWorld()->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass(), FTransform());
	if (sm)
	{
		
#if WITH_EDITOR
		sm->SetActorLabel("NotifyPropActor " + Animation->GetName());
#endif
		sm->SetActorEnableCollision(false);
		sm->SetOwner(MeshComp->GetOwner());
		sm->GetSkeletalMeshComponent()->SetOwnerNoSee(bOwnerNoSee);
		sm->GetSkeletalMeshComponent()->SetOnlyOwnerSee(bOnlyOwnerSee);
		sm->GetSkeletalMeshComponent()->SetSkeletalMesh(SkeletalMeshToSpawn);
		sm->GetSkeletalMeshComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		sm->GetSkeletalMeshComponent()->PlayAnimation(PlayAnimation, false);
		for (int32 i = 0; i < sm->GetSkeletalMeshComponent()->GetMaterials().Num(); i++)
		{
			UMaterialInstanceDynamic* DynMat = sm->GetSkeletalMeshComponent()->CreateAndSetMaterialInstanceDynamic(i);
			if (DynMat)
			{
				DynMat->SetScalarParameterValue("DisableWeaponFov", !bEnableWeaponFOVShader);
			}
		}
		sm->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetIncludingScale, BoneToSpawnOn);
		sm->SetActorRelativeTransform(MeshTransform);
		sm->SetReplicates(false);
		sm->Tags.AddUnique("SpawnedFromSkeletalMeshNotify");
		SpawnedSkeletalMeshGUID = FGuid::NewGuid();
		sm->Tags.AddUnique(*SpawnedSkeletalMeshGUID.ToString());

		if (!bSimulatePhysicsAtEnd && bDestroyAtEnd)
		{
			if (MeshComp->GetWorld()->WorldType == EWorldType::Editor || MeshComp->GetWorld()->WorldType == EWorldType::EditorPreview)
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

void USpawnSkeletalMeshAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{

}

void USpawnSkeletalMeshAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	ASkeletalMeshActor* SpawnSkeletalMeshActor = SkeletalMeshActor;
	for (TActorIterator<AReadyOrNotLevelScript>It(MeshComp->GetWorld()); It; ++It)
	{
		AReadyOrNotLevelScript* LevelScript = *It;
		if (LevelScript)
		{
			for (AActor* MeshActor : LevelScript->SpawnedFromNotifyActors)
			{
				if (MeshActor)
				{
					if (MeshActor->Tags.Contains(*SpawnedSkeletalMeshGUID.ToString()))
					{
						SpawnSkeletalMeshActor = (ASkeletalMeshActor*)MeshActor;
						break;
					}
				}
			}
		}
	}
	
	if (SpawnSkeletalMeshActor)
	{
		if (bDestroyAtEnd)
		{
			for (TActorIterator<AReadyOrNotLevelScript>It(MeshComp->GetWorld()); It; ++It)
			{
				AReadyOrNotLevelScript* LevelScript = *It;
				if (LevelScript)
				{
					LevelScript->SpawnedFromNotifyActors.Remove(SpawnSkeletalMeshActor);
				}
			}

			SpawnSkeletalMeshActor->Destroy();
			SkeletalMeshActor = nullptr;
			
			return;
		}
		
		if (bSimulatePhysicsAtEnd)
		{
			SpawnSkeletalMeshActor->SetActorEnableCollision(true);
			SpawnSkeletalMeshActor->GetSkeletalMeshComponent()->SetSimulatePhysics(true);
			SpawnSkeletalMeshActor->GetSkeletalMeshComponent()->AddImpulse(MeshComp->GetForwardVector() * ForceVector, NAME_None, true);
			SpawnSkeletalMeshActor->GetSkeletalMeshComponent()->AddAngularImpulseInRadians(MeshComp->GetForwardVector() * ForceVector, NAME_None);
			SpawnSkeletalMeshActor->GetSkeletalMeshComponent()->SetCollisionProfileName("Item");
			SpawnSkeletalMeshActor->GetSkeletalMeshComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		}
	}
}

// Copyright Void Interactive, 2022

#include "Animation/Notifies/AnimNotify_ApplyArteryDamage.h"

void UAnimNotify_ApplyArteryDamage::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	
	AReadyOrNotCharacter* MyCharacter = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner());
	if (MyCharacter)
	{
		if (MyCharacter->HasAuthority())
		{
			FHitResult FakeHit;
			FakeHit.TraceStart = MeshComp->GetBoneLocation(ArteryBoneName) + MyCharacter->GetActorForwardVector() * 100.0f;
			FakeHit.TraceEnd = MeshComp->GetBoneLocation(ArteryBoneName) - MyCharacter->GetActorForwardVector() * 100.0f;
			FakeHit.Distance = 200.0f;
			FakeHit.ImpactPoint = MeshComp->GetBoneLocation(ArteryBoneName);
			FakeHit.ImpactNormal = (FakeHit.TraceEnd - FakeHit.TraceStart).GetSafeNormal();
			FakeHit.BoneName = ArteryBoneName;
			FakeHit.Component = MeshComp;
			// ##UE5UPGRADE## Compatibility
			FakeHit.HitObjectHandle = MyCharacter;
			
			MyCharacter->LastHitBoneName = ArteryBoneName;
			
			FTimerDelegate Delegate;
			Delegate.BindUObject(MyCharacter, &AReadyOrNotCharacter::ApplyArteryDamage, FakeHit, (AActor*)MyCharacter);
			
			MyCharacter->GetWorld()->GetTimerManager().SetTimerForNextTick(Delegate);
		}
	}
}

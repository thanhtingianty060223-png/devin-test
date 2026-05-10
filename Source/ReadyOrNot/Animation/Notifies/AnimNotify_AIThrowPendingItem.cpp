// Void Interactive, 2020

#include "Animation/Notifies/AnimNotify_AIThrowPendingItem.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Info/Activities/Team/DoorBreachActivity.h"

void UAnimNotify_AIThrowPendingItem::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(MeshComp->GetOwner()))
	{
		if (!AICharacter->GetCyberneticsController())
		{
			AICharacter->PendingThrownItem = nullptr;
			return;
		}
		
		if (AICharacter->PendingThrownItem)
		{
			FTransform ThrowTransform = MeshComp->GetSocketTransform(BoneToSpawnOn);
			ThrowTransform.SetLocation(ThrowTransform.TransformPositionNoScale(RelativeTransform.GetLocation()));
			ThrowTransform.SetRotation(ThrowTransform.TransformRotation(RelativeTransform.GetRotation()));
			ThrowTransform.SetScale3D(RelativeTransform.GetScale3D());
			//DrawDebugSphere(MeshComp->GetWorld(), ThrowTransform.GetLocation(), 20.0f, 4, FColor::Yellow, false, 5.0f, 0, 2.0f);

			FVector FinalThrowLocation = ThrowTransform.GetLocation();
			FVector FinalThrowDirection = AICharacter->GetControlRotation().Vector();

			// TODO: Don't put this here, maybe delegate that UThrowGrenadeThroughDoorActivity responds to and returns the result of the code below
			if (UThrowGrenadeThroughDoorActivity* TGTD = AICharacter->GetCyberneticsController()->GetCurrentActivity<UThrowGrenadeThroughDoorActivity>())
			{
				TGTD->CalculateThrowTransform(ThrowTransform, FinalThrowLocation, FinalThrowDirection);
			}

			AICharacter->PendingThrownItem->SpawnThrownItemAtTransform(ThrowTransform, bCustomThrowDirection ? ThrowDirection.Vector() : FinalThrowDirection, FinalThrowLocation);
			AICharacter->OnPendingItemThrown_FromAnimNotify.Broadcast(AICharacter->PendingThrownItem);
			AICharacter->PendingThrownItem = nullptr;
		}
	}
}

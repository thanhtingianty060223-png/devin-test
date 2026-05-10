// Void Interactive, 2020

#include "AnimNotify_PushQueuedDoor.h"

#include "Characters/CyberneticCharacter.h"

#include "Actors/Door.h"

void UAnimNotify_PushQueuedDoor::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	
	ACyberneticCharacter* cc = Cast<ACyberneticCharacter>(MeshComp->GetOwner());
	if (cc)
	{
		if (cc->QueuedDoorToOpen)
		{
			if (!cc->QueuedDoorToOpen->IsOpening())
			{
				cc->QueuedDoorToOpen->OpenDoor(cc, false, true, true);
				cc->QueuedDoorToOpen->OpenSubDoor(cc);
			}
			
			cc->QueuedDoorToOpen = nullptr;
		}
		else if (cc->QueuedDoorToClose)
		{
			if (!cc->QueuedDoorToClose->IsClosing())
			{
				cc->QueuedDoorToClose->CloseDoor(cc);
				cc->QueuedDoorToClose->CloseSubDoor(cc);
			}

			cc->QueuedDoorToClose = nullptr;
		}
	}
}

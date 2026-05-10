// Void Interactive, 2020


#include "AnimNotify_DoorKickBreak.h"

void UAnimNotify_DoorKickBreak::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);
	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner());
	if (Character)
	{
		Character->Server_KickBreakQueuedDoor();
	}
}

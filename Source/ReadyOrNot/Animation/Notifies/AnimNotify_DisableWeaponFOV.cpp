// Void Interactive, 2020

#include "AnimNotify_DisableWeaponFOV.h"

void UAnimNotify_DisableWeaponFOV::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(MeshComp->GetOwner());
	
	if (PlayerCharacter && PlayerCharacter->IsLocalPlayer())
	{
		PlayerCharacter->SetDesiredWeaponFOVBlend(0.0f);
		PlayerCharacter->bDisableWeaponFOV_FromNotify = true;
		for (int32 i = 0; i < PlayerCharacter->GetInventoryComponent()->GetInventoryItems().Num(); i++)
		{
			ABaseItem* BaseItem = PlayerCharacter->GetInventoryComponent()->GetInventoryItems()[i];
			if (BaseItem)
			{
				BaseItem->bDisableWeaponFOV_FromNotify = true;
				BaseItem->bNoAttachmentRep = true;
				BaseItem->bEnabledWeaponFovShader = true;
				BaseItem->DisableWeaponFovShader();
			}
		}
	}
}

void UAnimNotify_DisableWeaponFOV::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(MeshComp->GetOwner());
	
	if (PlayerCharacter && PlayerCharacter->IsLocalPlayer())
	{
		PlayerCharacter->SetDesiredWeaponFOVBlend(0.0f);
		PlayerCharacter->bDisableWeaponFOV_FromNotify = true;
		
		for (int32 i = 0; i < PlayerCharacter->GetInventoryComponent()->GetInventoryItems().Num(); i++)
		{
			ABaseItem* BaseItem = PlayerCharacter->GetInventoryComponent()->GetInventoryItems()[i];
			if (BaseItem)
			{
				BaseItem->bDisableWeaponFOV_FromNotify = true;
				BaseItem->bNoAttachmentRep = true;
				BaseItem->bEnabledWeaponFovShader = true;
				BaseItem->DisableWeaponFovShader();
			}
		}
	}
}

void UAnimNotify_DisableWeaponFOV::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(MeshComp->GetOwner());
	if (PlayerCharacter && PlayerCharacter->IsLocalPlayer())
	{
		PlayerCharacter->SetDesiredWeaponFOVBlend(1.0f);
		PlayerCharacter->bDisableWeaponFOV_FromNotify = false;
		for (int32 i = 0; i < PlayerCharacter->GetInventoryComponent()->GetInventoryItems().Num(); i++)
		{
			ABaseItem* BaseItem = PlayerCharacter->GetInventoryComponent()->GetInventoryItems()[i];
			if (BaseItem)
			{
				BaseItem->bDisableWeaponFOV_FromNotify = false;
				BaseItem->bNoAttachmentRep = false;
				BaseItem->bEnabledWeaponFovShader = false;
				BaseItem->EnableWeaponFovShader();
			}
		}
	}
}

// Copyright Void Interactive, 2021

#include "MagMaskingAnimNotify.h"
#include "ReadyOrNot.h"

void UMagMaskingAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	ABaseMagazineWeapon* MagWeapon = Cast<ABaseMagazineWeapon>(MeshComp->GetOwner());
	if (MagWeapon)
	{
		if (MagWeapon->Mag_01_Comp && MagWeapon->Mag_01_Bullets_Comp
			&& MagWeapon->Mag_01_Extra_Comp && MagWeapon->Mag_02_Comp
			&& MagWeapon->Mag_02_Bullets_Comp && MagWeapon->Mag_02_Extra_Comp
			&& MagWeapon->Mag_ReloadInterpFix_Comp)
		{

			if (!MagWeapon->IsLocallyControlled())
			{
				if (MaskMag == EMaskMag::Mag01)
				{
					if (MagState == EMaskMagState::Show)
					{
						MagWeapon->Mag_01_Comp->SetHiddenInGame(false);
						MagWeapon->Mag_01_Extra_Comp->SetHiddenInGame(false);
						MagWeapon->Mag_01_Bullets_Comp->SetHiddenInGame(false);
					}
					else
					{
						MagWeapon->Mag_01_Comp->SetHiddenInGame(true);
						MagWeapon->Mag_01_Extra_Comp->SetHiddenInGame(true);
						MagWeapon->Mag_01_Bullets_Comp->SetHiddenInGame(true);
					}
				}

				if (MaskMag == EMaskMag::Mag02)
				{
					if (MagState == EMaskMagState::Show)
					{
						MagWeapon->Mag_02_Comp->SetHiddenInGame(false);
						MagWeapon->Mag_02_Extra_Comp->SetHiddenInGame(false);
						MagWeapon->Mag_02_Bullets_Comp->SetHiddenInGame(false);
					}
					else
					{
						MagWeapon->Mag_02_Comp->SetHiddenInGame(true);
						MagWeapon->Mag_02_Extra_Comp->SetHiddenInGame(true);
						MagWeapon->Mag_02_Bullets_Comp->SetHiddenInGame(true);
					}
				}

				if (MaskMag == EMaskMag::Dummy)
				{
					if (MagState == EMaskMagState::Show)
					{
						FTransform CopyTrans = MagWeapon->Mag_01_Comp->GetComponentTransform();

						if (bDummyCopyMag02)
							CopyTrans = MagWeapon->Mag_02_Comp->GetComponentTransform();

						MagWeapon->Mag_ReloadInterpFix_Comp->SetWorldTransform(CopyTrans);

						MagWeapon->Mag_ReloadInterpFix_Comp->AttachToComponent(MagWeapon->ItemMesh, FAttachmentTransformRules::KeepWorldTransform);

						MagWeapon->Mag_ReloadInterpFix_Comp->SetHiddenInGame(false);
					}
					else
					{
						MagWeapon->Mag_ReloadInterpFix_Comp->SetHiddenInGame(true);
					}
				}
			}
		}
	}	
}

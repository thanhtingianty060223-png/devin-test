// Copyright Void Interactive, 2023

#include "HUD/Widgets/Loadout/V2/AttachmentSlot_V2.h"
#include "Components/TextBlock.h"

void UAttachmentSlot_V2::NativeConstruct()
{
	Super::NativeConstruct();
	if (EmptyImage && ItemImage && !Attachment)
	{
		ItemImage = EmptyImage;
		ItemName = Attachment->ItemName;
	}
}

void UAttachmentSlot_V2::SetEquipped(bool IsEquipped)
{
	Equipped = IsEquipped;
	OnEquipped();
}

bool UAttachmentSlot_V2::GetEquipped()
{
	return Equipped;
}

void UAttachmentSlot_V2::SetAttachment(UWeaponAttachment* WeaponAttachment)
{
	Attachment = WeaponAttachment;
	if (Attachment)
	{
		ItemName = Attachment->ItemName;
		ItemImage = WeaponAttachment->UIElements.AttachmentIcon.LoadSynchronous();
		AttachmentType = Attachment->WeaponAttachmentType;
	}

	RefreshAttachmentInfo();
}

TSubclassOf<UWeaponAttachment> UAttachmentSlot_V2::GetAttachment()
{
	return Attachment ? Attachment->GetClass() : nullptr;
}

void UAttachmentSlot_V2::SetAttachmentType(EWeaponAttachmentType Type)
{
	AttachmentType = Type;
}

EWeaponAttachmentType UAttachmentSlot_V2::GetAttachmentType()
{
	return AttachmentType;
}

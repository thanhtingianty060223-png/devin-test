// Copyright Void Interactive, 2023

#include "ItemVisualizationComponent.h"

#include "InventoryComponent.h"

#include "Actors/Items/BallisticsShield.h"
#include "Actors/Items/Optiwand.h"
#include "Actors/Items/Shotgun.h"

#include "Characters/PlayerCharacter.h"

UItemVisualizationComponent::UItemVisualizationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 1.0f;
    
    bNoSkeletonUpdate = true;
    VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
    
    SetIsReplicatedByDefault(false);
    SetRenderStatic(true);
    
    UPrimitiveComponent::SetCollisionEnabled(ECollisionEnabled::NoCollision);

    MagazineComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Magazine Component"));
    MagazineComp->SetupAttachment(this, NAME_None);
    MagazineComp->SetIsReplicated(false);
    MagazineComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MagazineComp->SetVisibility(false);
    MagazineComp->SetHiddenInGame(true);
    MagazineComp->SetCastShadow(false);
    MagazineComp->SetCastHiddenShadow(false);
    MagazineComp->SetCastInsetShadow(false);
    MagazineComp->SetOwnerNoSee(true);
}

void CreateAttachment(USkeletalMeshComponent*& SkeletalMeshComponent, AReadyOrNotCharacter* Owner)
{
    SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(Owner);
    SkeletalMeshComponent->SetIsReplicated(false);
    SkeletalMeshComponent->RegisterComponent();
    SkeletalMeshComponent->SetSkeletalMesh(nullptr);
    SkeletalMeshComponent->SetRenderStatic(true);
    SkeletalMeshComponent->SetCastShadow(false);
    SkeletalMeshComponent->SetCastHiddenShadow(false);
    SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SkeletalMeshComponent->SetVisibility(false);
    SkeletalMeshComponent->SetHiddenInGame(true);
    SkeletalMeshComponent->SetOwnerNoSee(true);
}

void UItemVisualizationComponent::CreateAttachmentFromClass(USkeletalMeshComponent*& SkeletalMeshComponent, UWeaponAttachment* WeaponAttachment)
{
	if (!WeaponAttachment)
	{
	    if (IsValid(SkeletalMeshComponent))
	        SkeletalMeshComponent->SetSkeletalMesh(nullptr);
	    
	    return;
	}
    
    if (!SkeletalMeshComponent)
    {
        CreateAttachment(SkeletalMeshComponent, GetOwningCharacter());
    }
    
    SkeletalMeshComponent->SetSkeletalMesh(WeaponAttachment->SkeletalMesh);
    SkeletalMeshComponent->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponAttachment->GetAttachSocketName());
    SkeletalMeshComponent->SetVisibility(true);
    SkeletalMeshComponent->SetHiddenInGame(false);
    SkeletalMeshComponent->SetOwnerNoSee(true);
    SkeletalMeshComponent->SetCastShadow(true);
    SkeletalMeshComponent->SetCastHiddenShadow(true);
}

void HideComponent(UPrimitiveComponent* InComponent)
{
    if (InComponent)
    {
        InComponent->SetVisibility(false);
        InComponent->SetHiddenInGame(true);
    }
}

void UItemVisualizationComponent::DisableItemVisualizationComponent()
{
    MagazineComp->SetStaticMesh(nullptr);
    MagazineComp->SetCastShadow(false);
    MagazineComp->SetCastHiddenShadow(false);
    HideComponent(ScopeAttachment);
    HideComponent(MuzzleAttachment);
    HideComponent(UnderbarrelAttachment);
    HideComponent(OverbarrelAttachment);
    HideComponent(MagazineComp);

    SetSkeletalMesh(nullptr);
    SetComponentTickEnabled(false);
}

void UItemVisualizationComponent::UpdateItemVisualizationComponent()
{
    DisableItemVisualizationComponent();

    if (!GetOwningCharacter() || !GetOwningCharacter()->IsLocalPlayer())
        return;  
    
    BasedOfItem = nullptr;

    UInventoryComponent* InventoryComponent = GetOwningCharacter()->GetInventoryComponent();
    switch (VisualizationType)
    {
    case EItemVisualizationType::IVT_None: break;
    case EItemVisualizationType::IVT_Primary:       BasedOfItem = InventoryComponent->GetInventoryItemOfType(EItemCategory::IC_Primary); break;
    case EItemVisualizationType::IVT_Secondary:     BasedOfItem = InventoryComponent->GetInventoryItemOfType(EItemCategory::IC_Secondary); break;
    case EItemVisualizationType::IVT_LongTactical:  BasedOfItem = InventoryComponent->GetInventoryItemOfType(EItemCategory::IC_LongTactical); break;
    case EItemVisualizationType::IVT_Equipped:      BasedOfItem = InventoryComponent->GetEquippedItem(); break;
    case EItemVisualizationType::IVT_Helmet:        BasedOfItem = InventoryComponent->GetInventoryItemOfType(EItemCategory::IC_Helmet); break;
    case EItemVisualizationType::IVT_Armor:         BasedOfItem = InventoryComponent->GetInventoryItemOfType(EItemCategory::IC_Armor); break;
    default: break;
    }

    if (!BasedOfItem)
        return;

    SetSkeletalMesh(BasedOfItem->GetAppropriateSkeletalMesh());
    
    if (!SkeletalMesh)
    {
        DisableItemVisualizationComponent();
        return;  
    }

    // USED FOR SHADOW ONLY
    bool bEquipped = GetOwningCharacter()->GetEquippedItem() == BasedOfItem;
    if (!bEquipped)
    {
        if (const ABallisticsShield* BallisticsShield = Cast<ABallisticsShield>(GetOwningCharacter()->GetEquippedItem()))
        {
            bEquipped = BallisticsShield->PistolEquippedWithShield == BasedOfItem;
        }
    }

    const FName CustomSocket = GetOwningCharacter()->IsCarried() || GetOwningCharacter()->IsCarrying() ? BasedOfItem->BodySocket : NAME_None;
    const FName SocketName = CustomSocket == NAME_None ? (bEquipped ? BasedOfItem->HandsSocket : BasedOfItem->BodySocket) : CustomSocket;
        
    AttachToComponent(GetOwningCharacter()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
    
    EmptyOverrideMaterials();
    SetOwnerNoSee(true);
    SetCastShadow(true);
    SetCastHiddenShadow(true);
    SetRenderStatic(false);
    bNoSkeletonUpdate = false;
    SetMasterPoseComponent(BasedOfItem->ItemMesh);
    bUseAttachParentBound = true;

    if (VisualizationType == EItemVisualizationType::IVT_Armor)
    {
        // Needed to update armor skinning I guess
        SetComponentTickEnabled(true);
    }
    
    MagazineComp->SetStaticMesh(nullptr);
    MagazineComp->SetCastShadow(false);
    MagazineComp->SetCastHiddenShadow(false);
    HideComponent(MagazineComp);

    if (ABaseMagazineWeapon* bmw = Cast<ABaseMagazineWeapon>(BasedOfItem))
    {
        CreateAttachmentFromClass(ScopeAttachment, bmw->ScopeAttachment);
        CreateAttachmentFromClass(MuzzleAttachment, bmw->MuzzleAttachment);
        CreateAttachmentFromClass(UnderbarrelAttachment, bmw->UnderbarrelAttachment);
        CreateAttachmentFromClass(OverbarrelAttachment, bmw->OverbarrelAttachment);

        if (bmw->bHasVisibleMags) // Only setup magazine for items that have visible mags
        {
            UStaticMesh* AppropriateMesh = bmw->GetAppropriateMagazineMesh();
            
            MagazineComp->SetStaticMesh(AppropriateMesh);
            
            FName Mag01Socket = NAME_None, Mag02Socket = NAME_None;
            bmw->GetMagazineAttachmentSockets(Mag01Socket, Mag02Socket);
            MagazineComp->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale, Mag01Socket);

            MagazineComp->SetCastShadow(true);
            MagazineComp->SetCastHiddenShadow(true);
            MagazineComp->SetVisibility(true);
            MagazineComp->SetHiddenInGame(false);
        }
    }
}

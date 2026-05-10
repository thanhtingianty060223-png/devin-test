// Copyright Void Interactive, 2023

#include "WeaponAttachment.h"
#include "lib/ReadyOrNotFunctionLibrary.h"

UWeaponAttachment::UWeaponAttachment()
{
	SetIsReplicatedByDefault(true);
	
	UPrimitiveComponent::SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UPrimitiveComponent::SetCollisionResponseToChannels(ECR_Ignore);

	if (ToggleSound == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> ToggleEvent(TEXT("FMODEvent'/Game/FMOD/Events/Tacti_Gear/Tactical_Att.Tactical_Att'"));
	
		if(ToggleEvent.Object != nullptr)
			ToggleSound = ToggleEvent.Object;
	}

	bReceivesDecals = false;
}

void UWeaponAttachment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// We don't want to replicate anything inside of the actor (no subobjects) just the actor itself
	// If subclasses want to replicate they will override
	//Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Copied from ActorComponent
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	// ##UE5.3UPGRADE##
	// DOREPLIFETIME_WITH_PARAMS_FAST(UActorComponent, bReplicates, SharedParams);
	// ##UE5.3UPGRADE##
	
	// These are necessary to get rid of annoying warnings that appear
	DISABLE_REPLICATED_PROPERTY_FAST(UActorComponent, bIsActive);
	DISABLE_REPLICATED_PROPERTY_FAST(USceneComponent, bAbsoluteLocation);
	DISABLE_REPLICATED_PROPERTY_FAST(USceneComponent, bAbsoluteRotation);
	DISABLE_REPLICATED_PROPERTY_FAST(USceneComponent, bAbsoluteScale);
	DISABLE_REPLICATED_PROPERTY_FAST(USceneComponent, bVisible);
	DISABLE_REPLICATED_PROPERTY_FAST(USceneComponent, bShouldBeAttached);
	DISABLE_REPLICATED_PROPERTY_FAST(USceneComponent, bShouldSnapLocationWhenAttached);
	DISABLE_REPLICATED_PROPERTY_FAST(USceneComponent, bShouldSnapRotationWhenAttached);
	DISABLE_REPLICATED_PROPERTY_FAST(USceneComponent, AttachParent);
	DISABLE_REPLICATED_PROPERTY_FAST(USceneComponent, AttachChildren);
	DISABLE_REPLICATED_PROPERTY_FAST(USceneComponent, AttachSocketName);
	DISABLE_REPLICATED_PROPERTY_FAST(USceneComponent, RelativeLocation);
	DISABLE_REPLICATED_PROPERTY_FAST(USceneComponent, RelativeRotation);
	DISABLE_REPLICATED_PROPERTY_FAST(USceneComponent, RelativeScale3D);
}

void UWeaponAttachment::BeginPlay()
{
	Super::BeginPlay();
	
	// Create dynamic materials for each of the mesh's materials
	for (int32 i = 0; i < this->GetNumMaterials(); i++)
	{
		SkinMaterials.Add(this->CreateAndSetMaterialInstanceDynamic(i));
	}

}

void UWeaponAttachment::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	USkeletalMeshComponent* skeleAP = Cast<USkeletalMeshComponent>(GetAttachParent());
	if (skeleAP)
	{
		SetCastShadow(skeleAP->CastShadow);
		SetVisibility(skeleAP->IsVisible());
		bVisibleInRayTracing =  skeleAP->IsVisible();


		if (!HideBoneOnMesh.IsNone())
		{
			skeleAP->UnHideBoneByName(HideBoneOnMesh);
		}
	}
}

void UWeaponAttachment::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SetSimulatePhysics(false);

	ABaseMagazineWeapon* BaseMagazineWeapon = Cast<ABaseMagazineWeapon>(GetWeapon());
	if (BaseMagazineWeapon && BaseMagazineWeapon->GetGameTimeSinceCreation() > 1.0f)
	{
		if (!bAddedMagazineAmmo && MagazineAmmoIncrease > 0)
		{
			for (int32 i = 0; i < BaseMagazineWeapon->Magazines.Num(); i++)
			{
				BaseMagazineWeapon->Magazines[i].Ammo += MagazineAmmoIncrease;
			}
			bAddedMagazineAmmo = true;
		}
	}
	
	USkeletalMeshComponent* skeleAP = Cast<USkeletalMeshComponent>(GetAttachParent());
	if (!skeleAP)
	{
		if (GetWeapon())
		{
			skeleAP = GetWeapon()->GetItemMesh();
		}
	}
	if (skeleAP)
	{
		if (!skeleAP->SkeletalMesh)
		{
			SetHiddenInGame(true);
			return;
		}
		SetOwnerNoSee(skeleAP->bOwnerNoSee);
		SetOnlyOwnerSee(skeleAP->bOnlyOwnerSee);
		SetCastShadow(skeleAP->CastShadow);
		bSelfShadowOnly = skeleAP->bSelfShadowOnly;
		bCastHiddenShadow = skeleAP->bCastHiddenShadow;
		SetHiddenInGame(skeleAP->SkeletalMesh == nullptr || skeleAP->bHiddenInGame);
		SetVisibility(skeleAP->IsVisible());
		
		if (!HideBoneOnMesh.IsNone())
		{
			skeleAP->HideBoneByName(HideBoneOnMesh, PBO_None);
		}
		
		
		 if (GetWeapon() && GetWeapon()->IsLocallyControlled())
		 {
		 	bUseAttachParentBound = true;
		 	bVisibleInRayTracing = false;
		 	SkinMaterials.Remove(nullptr);
		 	for (int32 i = 0; i < SkinMaterials.Num(); i++)
		 	{
		 		SetMaterial(i, SkinMaterials[i]);
		 		SkinMaterials[i]->SetScalarParameterValue("DisableWeaponFOV", GetWeapon()->bEnabledWeaponFovShader == 0);
		 		// wetlevel is inverted
		 		AReadyOrNotLevelScript* Ls = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor());
		 		bool bOutSideInRain = GetWeapon()->GetOwnerCharacter()->IsOutside() && Ls && Ls->bRaining;
		 		float CurrentWetLevel = 0.0f;
		 		SkinMaterials[i]->GetScalarParameterValue(FHashedMaterialParameterInfo("WetLevel"), CurrentWetLevel);
		 		SkinMaterials[i]->SetScalarParameterValue("WetLevel", UKismetMathLibrary::FInterpTo_Constant(CurrentWetLevel,   bOutSideInRain ? 0.0f : 1.0f, DeltaTime, 1.0f));
		 	}
		 } else
		 {
			 EmptyOverrideMaterials();
		 }
	}
}

void UWeaponAttachment::ApplyWetness(float Wetness, float RainAmount)
{
	static const FName WetnessParam = FName("IsGunWet?");
	static const FName RainParam = FName("IsRainingOnGun?");

	for (int32 i = 0; i < SkinMaterials.Num(); i++)
	{
		if (SkinMaterials[i])
		{
			SkinMaterials[i]->SetScalarParameterValue(WetnessParam, Wetness);
			SkinMaterials[i]->SetScalarParameterValue(RainParam, RainAmount);
		}
	}
}

void UWeaponAttachment::PlayToggleSound()
{
	if (!ToggleSound || !GetWeapon() || !GetWeapon()->FMODAudioComp)
		return;

	if (!GetWeapon()->GetOwningPlayerController() || !GetWeapon()->GetOwningPlayerController()->IsLocalController())
		return;
	
	GetWeapon()->FMODAudioComp->Event = ToggleSound;
	GetWeapon()->FMODAudioComp->Play();
}

void UWeaponAttachment::DestroyComponent(bool bPromoteChildren)
{
	Super::DestroyComponent(bPromoteChildren);
	
	USkeletalMeshComponent* skeleAP = Cast<USkeletalMeshComponent>(GetAttachParent());
	if (skeleAP)
	{
		skeleAP->UnHideBoneByName(HideBoneOnMesh);
	}
}

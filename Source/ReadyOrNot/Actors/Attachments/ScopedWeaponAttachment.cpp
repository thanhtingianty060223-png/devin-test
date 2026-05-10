// Copyright Void Interactive, 2017

#include "ScopedWeaponAttachment.h"
#include "ReadyOrNot.h"
#include "Components/InventoryComponent.h"
#include "Components/PlayerPostProcessing.h"

UScopedWeaponAttachment::UScopedWeaponAttachment()
{
	
}

void UScopedWeaponAttachment::BeginPlay()
{
	Super::BeginPlay();
}

void UScopedWeaponAttachment::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bUsePipRendering && GetWeapon())
	{
		if (IsAttachedTo(GetWeapon()->ItemMesh) && GetWeapon()->IsLocallyControlled())
		{
			APlayerCharacter* pc = Cast<APlayerCharacter>(GetWeapon()->GetOwner());
			if (pc)
			{
				if (pc->bAiming)
				{
					PipFadeIn = FMath::FInterpTo(PipFadeIn, 1.0f, DeltaTime, 5.0f);
				}
				else
				{
					PipFadeIn = FMath::FInterpTo(PipFadeIn, 0.0f, DeltaTime, 20.0f);
				}

				if (!PipRenderDynamicMaterial)
				{
					PipRenderDynamicMaterial = Cast<UMaterialInstanceDynamic>(GetMaterial(PipMaterialIdx));
					if (!PipRenderDynamicMaterial)
					{
						PipRenderDynamicMaterial = UMaterialInstanceDynamic::Create(GetMaterial(PipMaterialIdx), this);
						SetMaterial(PipMaterialIdx, PipRenderDynamicMaterial);
					}
				}
				else
				{
					PipRenderDynamicMaterial->SetScalarParameterValue("UsesPIP", PipFadeIn);
				}

				
				if (pc->GetEquippedItem() == GetOwner())
				{
					if (!PipSceneCapture)
					{
						PipSceneCapture = NewObject<USceneCaptureComponent2D>(GetWeapon(), USceneCaptureComponent2D::StaticClass());
						if (PipSceneCapture)
						{
							PipSceneCapture->RegisterComponent();
							PipSceneCapture->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale, "PipRender");
							PipSceneCapture->FOVAngle = PipFOV;
							PipSceneCapture->HiddenComponents.Add(pc->GetMesh());
							PipSceneCapture->HiddenComponents.Add(pc->GetFaceMesh());
							PipSceneCapture->HiddenActors.Add(pc);
							for (ABaseItem* bi : pc->GetInventoryComponent()->GetInventoryItems())
							{
								PipSceneCapture->HiddenActors.Add(bi);
							}
							PipSceneCapture->CaptureSource = CaptureSource;

							if (bOverridePostProcessSettings)
							{
								PipSceneCapture->PostProcessSettings = OverridePostProcessSettings;
							}
							else if (UPlayerPostProcessing* PlayerPostProcessing = pc->GetPlayerPostProcessing())
							{
								PipSceneCapture->PostProcessSettings = PlayerPostProcessing->GetDefaultPlayerPostProcessSettings();
							}

						}

					}
					else if (PipSceneCapture)
					{
						PipSceneCapture->bNeverNeedsRenderUpdate = PipFadeIn == 0.0f;
						PipSceneCapture->bCaptureEveryFrame = PipFadeIn != 0.0f;
						PipSceneCapture->bCaptureOnMovement = PipFadeIn != 0.0f;	

						if (bNeedInventoryUpdate)
						{
							PipSceneCapture->HiddenActors.Empty();

							TArray<ABaseItem*> Items = pc->GetInventoryComponent()->GetInventoryItems();
							for (int32 i = 0; i < Items.Num(); i++)
							{
								if (Items[i] && Items[i]->ShouldHideInPictureInPictureScopes())
								{
									PipSceneCapture->HiddenActors.AddUnique(Items[i]);
								}
							}
							PipSceneCapture->HiddenActors.AddUnique(pc);

							bNeedInventoryUpdate = false;
						}
					}

					if (!PipRenderTarget)
					{
						PipRenderTarget = NewObject<UTextureRenderTarget2D>(GetWeapon(), UTextureRenderTarget2D::StaticClass());
						if (PipRenderTarget)
						{
							PipRenderTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
							PipRenderTarget->bAutoGenerateMips = false;
							PipRenderTarget->InitAutoFormat(PipResolution.X, PipResolution.Y);
							
							PipRenderDynamicMaterial->SetTextureParameterValue("PipRender", PipRenderTarget);

						}
					}
					else
					{
						if (PipSceneCapture)
						{
							PipSceneCapture->TextureTarget = PipRenderTarget;
						}
					}
				}
				else
				{
					if (PipSceneCapture)
					{
						PipSceneCapture->DestroyComponent();
						PipSceneCapture = nullptr;
					}
				}

			}
		} else
		{
			// must have been dropped clean it up
			if (PipSceneCapture)
			{
				PipSceneCapture->DestroyComponent();
				PipSceneCapture = nullptr;
			}
			PipRenderTarget = nullptr;
			PipRenderDynamicMaterial = nullptr;

			
		}
	}
}

float UScopedWeaponAttachment::GetMeshspaceOffsetVertical(class ABaseWeapon* Weapon)
{
	if (!Weapon)
		return 0.0f;

	for (int32 i = 0; i < ScopeMods.Num(); i++)
	{
		if (ScopeMods[i].WeaponClass == Weapon->GetClass())
			return ScopeMods[i].VerticalOffset;
	}

	return 0.0f;
}

float UScopedWeaponAttachment::GetMeshspaceOffsetHorizontal(class ABaseWeapon* Weapon)
{

	if (!Weapon)
		return 0.0f;

	for (int32 i = 0; i < ScopeMods.Num(); i++)
	{
		if (ScopeMods[i].WeaponClass == Weapon->GetClass())
			return ScopeMods[i].HorizontalOffset;
	}

	return 0.0f;
}

float UScopedWeaponAttachment::GetMeshspaceOffsetDistance(class ABaseWeapon* Weapon)
{
	if (!Weapon)
		return 0.0f;

	for (int32 i = 0; i < ScopeMods.Num(); i++)
	{
		if (ScopeMods[i].WeaponClass == Weapon->GetClass())
			return ScopeMods[i].DistanceOffset;
	}

	return 0.0f;
}

FScopeModifications UScopedWeaponAttachment::GetScopeMods(class ABaseWeapon* Weapon)
{
	if (!Weapon)
		return FScopeModifications();

	for (int32 i = 0; i < ScopeMods.Num(); i++)
	{
		if (ScopeMods[i].WeaponClass == Weapon->GetClass())
			return ScopeMods[i];
	}

	return FScopeModifications();
}

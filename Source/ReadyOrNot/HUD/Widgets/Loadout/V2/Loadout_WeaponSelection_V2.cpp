// Copyright Void Interactive, 2023


#include "HUD/Widgets/Loadout/V2/Loadout_WeaponSelection_V2.h"

void ULoadout_WeaponSelection_V2::SetShouldUpdateWorkbench(bool ShouldUpdate)
{
	ShouldUpdateWorkbench = ShouldUpdate;
}

bool ULoadout_WeaponSelection_V2::GetShouldUpdateWorkbench()
{
	return ShouldUpdateWorkbench;
}

// void ULoadout_WeaponSelection_V2::SetWorkbenchItemClass(TSubclassOf<ABaseItem> Item, FName Tag, FSavedLoadout ActiveLoadout)
// {
// 	// TODO get rid of troublesome global state
// 	// if (bIsWeaponCustomization)
// 	// {
// 	// 	Tag = "AttachmentWorkbench";
// 	// }
// 
// 	if (ABaseItem** DoubleItemPtr = WorkBenchItemPtrMap.Find(Tag))
// 	{
// 		if (ABaseItem* DereferencedItem = *DoubleItemPtr)
// 		{
// 			DereferencedItem->Destroy();
// 			DereferencedItem = nullptr;
// 		}
// 	}
// 
// 	// if (pc)
// 	// {
// 		AActor* PlacementActor = nullptr;
// 		V_LOGM(LogReadyOrNot, "Searching for bench placement actor of tag %s", *Tag.ToString());
// 		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
// 		{
// 			AActor* a = *It;
// 			if (a->Tags.Contains(Tag))
// 			{
// 				/*float Dist = (a->GetActorLocation() - pc->GetPawn()->GetActorLocation()).Size();
// 				if (Dist < 500.0f)
// 				{
// 					PlacementActor = a;
// 					break;
// 				}*/
// 				PlacementActor = a;
// 				break;
// 			}
// 		}
// 
// 		//must have a bench placement actor otherwise this equipment menu won't work correctly
// 		ensure(PlacementActor);
// 		if (!PlacementActor)
// 			return;
// 
// 		// if (ps)
// 		// {
// 			// FSavedLoadout Loadout = ps->GetLoadout();
// 			WorkBenchItemPtrMap.Add(Tag, GetWorld()->SpawnActor<ABaseItem>(Item, PlacementActor->GetActorTransform()));
// 			ABaseWeapon* bw = Cast<ABaseWeapon>(WorkBenchItemPtrMap[Tag]);
// 			if (bw)
// 			{
// 				bw->Tags.Add("NoRelevancy");
// 				bw->SetReplicates(false);
// 				if (bw->ItemCategories.Contains(EItemCategory::IC_Primary))
// 				{
// 					bw->GetItemMesh()->VisibilityBasedAnimTickOption =
// 						EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
// 					bw->GetItemMesh()->SetSimulatePhysics(true);
// 					bw->GetItemMesh()->SetLinearDamping(1.0f);
// 					bw->GetItemMesh()->SetAngularDamping(10000.0f);
// 					bw->AddAttachment(ActiveLoadout.PrimaryScope);
// 					bw->AddAttachment(ActiveLoadout.PrimaryMuzzle);
// 					bw->AddAttachment(ActiveLoadout.PrimaryOverbarrel);
// 					bw->AddAttachment(ActiveLoadout.PrimaryUnderbarrel);
// 					bw->AddAttachment(ActiveLoadout.PrimaryStock);
// 					bw->AddAttachment(ActiveLoadout.PrimaryGrip);
// 					bw->AddAttachment(ActiveLoadout.PrimaryIlluminator);
// 					bw->AddAttachment(ActiveLoadout.PrimaryAmmunition);
// 
// 					if (ActiveLoadout.PrimarySkin)
// 					{
// 						USkinComponent* SkinComp = NewObject<USkinComponent>(bw, ActiveLoadout.PrimarySkin);
// 						if (SkinComp)
// 						{
// 							SkinComp->RegisterComponent();
// 						}
// 					}
// 					bw->AttachStatic();
// 				}
// 				else if (bw->ItemCategories.Contains(EItemCategory::IC_Secondary))
// 				{
// 					bw->GetItemMesh()->VisibilityBasedAnimTickOption =
// 						EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
// 					bw->GetItemMesh()->SetLinearDamping(1.0f);
// 					bw->GetItemMesh()->SetAngularDamping(10000.0f);
// 					bw->GetItemMesh()->SetSimulatePhysics(true);
// 
// 					bw->AddAttachment(ActiveLoadout.SecondaryScope);
// 					bw->AddAttachment(ActiveLoadout.SecondaryMuzzle);
// 					bw->AddAttachment(ActiveLoadout.SecondaryOverbarrel);
// 					bw->AddAttachment(ActiveLoadout.SecondaryUnderbarrel);
// 					bw->AddAttachment(ActiveLoadout.SecondaryStock);
// 					bw->AddAttachment(ActiveLoadout.SecondaryGrip);
// 					bw->AddAttachment(ActiveLoadout.SecondaryIlluminator);
// 					bw->AddAttachment(ActiveLoadout.SecondaryAmmunition);
// 
// 					if (ActiveLoadout.SecondarySkin)
// 					{
// 						USkinComponent* SkinComp = NewObject<USkinComponent>(bw, ActiveLoadout.SecondarySkin);
// 						if (SkinComp)
// 						{
// 							SkinComp->RegisterComponent();
// 						}
// 					}
// 					bw->AttachStatic();
// 				}
// 			}
// 		// }
// 	// }
// }
// 
// void ULoadout_WeaponSelection_V2::UpdateWorkbenchItemAttachments(FSavedLoadout ActiveLoadout)
// {
// 	FName PrimaryTag = "PrimaryPlacementAttachments";
// 	FName SecondaryTag = "SecondaryPlacementAttachments";
// 	// TODO urgh
// 	// if (bIsWeaponCustomization)
// 	// {
// 	// bIsCustomizingPrimary ? PrimaryTag = "AttachmentWorkbench" : SecondaryTag = "AttachmentWorkbench";
// 	// }
// 	PrimaryTag = "AttachmentWorkbench";
// 	if (WorkBenchItemPtrMap.Find(PrimaryTag))
// 	{
// 		if (ABaseItem* PrimaryItem = *WorkBenchItemPtrMap.Find(PrimaryTag))
// 		{
// 			if (ABaseWeapon* PrimaryBW = Cast<ABaseWeapon>(PrimaryItem))
// 			{
// 				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryScope);
// 				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryMuzzle);
// 				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryOverbarrel);
// 				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryUnderbarrel);
// 				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryStock);
// 				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryGrip);
// 				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryIlluminator);
// 				PrimaryBW->AddAttachment(ActiveLoadout.PrimaryAmmunition);
// 
// 				TArray<USkinComponent*> SkinComps;
// 				PrimaryBW->GetComponents<USkinComponent>(SkinComps);
// 
// 				for (int32 i = 0; i < SkinComps.Num(); i++)
// 				{
// 					USkinComponent* SkinComp = SkinComps[i];
// 					if (SkinComp/* && SkinComp->GetClass() != ActiveLoadout.PrimarySkin*/)
// 					{
// 						SkinComp->ResetSkin();
// 						SkinComp->DestroyComponent();
// 					}
// 				}
// 
// 				if (ActiveLoadout.PrimarySkin)
// 				{
// 					PrimaryBW->GetComponents<USkinComponent>(SkinComps);
// 
// 					if (SkinComps.Num() == 0)
// 					{
// 						USkinComponent* SkinComp = NewObject<USkinComponent>(PrimaryBW, ActiveLoadout.PrimarySkin);
// 						if (SkinComp)
// 						{
// 							SkinComp->RegisterComponent();
// 
// 							if (SkinComp->bResetsToFactorySkin)
// 							{
// 								SkinComp->ResetSkin();
// 							}
// 							else
// 							{
// 								SkinComp->ApplySkin();
// 							}
// 						}
// 					}
// 				}
// 			}
// 		}
// 	}
// 
// 	if (WorkBenchItemPtrMap.Find(SecondaryTag))
// 	{
// 		if (ABaseItem* SecondaryItem = *WorkBenchItemPtrMap.Find(SecondaryTag))
// 		{
// 			if (ABaseWeapon* SecondaryBW = Cast<ABaseWeapon>(SecondaryItem))
// 			{
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryScope);
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryMuzzle);
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryOverbarrel);
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryUnderbarrel);
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryStock);
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryGrip);
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryIlluminator);
// 				SecondaryBW->AddAttachment(ActiveLoadout.SecondaryAmmunition);
// 
// 				TArray<USkinComponent*> SkinComps;
// 				SecondaryBW->GetComponents<USkinComponent>(SkinComps);
// 
// 				for (int32 i = 0; i < SkinComps.Num(); i++)
// 				{
// 					USkinComponent* SkinComp = Cast<USkinComponent>(SkinComps[i]);
// 					if (SkinComp/* && SkinComp->GetClass() != ActiveLoadout.SecondarySkin*/)
// 					{
// 						SkinComp->ResetSkin();
// 						SkinComp->DestroyComponent();
// 					}
// 				}
// 
// 				if (ActiveLoadout.SecondarySkin)
// 				{
// 					SecondaryBW->GetComponents<USkinComponent>(SkinComps);
// 
// 					if (SkinComps.Num() == 0)
// 					{
// 						USkinComponent* SkinComp = NewObject<USkinComponent>(SecondaryBW, ActiveLoadout.SecondarySkin);
// 						if (SkinComp)
// 						{
// 							SkinComp->RegisterComponent();
// 
// 							if (SkinComp->bResetsToFactorySkin)
// 								SkinComp->ResetSkin();
// 							else
// 								SkinComp->ApplySkin();
// 						}
// 					}
// 				}
// 			}
// 		}
// 	}
// }

void ULoadout_WeaponSelection_V2::NativeOnActivated()
{
	Super::NativeOnActivated();
}

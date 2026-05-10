// Void Interactive, 2020


#include "Actors/Triggers/CharacterCustomizationPortal.h"

#include "Components/InteractableComponent.h"
#include "HUD/Widgets/PersonalizationWidget.h"
#include "lib/GameFeatureLibrary.h"

ACharacterCustomizationPortal::ACharacterCustomizationPortal()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
	
	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Interactable Component"));
	InteractableComponent->RequiredLookAtPercentage = 0.8f;
	InteractableComponent->bDistanceFadeIcon = false;
	InteractableComponent->bImprintIconOnHUDUponInteraction = true;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromString("Change Appearance"));
	InteractableComponent->ActionSlot1.bCondition = true;
	InteractableComponent->bClientInteract = true;
	SetRootComponent(InteractableComponent);
	
	CharacterSpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("CharacterSpawnPoint"));
	CharacterSpawnPoint->SetupAttachment(GetRootComponent());

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	BillboardComponent->SetupAttachment(GetRootComponent());
	
}

void ACharacterCustomizationPortal::BeginPlay()
{
	Super::BeginPlay();
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), LightActorsOfTag, OutActors);
	for (AActor* a : OutActors)
	{
		TArray<UStaticMeshComponent*> OutStatics;
		a->GetComponents(OutStatics);
		CompsToOutline.Append(OutStatics);
		TArray<ULightComponent*> OutLights;
		a->GetComponents(OutLights);
		LightsToEnable.Append(OutLights);
	}
}

void ACharacterCustomizationPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const bool bHasPersonalizationWidget = UBpGameplayHelperLib::HasWidgetInViewport("Personalization");

	InteractableComponent->ActionSlot1.bCondition = !bHasPersonalizationWidget;
	InteractableComponent->SetHiddenInGame(bHasPersonalizationWidget);
	if (!bHasPersonalizationWidget)
	{
		APlayerCharacter* LocalCharacter = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());
		if (LocalCharacter && LocalCharacter->IsLocalPlayer() && bHasLocked)
		{
			LocalCharacter->UnlockAim();
			LocalCharacter->UnlockMovement();

			bHasLocked = false;
		}
		if (CustomizationCharacter)
		{
			CustomizationCharacter->Destroy();
			CustomizationCharacter = nullptr;
		}
	}
	
	if (CustomizationCharacter)
	{
		if (!CustomizationCharacter->IsTableMontagePlaying("tp_pregame_idle"))
		{
			CustomizationCharacter->PlayMontageFromTable("tp_pregame_idle");
		}
	}
	
	if (InteractableComponent->ActionSlot1.bCondition)
	{
		InteractableComponent->IsBeingLookedAt(UReadyOrNotStatics::GetReadyOrNotPlayerController(), InteractableComponent->ShowPromptAtDistance, InteractableComponent->RequiredLookAtPercentage) ? DrawOutline() : DisableOutline();
	}
}

void ACharacterCustomizationPortal::UpdateCharacterLookOverride(FName Head, FName Body)
{
	if (LastSetBody != Body || LastSetHead != Head)
	{
		if (CustomizationCharacter)
		{
			LastSetBody = Body;
			LastSetHead = Head;
			CustomizationCharacter->UpdateOverridesFromCharacterLookOverrideDataTable(Body.ToString() + "_" + Head.ToString());
		}
		
	}
}

void ACharacterCustomizationPortal::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (InteractInstigator->IsLocalPlayer())
	{
		AReadyOrNotPlayerController* pc = UReadyOrNotStatics::GetReadyOrNotPlayerController();
		pc->Client_CreateWidget("Personalization");
		for (TObjectIterator<UPersonalizationWidget>It; It; ++It)
		{
			It->SpawnedFromPortal = this;
		}
		APlayerCharacter* LocalCharacter = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());
		if (LocalCharacter)
		{
			LocalCharacter->LockAim();
			LocalCharacter->LockMovement();

			bHasLocked = true;
		}
		
		SpawnCustomizationCharacter(InteractInstigator);
	}
}

void ACharacterCustomizationPortal::SpawnCustomizationCharacter(AReadyOrNotCharacter* InteractInstigator)
{
	if (CustomizationCharacter)
	{
		CustomizationCharacter->Destroy();
		CustomizationCharacter = nullptr;
	}
	
	if (InteractInstigator)
	{
		FTransform SpawnTransform;
		SpawnTransform = CharacterSpawnPoint->GetComponentTransform();
		FRotator SpawnRotation = SpawnTransform.GetRotation().Rotator();
		SpawnRotation.Yaw = UKismetMathLibrary::FindLookAtRotation(SpawnTransform.GetLocation(), InteractInstigator->GetActorLocation()).Yaw;
		SpawnTransform.SetRotation(SpawnRotation.Quaternion());
	    CustomizationCharacter = GetWorld()->SpawnActor<AReadyOrNotCharacter>(InteractInstigator->GetClass(), SpawnTransform);
		CustomizationCharacter->SetGodMode(true);
		if (HasAuthority())
		{
			CustomizationCharacter->SetReplicates(false);
			CustomizationCharacter->TearOff();
		}

		CustomizationCharacter->PlayMontageFromTable("tp_pregame_idle");
		FSavedLoadout Loadout;
        UBpGameplayHelperLib::LoadLoadout(Loadout, "default");
		FLoadoutEquipOptions LoadoutEquipOptions;
		LoadoutEquipOptions.bReplicates = false;
		UBpGameplayHelperLib::EquipLoadoutOnPlayer(Loadout, CustomizationCharacter , LoadoutEquipOptions);
		CustomizationCharacter->GetInventoryComponent()->GetSpawnedGear().Primary->Destroy();
		CustomizationCharacter->GetInventoryComponent()->GetSpawnedGear().Secondary->Destroy();
		CustomizationCharacter->Tags.Add("Personalization");
	}
}

bool ACharacterCustomizationPortal::SaveCharacterLookOverride(FName InHead, FName InBody)
{
	FSavedLoadout Loadout;
	if (UBpGameplayHelperLib::LoadLoadout(Loadout, "default"))
	{
		Loadout.CharacterLookOverride = InBody.ToString() + "_" + InHead.ToString();
		UReadyOrNotStatics::GetReadyOrNotPlayerController()->Server_RequestLoadoutChange(Loadout);
		return UBpGameplayHelperLib::SaveLoadout(Loadout, "default");
	}
	return false;
}

bool ACharacterCustomizationPortal::GetCurrentCharacterLookOverride(FName& OutHead, FName& OutBody)
{
	FSavedLoadout Loadout;
	UBpGameplayHelperLib::LoadLoadout(Loadout, "default");
	FString Head, Body;
	if (Loadout.CharacterLookOverride.Split("_", &Body, &Head))
	{
		OutHead = *Head;
		OutBody = *Body;
		return true;
	}
	return false;
}

void ACharacterCustomizationPortal::GetCustomizationEntries(TArray<FCharacterPersonalizationData>& OutHeads, TArray<FCharacterPersonalizationData>& OutBodys)
{
	
	UDataTable* dt = UBpGameplayHelperLib::GetCharacterLookOverrideDataTable();
	if (dt)
	{
		for (FName row : dt->GetRowNames())
		{
			if (row.ToString().Contains("_"))
			{
				FCharacterLookOverride* LookupRow = dt->FindRow<FCharacterLookOverride>(row, "Character Override Lookup", false);
				if (LookupRow)
				{
					FString Head, Body;
					if (row.ToString().Split("_", &Body, &Head))
					{
						FCharacterPersonalizationData HeadData;
						HeadData.RowName = *Head;
						HeadData.Icon = LookupRow->HeadIcon;
						HeadData.FriendlyName = LookupRow->FriendlyHeadName;
						HeadData.LockedToDLC = LookupRow->LockedToDLC;
						OutHeads.AddUnique(HeadData);
						FCharacterPersonalizationData BodyData;
						BodyData.RowName = *Body;
						BodyData.Icon = LookupRow->BodyIcon;
						BodyData.FriendlyName = LookupRow->FriendlyBodyName;
						BodyData.LockedToDLC = LookupRow->LockedToDLC;
						OutBodys.AddUnique(BodyData);
					}
				}
				
			}
		}
	}
}

bool ACharacterCustomizationPortal::IsDLCLocked(FCharacterPersonalizationData Data)
{
	return UGameFeatureLibrary::IsGameVersionEnabled(Data.LockedToDLC);
}

bool ACharacterCustomizationPortal::GetCharacterLookOverride(FName Head, FName Body, FCharacterLookOverride& OutCharacterLookOverride)
{
	UDataTable* dt = UBpGameplayHelperLib::GetCharacterLookOverrideDataTable();
	if (dt)
	{
		
		FString Combined = Body.ToString() + "_" + Head.ToString();
		FCharacterLookOverride* LookupRow = dt->FindRow<FCharacterLookOverride>(*Combined, "Character Override Lookup", false);
        if (LookupRow)
        {
        	OutCharacterLookOverride = *LookupRow;
        	return true;
        }
	}
	return false;
}

void ACharacterCustomizationPortal::GetAllCompatibleBodies(FName InHead, TArray<FName>& OutBodies)
{
	UDataTable* dt = UBpGameplayHelperLib::GetCharacterLookOverrideDataTable();
	if (dt)
	{
		for (FName row : dt->GetRowNames())
		{
			if (row.ToString().Contains("_"))
			{
				if (!row.ToString().Contains(InHead.ToString()))
					continue;
				
				FString Head, Body;
				if (row.ToString().Split("_", &Body, &Head))
				{
					OutBodies.AddUnique(*Body);
				}
				
			}
		}
	}
}

void ACharacterCustomizationPortal::GetAllCompatibleHeads(FName InBody, TArray<FName>& OutHeads)
{
	UDataTable* dt = UBpGameplayHelperLib::GetCharacterLookOverrideDataTable();
	if (dt)
	{
		for (FName row : dt->GetRowNames())
		{
			if (row.ToString().Contains("_"))
			{
				if (!row.ToString().Contains(InBody.ToString()))
					continue;
				
				FString Head, Body;
				if (row.ToString().Split("_", &Body, &Head))
				{
					OutHeads.AddUnique(*Head);
				}
				
			}
		}
	}
}

void ACharacterCustomizationPortal::DrawOutline()
{
}

void ACharacterCustomizationPortal::DisableOutline()
{
}

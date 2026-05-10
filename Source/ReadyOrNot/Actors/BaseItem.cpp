// Copyright Void Interactive, 2023

#include "BaseItem.h"

#include "GameModes/VIPEscortGS.h"

#include "AIController.h"
#include "Door.h"
#include "NavigationSystem.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "Components/InteractableComponent.h"
#include "Components/ScoringComponent.h"

#include "Audio/RoNSoundData.h"

#include "Info/ReadyOrNotSignificanceManager.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

#include "ReadyOrNotDebugSubsystem.h"
#include "ThrownEvidenceActor.h"
#include "Engine/AssetManager.h"
#include "Gameplay/CollectedEvidenceActor.h"
#include "Gameplay/IncapacitatedHuman.h"
#include "Subsystems/AchievementSubsystem.h"


DECLARE_CYCLE_STAT(TEXT("RoN ~ Item Tick"), STAT_BaseItemTick, STATGROUP_BaseItem);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Item Tick ~ No Owner Branch"), STAT_BaseItemTick_NoOwner, STATGROUP_BaseItem);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Item Tick ~ Update FOV Shader"), STAT_FOVShaderTick, STATGROUP_BaseItem);
DECLARE_CYCLE_STAT(TEXT("RoN ~ Item Tick ~ Nav Mesh Bounds Check"), Stat_ItemNavMeshBoundsCheck, STATGROUP_BaseItem);

ABaseItem::ABaseItem()
{
	IFMODStudioModule::Get();

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.033f;
	
	bNetUseOwnerRelevancy = true;
	NetUpdateFrequency = 15.0f;
	MinNetUpdateFrequency = 2.0f;
	NetPriority = 2.0f;

	bDisableTickWhenNotEquipped = true;
	
	SceneComp = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(SceneComp);
	
	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>("ItemMesh");
	ItemMesh->SetupAttachment(SceneComp);
	ItemMesh->SetCollisionProfileName("Item");
	ItemMesh->SetIsReplicated(false);
	ItemMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	ItemMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	ItemMesh->SetOnlyOwnerSee(false);
	ItemMesh->SetCastShadow(true);
	ItemMesh->SetReceivesDecals(false);
	ItemMesh->SetAngularDamping(1.0f);
	ItemMesh->SetNotifyRigidBodyCollision(true);
	ItemMesh->bAlwaysCreatePhysicsState = true;
	ItemMesh->bEnableUpdateRateOptimizations = false; // Disabled because it causes issues with gear that has to follow the main players skele and it will fall out of sync..
	ItemMesh->bUpdateOverlapsOnAnimationFinalize = false;
	ItemMesh->bSimulationUpdatesChildTransforms = false;
	ItemMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	ItemMesh->bVisibleInRayTracing = false;
	ItemMesh->bCastHiddenShadow = false;
	ItemMesh->bLightAttachmentsAsGroup = true;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>("Interaction Box");
	InteractionBox->AreaClass = nullptr;
	InteractionBox->SetRelativeLocation(FVector::ZeroVector);
	InteractionBox->SetBoxExtent(FVector(5.0f, 15.0f, 15.0f));
	InteractionBox->SetCollisionProfileName("Item");
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_SOUND, ECR_Ignore);
	InteractionBox->SetNotifyRigidBodyCollision(false);
	InteractionBox->SetGenerateOverlapEvents(false);
	InteractionBox->SetEnableGravity(false);
	InteractionBox->SetVisibility(false);
	InteractionBox->SetCanEverAffectNavigation(false);
	InteractionBox->bApplyImpulseOnDamage = false;
	InteractionBox->bIgnoreRadialForce = true;
	InteractionBox->bIgnoreRadialImpulse = true;
	InteractionBox->SetupAttachment(ItemMesh);
	
	FMODAudioComp = CreateDefaultSubobject<UFMODAudioComponent>("FMODAudioPlayer");
	FMODAudioComp->SetupAttachment(ItemMesh);

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Interactable Component"));
	InteractableComponent->ShowPromptAtDistance = 240.0f;
	InteractableComponent->bImprintIconOnHUDUponInteraction = true;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "PickupItem"));
	InteractableComponent->ActionSlot1.bCheckForDisallowedItems = false;
	InteractableComponent->ActionSlot2.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "SecuringEvidence");
	InteractableComponent->ActionSlot2.bAnimate = true;
	InteractableComponent->ActionSlot2.bLoopAnimation = true;
	InteractableComponent->ActionSlot1.bCondition = false;
	InteractableComponent->ActionSlot2.bCondition = false;
	InteractableComponent->ActionSlot3.bCondition = false;
	InteractableComponent->ActionSlot4.bCondition = false;
	InteractableComponent->SetupAttachment(ItemMesh);
	InteractableComponent->SetRelativeLocation(FVector::ZeroVector);
	
	ScoringComponent = CreateDefaultSubobject<UScoringComponent>(TEXT("Scoring Component"));
	ScoringComponent->bAutoAddToScorePool = false;
	ScoringComponent->ScoreGroupName = "EvidenceSecured";
	ScoringComponent->ObjectiveLevel = EObjectiveLevel::SecondaryObjective;

	FreeAimLimit = 4.0f;
	FreeAimLimitADS = 4.0f;

	LazySpringStrength = 1.0f;
	LazySpringStrengthADS = 1.0f;

	bShowStaticMeshOnBody = true;
	bIsOneHandedItem = false;

	/* We need a default value so non-weapon items still have move motion weight */
	CUR_FPS_ADS_Weight = 1.0f;

	bReplicates = true;
	AActor::SetReplicateMovement(true);
}

void ABaseItem::SpawnThrownItemAtTransform(const FTransform& Transform, const FVector& ThrowDirection, const FVector& ThrowLocation)
{
}

void ABaseItem::OnPhysicsImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherComp && (OtherComp->GetOwner() == this || OtherComp->GetAttachmentRootActor() == this) && OtherComp != ItemMesh && HitComponent == ItemMesh)
	{
		OtherComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	const float NormalImpulseSize = NormalImpulse.Size();
	//ULog::Number(NormalImpulseSize, "PhysicsImpact: Velocity: ");
	//FString DebugStr = "Physics Impact: {0} {1} {2}";
	//DebugStr = FString::Format(*DebugStr, {GetNameSafe(HitComponent), GetNameSafe(OtherActor), GetNameSafe(OtherComp)});
	//DrawDebugString(GetWorld(), GetItemLocation(), DebugStr, nullptr, FColor::White, 0.0167f);
    if (NormalImpulseSize > 10.0f)
    {
    	// Apply a small amount of damage based on the impact velocity, ignore characters
    	if (OtherActor && !OtherActor->IsA(AReadyOrNotCharacter::StaticClass()) && HasAuthority())
    	{
    		Server_ApplyPointDamage(OtherActor, NormalImpulseSize/1000.0f, -NormalImpulse, Hit, nullptr, this, UDamageType::StaticClass());
    	}
    	
		PlayHitImpactSound(Hit, NormalImpulseSize);
    }
}

void ABaseItem::PlayHitImpactSound(const FHitResult& Hit, const float Impulse)
{
	if (HitImpactCount > 2)
		return;
	
	if (Impulse < 200.0f)
		return;

	// Once collected as evidence, don't try to play a hit sound
	if (IsPendingKillPending())
		return;

	// don't trigger this on invisible components (and therefore hear a double sound sometimes)
	if (!ItemMesh->IsVisible())
		return;

	if (UFMODBlueprintStatics::EventInstanceIsValid(HitEventInstance))
		return;

	if (SoundData && Impulse >= SoundData->PhysicsImpactMinimumVelocity)
	{
		HitImpactCount++;
		HitEventInstance = UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), SoundData->PhysicsImpact, GetActorTransform(), true);
		if (HitEventInstance.Instance)
		{
			HitEventInstance.Instance->setParameterByName("Material", UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()));
			HitEventInstance.Instance->setParameterByName("Velocity", Impulse);
		}
	}
}

void ABaseItem::Server_ApplyPointDamage_Implementation(AActor* DamagedActor, float BaseDamage, FVector const& HitFromDirection, FHitResult const& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass)
{
	UGameplayStatics::ApplyPointDamage(DamagedActor, BaseDamage, HitFromDirection, HitInfo, EventInstigator, DamageCauser, DamageTypeClass);
}

void ABaseItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseItem, AnimationData);
	DOREPLIFETIME(ABaseItem, Rep_CustomItemMeshFromAttachment);

	DOREPLIFETIME(ABaseItem, Skin);
	
	DOREPLIFETIME(ABaseItem, TargetWorldScale);
	DOREPLIFETIME(ABaseItem, TargetWorldScaleInterpSpeed);
	DOREPLIFETIME(ABaseItem, AnimationIndex1P);
	DOREPLIFETIME(ABaseItem, AnimationIndex3P);
	DOREPLIFETIME(ABaseItem, HandsSocket);
	DOREPLIFETIME(ABaseItem, BodySocket);

	DOREPLIFETIME(ABaseItem, bStartAsEvidence);
	DOREPLIFETIME(ABaseItem, bIsEvidence);
	DOREPLIFETIME(ABaseItem, bIsClearable);
	DOREPLIFETIME(ABaseItem, bDropping);
	DOREPLIFETIME(ABaseItem, MasterPoseRep);
	//DOREPLIFETIME(ABaseItem, bForceInvisible);
	DOREPLIFETIME(ABaseItem, bHasBeenCleared);
	DOREPLIFETIME(ABaseItem, bEasyPickup);
	DOREPLIFETIME(ABaseItem, bNoPickup);
	DOREPLIFETIME(ABaseItem, bInInventory);
	DOREPLIFETIME(ABaseItem, Server_ReplicatedPhysicsLocation);

	DOREPLIFETIME(ABaseItem, bIsBeingCollected);
	DOREPLIFETIME(ABaseItem, CurrentCollectionTime);
	DOREPLIFETIME(ABaseItem, MaxCollectionTime);
	DOREPLIFETIME(ABaseItem, CollectingCharacter);

	//DOREPLIFETIME(ABaseItem, ThrownEvidenceActor);
}

void ABaseItem::BeginPlay()
{
	Super::BeginPlay();

	InitializeFOVMaterials();

	OnRep_Skin();
	
	if (AReadyOrNotGameState* GS = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState()))
	{
		GS->AllItems.AddUnique(this);
	}
	
	ItemMesh->SetNotifyRigidBodyCollision(true);
	ItemMesh->OnComponentHit.RemoveAll(this);
	ItemMesh->OnComponentHit.AddDynamic(this, &ABaseItem::OnPhysicsImpact);
	
	// hide a 1frame middle of screen glitch when spawning for players
	OriginalTransform = GetActorTransform();
	if (!bDeployable && !bStartAsEvidence && !bIsEvidence)
	{
		UReadyOrNotSignificanceManager::RegisterActorWithSignificanceManager(this);

		SetActorLocation(FVector(0.0f, 0.0f, -50000.0f));
	}
	
	//ApplyLookupData(false);
	SetupBaseEvents();
	
	DefaultAnimationData = AnimationData;

	DefaultAnimationIndex1P = AnimationIndex1P;
	DefaultAnimationIndex3P = AnimationIndex3P;

	// // Get the item class from the data table and store it
	// if (const UDataTable* DT = UBpGameplayHelperLib::GetItemLookupDataTable())
	// {
	// 	TArray<FName> RowNames = DT->GetRowNames();
	// 	
	// 	for (const FName& RowName : RowNames)
	// 	{
	// 		FItemLookupTable* LookUpRow = DT->FindRow<FItemLookupTable>(RowName, "Item Lookup");
	// 		if (LookUpRow && LookUpRow->ItemName.ToString() == ItemName)
	// 		{
	// 			ItemClass = LookUpRow->ItemClass;
	// 			break;
	// 		}
	// 	}
	// }

	if (bStartAsEvidence || bIsEvidence)
	{
		MarkAsEvidence(true);
	}

	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ABaseItem::SetOriginalTransform, 0.25);
	
	PrimaryActorTick.TickInterval = 0.033f;
}

void ABaseItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	ScoringComponent->RemoveFromScorePool();
	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(this);
	
	if (AReadyOrNotGameState* GS = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState()))
	{
		GS->AllItems.Remove(this);
	}
}

void ABaseItem::FellOutOfWorld(const UDamageType& dmgType)
{
	if (HasAuthority() || GetLocalRole() == ROLE_None)
	{
		if (LOCAL_PLAYER)
		{
			ItemMesh->SetWorldLocation(LocalPlayer->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
			CompleteEvidenceCollection_COOP(LocalPlayer);
		}
	}
}

void ABaseItem::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	SCOPE_CYCLE_COUNTER(STAT_BaseItemTick);

	if (!GetWorld() || !ItemMesh || (GetWorld() && GetWorld()->bIsTearingDown))
	{
		return;
	}

	const AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		SCOPE_CYCLE_COUNTER(STAT_BaseItemTick_NoOwner);
		
		bInInventory = false;
		
		DisableWeaponFovShader();

		if (bIsBeingCollected && CollectingCharacter)
		{
			if (!CollectingCharacter->HasCollectionAnimTriggered())
			{
				CollectingCharacter->TriggerCollectionAnim();
			}
			
			CurrentCollectionTime += DeltaTime;
		}

		// Want to remove this
		if (UReadyOrNotFunctionLibrary::IsCoop(GetWorld()) && UReadyOrNotFunctionLibrary::HasStartedMatch(GetWorld()) && bMarkAsEvidenceWhenNoOwner)
		{
			MarkAsEvidence(true);
		}

		//if (ItemMesh)
		{
			ItemMesh->SetCastShadow(true);
			
			if (InteractableComponent)
			{
				InteractableComponent->bEnabled = !bInInventory && !IsHidden() && ItemMesh->IsVisible() && !Tags.Contains("CustomizationMenu");
				InteractableComponent->bImprintIconOnHUDUponInteraction = !IsEvidence();
				InteractableComponent->SetAnimatedIconName(DetermineAnimatedIcon_Implementation());
				InteractableComponent->ActionSlot1.ActionText = DetermineActionText_Implementation();
				InteractableComponent->ActionSlot1.InputEvent = DetermineInputEvent_Implementation();
				InteractableComponent->ActionSlot1.bCondition = !bInInventory && !bIsBeingCollected;
				InteractableComponent->ActionSlot2.bCondition = bIsBeingCollected;
				InteractableComponent->CurrentProgress = bIsBeingCollected ? FMath::Clamp(CurrentCollectionTime/MaxCollectionTime, 0.0f, 1.0f) : 0.0f;
			}

			if (IsEvidence())
			{
				// Only update interaction box if the bounds have changed
				if (ItemMesh->SkeletalMesh)
				{
					if (IsValid(InteractionBox))
					{
						if (InteractionBox->GetUnscaledBoxExtent() != ItemMesh->GetCachedLocalBounds().BoxExtent)
							InteractionBox->SetBoxExtent(ItemMesh->GetCachedLocalBounds().BoxExtent);
					}
				}
			}

			if (bIsEvidence)
			{
				if (!ItemMesh->IsAnySimulatingPhysics())
					ItemMesh->SetSimulatePhysics(true);
			}

			if (ItemMesh->IsAnySimulatingPhysics())
			{
				SetActorEnableCollision(true);
				SetActorTickInterval(0.033f);
				
				if (!UReadyOrNotSignificanceManager::IsActorRelevant(this))
				{
					UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(this);
				}
				
				ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				ItemMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
				ItemMesh->SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Ignore);
				ItemMesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
				
				ItemMesh->SetAllUseCCD(true);
				ItemMesh->SetNotifyRigidBodyCollision(true);
				ItemMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
				
				// slow down the object
				// stop it flyign into space if we turned on physics while it is colliding
				//LOG_NUMBER(ItemMesh->GetPhysicsLinearVelocity().Size());
				if (ItemMesh->GetPhysicsLinearVelocity().Size() > 1000.0f)
				{
					ItemMesh->SetPhysicsLinearVelocity(-1 * ItemMesh->GetPhysicsLinearVelocity(), true);
				}
				
				// Keep it within the navmesh (check every 0.1 sec)
				TimeSinceLastNavMeshCheck -= DeltaTime;
				if (TimeSinceLastNavMeshCheck <= 0.0f)
				{
					TimeSinceLastNavMeshCheck = 0.1f;
					
					SCOPE_CYCLE_COUNTER(Stat_ItemNavMeshBoundsCheck);
					
					if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(this))
					{
						FNavLocation NavLocation;
						constexpr float ZHeight = 200.0f;
						const FVector Extent = FVector(75.0f, 75.0f, ZHeight);
						const bool bSuccess = NavSys->ProjectPointToNavigation(GetItemLocation() - FVector::UpVector * (ZHeight - 40.0f), NavLocation, Extent);

						//DrawDebugBox(GetWorld(), GetItemLocation() - FVector::UpVector * (ZHeight - 40.0f), Extent, FColor::Cyan, false, 0.033f);
						FVector AdjustedLocation = FVector::ZeroVector;
						if (bSuccess && NavLocation.Location != FVector::ZeroVector)
						{
							TimeStuck = 0.0f;
							
							//LOG_NUMBER(FVector::Distance(GetItemLocation(), NavLocation.Location));
							
							const bool bSameXY = FMath::IsNearlyEqual(GetItemLocation().X, NavLocation.Location.X, 40.0f) &&
												FMath::IsNearlyEqual(GetItemLocation().Y, NavLocation.Location.Y, 40.0f); // 40 = Nav agent radius 
							
							if (!bSameXY && FVector2D::Distance(FVector2D(GetItemLocation()), FVector2D(NavLocation.Location)) > 40.0f) // 40 = Nav agent radius
							{
								AdjustedLocation = FVector(NavLocation.Location.X, NavLocation.Location.Y, GetItemLocation().Z);
								//ULog::Info("NavMesh: Out of bounds of navmesh " + GetName());
							}
							// less than 50cm below the nav mesh?
							else if (GetItemLocation().Z < (NavLocation.Location.Z - 50.0f))
							{
								AdjustedLocation = NavLocation.Location;
								//ULog::Info("NavMesh: " + GetName() + " below nav mesh by " + FString::SanitizeFloat((NavLocation.Location.Z - 35.0f) - GetItemLocation().Z));
							}

							// If adjusted location is zero, then we're fine, we're in the bounds of the navmesh
							// Only adjust when the conditions above are satisfied
							if (AdjustedLocation != FVector::ZeroVector)
							{
								ItemMesh->SetWorldLocation(AdjustedLocation, false, nullptr, ETeleportType::TeleportPhysics);
								//ULog::Info("NavMesh: Adjusting " + GetName());
							}
						}
						// failsafe for when we couldn't successfully project on the the navmesh
						else
						{
							TimeStuck += TimeSinceLastNavMeshCheck;
							if (TimeStuck > 30.0f)
							{
								if (LOCAL_PLAYER)
									CompleteEvidenceCollection_COOP(LocalPlayer);
							}
						}

						//DrawDebugPoint(GetWorld(), NavLocation.Location, 5.0f, FColor::Yellow, false, DeltaTime);
					}
				}
			}
			else
			{
				if (LOCAL_PLAYER)
				{
					SetActorTickInterval(FMath::GetMappedRangeValueClamped(FVector2D(500.0f, 3000.0f), FVector2D(0.033f, 0.1f), FVector::Distance(LocalPlayer->GetActorLocation(), GetItemLocation())));
				}
				else
				{
					SetActorTickInterval(0.033f);
				}
			}
		}
		
		return;
	}
	
	// Note(Ali): Needed for weapons to stay attached to our character, at all times
	// We dont know who will offset this from frame to frame, so always force a zero relative transform
	if (GetAttachParentActor())
	{
		if (!GetRootComponent()->GetRelativeTransform().EqualsNoScale(FTransform()))
		{
			SetActorRelativeTransform(FTransform());
		}
	}

	const bool bIsEquipped = IsEquipped();
	
	if (bIsEquipped)
		PreviousOwner = GetOwnerCharacter();
	
	if (bIsEquipped && !bDisableWeaponFOV_FromNotify)
	{
		if (!bEnabledWeaponFovShader)
			EnableWeaponFovShader();
		
		UpdateFOVShader(DeltaTime);
	}
	else
	{
		DisableWeaponFovShader();
	}

	if (InteractableComponent)
	{
		InteractableComponent->SetCanEverAffectNavigation(false);
		
		InteractableComponent->bEnabled = false;
		InteractableComponent->ActionSlot1.bCondition = false; // Use slot
		InteractableComponent->ActionSlot2.bCondition = false; // Securing evidence slot
	}

	if (bIsEquipped)
		PrimaryActorTick.TickInterval = 0.0f;
	else
		PrimaryActorTick.TickInterval = 0.1f;
}

void ABaseItem::SetOriginalTransform()
{
	if (!bHasSetOriginalTransform)
	{
		bHasSetOriginalTransform = true;
		if (!GetAttachParentActor())
		{
			SetActorTransform(OriginalTransform);
		}
	}
}

bool ABaseItem::CanEquip(AReadyOrNotCharacter* ToCharacter) const
{
	if (GetOwnerCharacter())
	{
		if (GetOwnerCharacter()->IsCarried() || GetOwnerCharacter()->IsCarrying())
			return false;
	}
	
	return true;
}

bool ABaseItem::CanShowActionSlot1_Implementation(AReadyOrNotCharacter* PC)
{
	if (!PC)
		return !bInInventory;

	if (PC != GetOwnerCharacter())
		return false;

	return !IsEquipped() && !PC->GetInventoryComponent()->HasInventoryItem(this) && !IsHidden() && !bInInventory && !bIsBeingCollected;
}

void ABaseItem::InterpToTargetScale(FVector NewScale, float InterpSpeed)
{
	TargetWorldScale = NewScale;
	TargetWorldScaleInterpSpeed = InterpSpeed;
	bInterpToTargetScale = true;
}

bool ABaseItem::ConsumeMouseMovement(FRotator RotateVector)
{
	return false;
}

bool ABaseItem::ConsumeIncrementalUse(float UseAmount)
{
	return false;
}

bool ABaseItem::ConsumeLeanInput(float leanAmount)
{
	return false;
}

bool ABaseItem::ConsumeMovementForward(float Val)
{
	return false;
}

bool ABaseItem::ConsumeMovementRight(float Val)
{
	return false;
}

bool ABaseItem::ConsumeSprintInput()
{
	return false;
}

bool ABaseItem::ConsumeCrouchInput()
{
	return false;
}

bool ABaseItem::ConsumeJumpInput()
{
	return false;
}

void ABaseItem::OnItemUseComplete()
{

}

void ABaseItem::SetItemVisibility(bool bNewVisibility)
{
	if (bNewVisibility != ItemMesh->GetVisibleFlag())
	{
		TArray<USceneComponent*> SceneComps;
		GetComponents(SceneComps);
		for (USceneComponent* Scene : SceneComps)
		{
			Scene->SetVisibility(bNewVisibility);
		}
	}
}

void ABaseItem::SetItemHiddenInSceneCapture(bool bNewHiddenInSceneCapture)
{
	if (bNewHiddenInSceneCapture != ItemMesh->bHiddenInSceneCapture)
	{
		TArray<UPrimitiveComponent*> PrimitiveComponents;
		GetComponents<UPrimitiveComponent>(PrimitiveComponents);
		
		for (UPrimitiveComponent* Scene : PrimitiveComponents)
		{
			Scene->SetHiddenInSceneCapture(bNewHiddenInSceneCapture);
		}
	}
}

void ABaseItem::OnItemPrimaryUse()
{
	
	OnItemPrimaryUseStart.Broadcast(this);
	UAchievementStatics::UsedItem(GetWorld(),this);
}

void ABaseItem::OnItemPrimaryUseEnd()
{

}

void ABaseItem::OnItemSecondaryUsed()
{
	UAchievementStatics::UsedItem(GetWorld(),this);
}

void ABaseItem::OnItemEndSecondaryUse()
{

}

void ABaseItem::Client_OnItemPickedUp_Implementation(AActor* NewOwner, bool bEquipped)
{
	/*if (const APlayerCharacter* NewOwnerCharacter = Cast<APlayerCharacter>(NewOwner))
	{
		if (bOverrideBreathingEvent && NewOwnerCharacter->IsLocallyControlled())
		{
			if (NewOwnerCharacter->GetFMODBreathingComp())
				NewOwnerCharacter->GetFMODBreathingComp()->SetEvent(BreathingAudioOverride);
		}
	}*/
}

void ABaseItem::GetMovementSpeedScale(FVector& OutMovementSpeedScale,
	FRotator& OutMovementSpeedRotationScalePitchYawRoll)
{
	OutMovementSpeedScale = MovementSpeedScale;
	OutMovementSpeedRotationScalePitchYawRoll = MovementSpeedRotationScalePitchYawRoll;
}

bool ABaseItem::GetMeshspaceTransform(FTransform& Default, FTransform& Aiming, FTransform& Back)
{
	Default = MeshspaceTransform_Default;
	Aiming = MeshspaceTransform_Aiming;
	Back = MeshspaceTransform_Back;
	return true;
}

void ABaseItem::ResetAnimationIndex()
{
	AnimationIndex1P = DefaultAnimationIndex1P;
	AnimationIndex3P = DefaultAnimationIndex3P;
}

bool ABaseItem::ContainsItemCategory(EItemCategory TestCategory) const
{
	return ItemCategories.Contains(TestCategory);
}

FName ABaseItem::GetEquipSocket()
{
	return BodySocket;
}

bool ABaseItem::IsMontagePlaying(UAnimMontage* Montage, bool bIncludeFP, bool bIncludeTP)
{
	bool bIsFPMontagePlaying = false;
	bool bIsTPMontagePlaying = false;
	if (ItemMesh->GetAnimInstance() && bIncludeFP)
	{
		bIsFPMontagePlaying = ItemMesh->GetAnimInstance()->Montage_IsPlaying(Montage);
	}

	if (ItemMesh->GetAnimInstance() && bIncludeTP)
	{
		bIsTPMontagePlaying = ItemMesh->GetAnimInstance()->Montage_IsPlaying(Montage);
	}

	if (bIsFPMontagePlaying || bIsTPMontagePlaying)
	{
		return true;
	}
	
	return false;
}

void ABaseItem::PlayFPMontage(UAnimMontage* NewMontage, float PlayRate)
{
	if (!IsLocallyControlled())
		return;
	
	if (!GetItemMesh()->GetAnimInstance())
		return;

	Client_PlayFPMontage(NewMontage, PlayRate);
}

class UAnimMontage * ABaseItem::GetCurrentFPMontage()
{
	if (!IsLocallyControlled())
		return nullptr;
	
	UAnimInstance * AnimInstance = (ItemMesh) ? ItemMesh->GetAnimInstance() : nullptr;
	if (AnimInstance)
	{
		return AnimInstance->GetCurrentActiveMontage();
	}

	return nullptr;
}

class UAnimMontage * ABaseItem::GetCurrentTPMontage()
{
	if (IsLocallyControlled())
		return nullptr;
	
	UAnimInstance * AnimInstance = (ItemMesh) ? ItemMesh->GetAnimInstance() : nullptr;
	if (AnimInstance)
	{
		return AnimInstance->GetCurrentActiveMontage();
	}

	return nullptr;
}

void ABaseItem::StopFPMontage(class UAnimMontage* AnimMontage)
{
	if (!IsLocallyControlled())
		return;
	
	UAnimInstance * AnimInstance = (GetItemMesh()) ? GetItemMesh()->GetAnimInstance() : nullptr;
	UAnimMontage * MontageToStop = (AnimMontage) ? AnimMontage : GetCurrentFPMontage();

	bool bShouldStopMontage = AnimInstance && MontageToStop && !AnimInstance->Montage_GetIsStopped(MontageToStop);

	if (bShouldStopMontage)
	{
		AnimInstance->Montage_Stop(MontageToStop->BlendOut.GetBlendTime(), MontageToStop);
	}
}

void ABaseItem::PlayLocalFPMontage(UAnimMontage* NewMontage, const float PlayRate)
{
	//Client_PlayFPMontage(NewMontage, PlayRate);
	Client_PlayFPMontage_Implementation(NewMontage, PlayRate);
	
	//if (!IsLocallyControlled())
	//	return;
	//
	//if (!GetItemMesh()->GetAnimInstance())
	//	return;
//
	//GetItemMesh()->GetAnimInstance()->Montage_Play(NewMontage, PlayRate);
}

void ABaseItem::StopTPMontage(class UAnimMontage* AnimMontage)
{
	if (IsLocallyControlled())
		return;
		
	UAnimInstance * AnimInstance = (GetItemMesh()) ? GetItemMesh()->GetAnimInstance() : nullptr;
	UAnimMontage * MontageToStop = (AnimMontage) ? AnimMontage : GetCurrentTPMontage();

	bool bShouldStopMontage = AnimInstance && MontageToStop && !AnimInstance->Montage_GetIsStopped(MontageToStop);

	if (bShouldStopMontage)
	{
		AnimInstance->Montage_Stop(MontageToStop->BlendOut.GetBlendTime(), MontageToStop);
	}
}

void ABaseItem::Client_PlayFPMontage_Implementation(UAnimMontage* NewMontage, float PlayRate)
{
	if (!NewMontage)
		return;
	
	if (!IsLocallyControlled())
		return;
	
	if (!GetItemMesh()->GetAnimInstance())
		return;

	GetItemMesh()->GetAnimInstance()->Montage_Play(NewMontage, PlayRate);
}

bool ABaseItem::IsPlayingHolster() const
{
	if (AnimationData)
	{
		return IsItemAnimationPlaying(AnimationData->Holster, false, true) ||
				IsItemAnimationPlaying(AnimationData->Crouch_Holster, false, true);
	}

	return false;
}

bool ABaseItem::IsPlayingDraw() const
{
	if (AnimationData)
	{
		return IsItemAnimationPlaying(AnimationData->Draw, false, true) ||
				IsItemAnimationPlaying(AnimationData->Crouch_Draw, false, true) ||
				IsItemAnimationPlaying(AnimationData->DrawFirst, false, true) ||
				IsItemAnimationPlaying(AnimationData->Crouch_DrawFirst, false, true);
	}

	return false;
}

bool ABaseItem::PlayDraw(bool bDrawFirst)
{
 	AReadyOrNotCharacter* pc = Cast<AReadyOrNotCharacter>(GetOwner());
	if (!pc)
		return false;

	// never draw an invisible item
	//bForceInvisible = false;

	bDrawnBefore = true;

	if (bIsClearable && bIsEvidence && pc->IsLocalPlayer())
	{
		PlayWeaponCleaning();
		return false;
	}

	if (SoundData && pc->IsLocalPlayer())
	{
		if (bDrawFirst)
		{
			if (SoundData->DrawFirst)
			{
				PlayFMODAudio(SoundData->DrawFirst);
			}
			else
			{
				PlayFMODAudio(SoundData->Draw);
			}
		}
		else
		{
			if (SoundData->Draw)
			{
				PlayFMODAudio(SoundData->Draw);
			}
			else
			{
				PlayFMODAudio(SoundData->DrawFirst);
			}
		}
	}

	if (AnimationData)
	{
		const FString ServerClientStr = GetNetMode() == NM_Client ? "[Client]" : "[Server]";
		V_LOGM(LogReadyOrNot, "%s Playing draw for %s", *ServerClientStr, *GetName());

		const float PlayRate = bDrawFirst ? 1.0f : 1.4f;
		if (bDrawFirst)
		{
			if (pc->bIsCrouched)
			{
				PlayLocalFPMontage(AnimationData->Crouch_DrawFirst.Gun_FP, PlayRate);
				PlayLocalTPMontage(AnimationData->Crouch_DrawFirst.Gun_TP, PlayRate);

				pc->PlayLocal1PMontage(AnimationData->Crouch_DrawFirst.Body_FP, PlayRate);

				if (AnimationData->Crouch_DrawFirst.Body_TP)
					pc->PlayLocal3PMontage(AnimationData->Crouch_DrawFirst.Body_TP, PlayRate);
				else
					pc->PlayLocal3PMontage(AnimationData->Crouch_Draw.Body_TP, PlayRate);

				if (AnimationData->Crouch_DrawFirst.Body_FP)
				{	
					UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_DrawComplete, this,  &ABaseItem::OnDrawComplete, (AnimationData->Crouch_DrawFirst.Body_FP->GetPlayLength() / PlayRate) * 0.75f);
					return true;
				}
			}
			else
			{
				PlayLocalFPMontage(AnimationData->DrawFirst.Gun_FP, PlayRate);
				PlayLocalTPMontage(AnimationData->DrawFirst.Gun_TP, PlayRate);

				pc->PlayLocal1PMontage(AnimationData->DrawFirst.Body_FP, PlayRate);

				if (AnimationData->DrawFirst.Body_TP)
					pc->PlayLocal3PMontage(AnimationData->DrawFirst.Body_TP, PlayRate);
				else
					pc->PlayLocal3PMontage(AnimationData->Draw.Body_TP, PlayRate);
				
				if (AnimationData->DrawFirst.Body_FP)
				{
					UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_DrawComplete, this, &ABaseItem::OnDrawComplete, (AnimationData->DrawFirst.Body_FP->GetPlayLength() / PlayRate) * 0.75f);
					return true;
				}
			}
		}
		else
		{
			if (pc->bIsCrouched)
			{
				PlayLocalFPMontage(AnimationData->Crouch_Draw.Gun_FP, PlayRate);
				PlayLocalTPMontage(AnimationData->Crouch_Draw.Gun_TP, PlayRate);

				pc->PlayLocal1PMontage(AnimationData->Crouch_Draw.Body_FP, PlayRate);
				pc->PlayLocal3PMontage(AnimationData->Crouch_Draw.Body_TP, PlayRate);
				
				if (AnimationData->Crouch_Draw.Body_FP)
				{
					UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_DrawComplete, this, &ABaseItem::OnDrawComplete, (AnimationData->Crouch_Draw.Body_FP->GetPlayLength() / PlayRate) * 0.75f);
					return true;
				}
			}
			else
			{
				PlayLocalFPMontage(AnimationData->Draw.Gun_FP, PlayRate);
				PlayLocalTPMontage(AnimationData->Draw.Gun_TP, PlayRate);

				pc->PlayLocal1PMontage(AnimationData->Draw.Body_FP, PlayRate);
				pc->PlayLocal3PMontage(AnimationData->Draw.Body_TP, PlayRate);

				if (AnimationData->Draw.Body_FP)
				{
					UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_DrawComplete, this, &ABaseItem::OnDrawComplete, (AnimationData->Draw.Body_FP->GetPlayLength() / PlayRate));
					return true;
				}
			}
		}
	}
	// AI characters don't draw with player animations....
    if (Cast<ACyberneticCharacter>(GetOwnerCharacter()))
    {
	    return true;
    }
	return false;
}

void ABaseItem::ClientPlayDraw_Implementation(bool bDrawFirst)
{
	PlayDraw(bDrawFirst);
}

void ABaseItem::OnHolsterComplete()
{
}

void ABaseItem::OnDrawComplete()
{
	OnItemDrawComplete.Broadcast(this);
}

bool ABaseItem::PlayHolster()
{
	AReadyOrNotCharacter* pc = GetOwnerCharacter();
	if (!pc)
		return false;
	
	if (IsLocallyControlled())
	{
		APlayerController* controller = Cast<APlayerController>(pc->GetController());
		if (controller)
		{
			controller->ClientStartCameraShake(HolsterCameraShake);
		}
	}
	
	if (SoundData)
	{
		PlayFMODAudio(SoundData->Holster);
	}

	if (AnimationData)
	{
		const FWeaponAnim& Anim = pc->IsCrouching() ? AnimationData->Crouch_Holster : AnimationData->Holster;
		PlayItemAnimation(Anim, true, true);

		if (IsLocallyControlled())
		{
			if (AnimationData->Holster.Body_FP)
			{
				UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_HolsterComplete, this, &ABaseItem::OnHolsterComplete, Anim.Body_FP->GetPlayLength());
				return true;
			}
		}
		else
		{
			if (AnimationData->Holster.Body_TP)
			{
				UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_HolsterComplete, this, &ABaseItem::OnHolsterComplete, Anim.Body_TP->GetPlayLength());
				return true;
			}
		}
	}

	return true;
}

void ABaseItem::PlayTPMontage(UAnimMontage* NewMontage, float PlayRate)
{
	if (!GetItemMesh()->GetAnimInstance())
		return;

	Server_PlayTPMontage(NewMontage, PlayRate);
}

void ABaseItem::PlayLocalTPMontage(UAnimMontage* NewMontage, float PlayRate)
{
	if (!GetItemMesh()->GetAnimInstance())
		return;

	if (IsLocallyControlled())
		return;

	GetItemMesh()->GetAnimInstance()->Montage_Play(NewMontage, PlayRate);
}

void ABaseItem::PlayItemAnimation(const FWeaponAnim& InWeaponAnim, const bool bRestartIfAlreadyPlaying, const bool bOnlyLocal, const bool bOnlyTP)
{
	if (AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter())
	{
		// Play all 1P and 3P anims if owner is a human player, otherwise play only 3P anims for everyone else
		if (APlayerCharacter* OwnerPlayerCharacter = GetOwnerPlayerCharacter())
		{
			// Play first-person body animation
			if (!bOnlyTP && (bRestartIfAlreadyPlaying || !OwnerPlayerCharacter->Is1PMontagePlaying(InWeaponAnim.Body_FP)))
			{
				if (bOnlyLocal)
					OwnerPlayerCharacter->PlayLocal1PMontage(InWeaponAnim.Body_FP);
				else
					OwnerPlayerCharacter->Play1PMontage(InWeaponAnim.Body_FP);
			}
			
			// Play first-person item animation
			if (!bOnlyTP && (bRestartIfAlreadyPlaying || !IsFPMontagePlaying(InWeaponAnim.Gun_FP)))
			{
				if (bOnlyLocal)
					PlayLocalFPMontage(InWeaponAnim.Gun_FP);
				else
					PlayFPMontage(InWeaponAnim.Gun_FP);
			}
		}

		// Play third-person body animation
		if (bRestartIfAlreadyPlaying || !OwnerCharacter->Is3PMontagePlaying(InWeaponAnim.Body_TP))
		{
			if (bOnlyLocal)
				OwnerCharacter->PlayLocal3PMontage(InWeaponAnim.Body_TP);
			else
				OwnerCharacter->Play3PMontage(InWeaponAnim.Body_TP);
		}

		// Play third-person item animation
		if (bRestartIfAlreadyPlaying || !IsTPMontagePlaying(InWeaponAnim.Gun_TP))
		{
			if (bOnlyLocal)
				PlayLocalTPMontage(InWeaponAnim.Gun_TP);
			else
				PlayTPMontage(InWeaponAnim.Gun_TP);
		}
	}
}

void ABaseItem::StopItemAnimation(const FWeaponAnim& InWeaponAnim, const bool bOnlyTP)
{
	if (AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter())
	{
		// Stop all 1P and 3P anims if owner is a human player, otherwise stop only 3P anims for everyone else
		if (APlayerCharacter* OwnerPlayerCharacter = GetOwnerPlayerCharacter())
		{
			// Stop first-person body animation
			if (!bOnlyTP && OwnerPlayerCharacter->Is1PMontagePlaying(InWeaponAnim.Body_FP))
			{
				OwnerPlayerCharacter->StopFPAnimMontage(InWeaponAnim.Body_FP);
			}
			
			// Stop first-person item animation
			if (!bOnlyTP && IsFPMontagePlaying(InWeaponAnim.Gun_FP))
			{
				StopFPMontage(InWeaponAnim.Gun_FP);
			}
		}

		// Stop third-person body animation
		if (OwnerCharacter->Is3PMontagePlaying(InWeaponAnim.Body_TP))
		{
			OwnerCharacter->StopTPMontage(InWeaponAnim.Body_TP);
		}

		// Stop third-person item animation
		if (IsTPMontagePlaying(InWeaponAnim.Gun_TP))
		{
			StopTPMontage(InWeaponAnim.Gun_TP);
		}
	}
}

bool ABaseItem::IsItemAnimationPlaying(const FWeaponAnim& InWeaponAnim, const bool bOnlyFP, const bool bOnlyBody) const
{
	if (const AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter())
	{
		// Check FP anims if we are player characters
		if (const APlayerCharacter* OwnerPlayerCharacter = GetOwnerPlayerCharacter())
		{
			if (bOnlyFP)
			{
				if (bOnlyBody)
					return OwnerPlayerCharacter->Is1PMontagePlaying(InWeaponAnim.Body_FP);

				return OwnerPlayerCharacter->Is1PMontagePlaying(InWeaponAnim.Body_FP) || IsFPMontagePlaying(InWeaponAnim.Gun_FP);
			}

			if (bOnlyBody)
				return OwnerPlayerCharacter->Is1PMontagePlaying(InWeaponAnim.Body_FP) || (InWeaponAnim.Body_TP && OwnerPlayerCharacter->Is3PMontagePlaying(InWeaponAnim.Body_TP));

			return  OwnerPlayerCharacter->Is1PMontagePlaying(InWeaponAnim.Body_FP) ||
					(InWeaponAnim.Body_TP && OwnerPlayerCharacter->Is3PMontagePlaying(InWeaponAnim.Body_TP)) ||
					IsFPMontagePlaying(InWeaponAnim.Gun_FP) ||
					(InWeaponAnim.Gun_TP && IsTPMontagePlaying(InWeaponAnim.Gun_TP));
		}

		// If non-player character, just check TP anims
		if (bOnlyBody)
			return InWeaponAnim.Body_TP && OwnerCharacter->Is3PMontagePlaying(InWeaponAnim.Body_TP);
		
		return  (OwnerCharacter->Is3PMontagePlaying(InWeaponAnim.Body_TP) && InWeaponAnim.Body_TP) || (IsTPMontagePlaying(InWeaponAnim.Gun_TP) && InWeaponAnim.Gun_TP);
	}

	return false;
}

bool ABaseItem::IsFPMontagePlaying(const UAnimMontage* InMontage) const
{
	if (GetItemMesh() && !GetItemMesh()->GetAnimInstance())
		return false;

	return GetItemMesh()->GetAnimInstance()->Montage_IsPlaying(InMontage);
}

bool ABaseItem::IsTPMontagePlaying(const UAnimMontage* InMontage) const
{
	if (GetItemMesh() && !GetItemMesh()->GetAnimInstance())
		return false;

	return GetItemMesh()->GetAnimInstance()->Montage_IsPlaying(InMontage);
}

void ABaseItem::Multicast_PlayTPMontage_Implementation(UAnimMontage* NewMontage, const float PlayRate)
{
	if (!GetItemMesh()->GetAnimInstance())
		return;

	if (IsLocallyControlled())
		return;

	#if WITH_EDITOR
	if (CHECK_DEBUG_SUBSYSTEM)
	{
		if (DEBUG_SUBSYSTEM->bLogPlayerAnimationStatus)
		{
			if (NewMontage)
				V_LOGM(LogReadyOrNot, "Playing %s on %s", *NewMontage->GetName(), *GetName());
		}
	}
	#endif

	GetItemMesh()->GetAnimInstance()->Montage_Play(NewMontage, PlayRate);
}

void ABaseItem::Server_PlayTPMontage_Implementation(UAnimMontage* NewMontage, float PlayRate)
{
	if (!GetItemMesh()->GetAnimInstance())
		return;

	Multicast_PlayTPMontage(NewMontage, PlayRate);
}

AReadyOrNotCharacter* ABaseItem::GetOwnerCharacter() const
{
	if (IsValid(GetOwner()))
	{
		return Cast<AReadyOrNotCharacter>(GetOwner());
	}
	return nullptr;
}

APlayerCharacter* ABaseItem::GetOwnerPlayerCharacter() const
{
	if (IsValid(GetOwner()))
	{
		return Cast<APlayerCharacter>(GetOwner());
	}
	return nullptr;
}

APlayerController* ABaseItem::GetOwningPlayerController() const
{
	if (const AReadyOrNotCharacter* Player = Cast<AReadyOrNotCharacter>(GetOwner()))
	{
		return Player->GetController<APlayerController>();
	}
	
	return nullptr;
}

ACyberneticController* ABaseItem::GetOwningAIController() const
{
	if (const AReadyOrNotCharacter* AI = Cast<AReadyOrNotCharacter>(GetOwner()))
	{
		return AI->GetController<ACyberneticController>();
	}
	
	return nullptr;
}

ACyberneticCharacter* ABaseItem::GetOwningAICharacter() const
{
	return Cast<ACyberneticCharacter>(GetOwner());
}

bool ABaseItem::IsEquipped() const
{
	if (const AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter())
	{
		return OwnerCharacter->GetEquippedItem() == this || OwnerCharacter->GetInventoryComponent()->IsEquippedWithShield(this);
	}
	
	return false;
}

bool ABaseItem::IsBlockingAnimationPlaying(const TArray<EBlockingAnimationExclusion> Exclusions) const
{
	if (!AnimationData)
		return false;

	if (GetWorld()->GetTimerManager().IsTimerActive(TH_DrawComplete))
		return true;

	if (IsItemAnimationPlaying(AnimationData->MeleeHit, true, true))
		return true;

	if (IsItemAnimationPlaying(AnimationData->Holster, true, true) || IsItemAnimationPlaying(AnimationData->Crouch_Holster, true, true))
	{
		if (!Exclusions.Contains(EBlockingAnimationExclusion::BAE_Holster))
		{
			#if WITH_EDITOR
			if (CHECK_DEBUG_SUBSYSTEM && DEBUG_SUBSYSTEM->bLogPlayerAnimationStatus)
			{
				ULog::Warning("Unable to play animation: Holster animation is playing on 1P mesh");
			}
			#endif
			
			return true;
		}
	}

	if (IsItemAnimationPlaying(AnimationData->Draw, true, true) || IsItemAnimationPlaying(AnimationData->Crouch_Draw, true, true) || IsItemAnimationPlaying(AnimationData->DrawFirst, true, true) || IsItemAnimationPlaying(AnimationData->Crouch_DrawFirst, true, true))
	{
		if (!Exclusions.Contains(EBlockingAnimationExclusion::BAE_Draw))
		{
			#if WITH_EDITOR
			if (CHECK_DEBUG_SUBSYSTEM && DEBUG_SUBSYSTEM->bLogPlayerAnimationStatus)
			{
				ULog::Warning("Unable to play animation: Draw animation is playing on 1P mesh");
			}
			#endif
			
			return true;
		}
	}

	if (IsItemAnimationPlaying(AnimationData->EnableNVG, true, true) || IsItemAnimationPlaying(AnimationData->DisableNVG, true, true))
	{
		#if WITH_EDITOR
		if (CHECK_DEBUG_SUBSYSTEM && DEBUG_SUBSYSTEM->bLogPlayerAnimationStatus)
		{
			ULog::Warning("Unable to play animation: NVG animation is playing on 1P mesh");
		}
		#endif

		return true;
	}

	if (IsItemAnimationPlaying(AnimationData->MeleeMiss, true, true))
	{
		#if WITH_EDITOR
		if (CHECK_DEBUG_SUBSYSTEM && DEBUG_SUBSYSTEM->bLogPlayerAnimationStatus)
		{
			ULog::Warning("Unable to play animation: MeleeMiss animation is playing on 1P mesh");
		}
		#endif
		
		return true;
	}

	if (IsButtonPushAnimationPlaying())
	{
		return true;
	}
	
	return false;
}

bool ABaseItem::IsLocallyControlled()
{
	if (const APlayerCharacter* Player = GetOwnerPlayerCharacter())
	{
		return Player->IsLocalPlayer();
	}

	return false;
}

void ABaseItem::MarkAsEvidence(const bool bMarkAsEvidence)
{
	if (bDeployable)
		return;

	if (bIsEvidence == bMarkAsEvidence)
		return;

	bIsEvidence = bMarkAsEvidence;

	if (bMarkAsEvidence)
	{
		V_LOGM(LogReadyOrNot, "Evidence Dropped %s", *GetName())
		
		if (!GetOwnerCharacter() || (GetOwnerCharacter() && !GetOwnerCharacter()->IsOnSWATTeam()))
			ScoringComponent->AddToScorePool();
		
		if (InteractableComponent)
		{
			InteractableComponent->RequiredLookAtPercentage = 0.98f;
            InteractableComponent->bImprintIconOnHUDUponInteraction = false;
            InteractableComponent->bHideUponPlayerMovement = false;
            InteractableComponent->bShowIconWhenActionsLocked = true;
            InteractableComponent->ActionSlot1.bUseCustomDisallowedActionText = true;
            InteractableComponent->ActionSlot1.DisallowedItemActionText = FText::FromStringTable("ActionPromptTable", "SecureEvidence");
            InteractableComponent->ActionSlot1.bCheckForDisallowedItems = true;
		}
	}
	else
	{
		if (InteractableComponent)
		{
			InteractableComponent->RequiredLookAtPercentage = 0.9f;
            InteractableComponent->bImprintIconOnHUDUponInteraction = true;
            InteractableComponent->bHideUponPlayerMovement = false;
			InteractableComponent->bShowIconWhenActionsLocked = false;
            InteractableComponent->ActionSlot1.bUseCustomDisallowedActionText = false;
            InteractableComponent->ActionSlot1.bCheckForDisallowedItems = false;
		}

		//FinalDropLocation = FVector::ZeroVector;

		/*
		if (ThrownEvidenceActor)
		{
			ThrownEvidenceActor->Destroy(true);
			ThrownEvidenceActor = nullptr;
		}
		*/
	}
}

/*
FVector ABaseItem::ProjectDropLocationToNavmesh()
{
	if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation NavLocation(FVector(ItemMesh->GetComponentLocation().X, ItemMesh->GetComponentLocation().Y, FinalDropLocation.Z));
		if (NavSys->ProjectPointToNavigation(NavLocation.Location, NavLocation, FVector(200.0f, 200.0f, 200.0f)))
		{
			DrawDebugPoint(GetWorld(), NavLocation.Location, 10.0f, FColor::White, false, 1.0f, 0);
			return NavLocation.Location;
		}
	}
	
	return FVector::ZeroVector;
}
*/

void ABaseItem::StartEvidenceCollection_COOP(AReadyOrNotCharacter* Collector)
{
	if (!Collector)
		return;
	
	if (bIsBeingCollected)
		return;
	
	CurrentCollectionTime = 0.0f;
	MaxCollectionTime = Collector->GetEvidenceCollectionTime();

	CollectingCharacter = Collector;
	Collector->BeginEvidenceCollection_COOP(this, InteractableComponent, MaxCollectionTime);
	
	bIsBeingCollected = true;
}

void ABaseItem::StopEvidenceCollection_COOP()
{
	if (!bIsBeingCollected)
		return;
	
	CurrentCollectionTime = 0.0f;
	bIsBeingCollected = false;

	if (CollectingCharacter)
	{
		CollectingCharacter->EndEvidenceCollection_COOP(InteractableComponent);
		
		CollectingCharacter = nullptr;
	}
}

void ABaseItem::CompleteEvidenceCollection_COOP(AReadyOrNotCharacter* Collector)
{
	bIsBeingCollected = false;

	if (Collector)
	{
        Collector->PickupEvidence(this);
	}
	
	StopEvidenceCollection_COOP();
}

void ABaseItem::OnRep_IsDropping()
{
	if (bDropping)
	{
		V_LOGM(LogReadyOrNot, "Dropping True %s", *GetName());

		ItemMesh->SetCollisionProfileName("Item");
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::Type::QueryAndPhysics);		
		ItemMesh->SetSimulatePhysics(true);
		ItemMesh->IgnoreActorWhenMoving(GetOwner(), true);
		ItemMesh->SetVisibility(true);
		
	} else
	{
		V_LOGM(LogReadyOrNot, "Dropping False %s", *GetName());
	}
}

void ABaseItem::OnRep_AttachmentRep()
{
	return;
	/*if (bNoAttachmentRep && !Cast<ABaseArmour>(this))
		return;

	//V_LOGM(LogReadyOrNotAI, "Item is being equipped / unequipped. Item: %s", *ItemName);

	if (!GetOwner())
	{
		return;
	}
	
	if (bDropping)
	{
		return;
	}

	APlayerCharacter* Char = Cast<APlayerCharacter>(GetOwner());
	if (!Char)
		return;

	if (Char->GetEquippedItem() == this && (Char->IsArrested() || Char->IsBeingArrested()))
	{
		ItemMesh->SetSkeletalMesh(nullptr);
		return;
	}

	AttachToActor(GetOwner(), FAttachmentTransformRules::SnapToTargetIncludingScale);

	bool bEquippedWithShield = false;
	ABallisticsShield* bShield = Cast<ABallisticsShield>(Char->GetEquippedItem());
	if (bShield)
	{
		bEquippedWithShield = bShield->PistolEquippedWithShield == this;
	}

	if (Cast<ABaseArmour>(this))
	{
		if (!IsLocallyControlled())
		{
			// if follow master pose tp is true then this is probably armor and we don't use the tp mesh we use the mesh gear slot
			ItemMesh->SetSkeletalMesh(bFollowMasterPoseTP ? nullptr : GetAppropriateSkeletalMesh());
			ItemMesh->EmptyOverrideMaterials();
			ItemMesh->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), BodySocket);
			ItemMesh->SetVisibility(true);
		}
		else
		{
			ItemMesh->SetSkeletalMesh(nullptr);
			return;
		}
	}
	else
	{
		if (IsLocallyControlled())
		{	
			if (ItemMesh)
			{
				//V_LOGM(LogReadyOrNot, "%s is attaching the first person item mesh to %s.", *ItemName, *HandsSocket.ToString());
				if (ItemMesh->GetAttachSocketName() != HandsSocket || ItemMesh->GetAttachParent() != (IsLocallyControlled() ? Char->GetMesh1P() : Char->GetMesh()))
				{
					ItemMesh->AttachToComponent(Char->GetMesh1P(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), HandsSocket);
				}
				ItemMesh->SetCastShadow(false);
				ItemMesh->bCastHiddenShadow = false;
			}
			
			if (Char->GetEquippedItem() == this || bEquippedWithShield)
			{
				if (GetAppropriateSkeletalMesh())
				{
					ItemMesh->SetSkeletalMesh(GetAppropriateSkeletalMesh());
					
				}
			}
			else if (!bDeployable)
			{
				ItemMesh->SetSkeletalMesh(nullptr);
				ItemMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			}
		}
		else
		{
			// stops seein gun through walls etc..
			if (bEnabledWeaponFovShader)
			{
				DisableWeaponFovShader();
			}
			
			ItemMesh->SetCastShadow(true);
			if (Char->GetEquippedItem() == this || bEquippedWithShield)
			{
				ItemMesh->SetSkeletalMesh(GetAppropriateSkeletalMesh());
				ItemMesh->EmptyOverrideMaterials();
				ItemMesh->SetHiddenInGame(false);
				ItemMesh->SetVisibility(true);
				ItemMesh->SetOnlyOwnerSee(false);
				ItemMesh->SetRenderStatic(false);
				ItemMesh->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), HandsSocket);
			}
			else if (!Cast<ABaseArmour>(this) && !bDeployable)
			{
				if (Char->GetMesh()->DoesSocketExist(BodySocket) &&
					(ItemCategories.Contains(EItemCategory::IC_LongTactical)
					|| ItemCategories.Contains(EItemCategory::IC_Primary)
						|| ItemCategories.Contains(EItemCategory::IC_Secondary)))
				{
					ItemMesh->AttachToComponent(Char->GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), BodySocket);
					if (!ItemCategories.Contains(EItemCategory::IC_Shield))
					{
						ItemMesh->SetRenderStatic(true);
					}
				}
				else
				{
					ItemMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
					ItemMesh->SetSkeletalMesh(nullptr);
				}
			}
		}
	}

	if (ItemMesh->SkeletalMesh == nullptr)
	{
		if (IsAttachedTo(Char))
		{
			DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
		ItemMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
	else
	{
		if (!IsAttachedTo(Char))
		{
			AttachToActor(Char, FAttachmentTransformRules::SnapToTargetIncludingScale);
		}
	}*/
}

bool ABaseItem::ShouldEnableWeaponFovShader() const
{
	return !bEnabledWeaponFovShader;
}

bool ABaseItem::ShouldAttachToOwner() const
{
	return true;
}

void ABaseItem::EnableWeaponFovShader()
{
	if (!IsLocallyControlled())
		return;

	bEnabledWeaponFovShader = true;
	DesiredDynamicWeaponFoVBlendEffectAmount = 1.0f;
	DynamicWeaponFovMats.Reset();
	ItemMesh->EmptyOverrideMaterials();
	
	if (USkeletalMesh* AppropriateMesh = GetAppropriateSkeletalMesh())
	{
		if (const FMeshFOVMaterials* FoundFOVMats = SkeletalMeshToFOVMats.Find(AppropriateMesh))
		{
			DynamicWeaponFovMats = FoundFOVMats->FovMats;
			uint32 i = 0;
			for (UMaterialInstanceDynamic* DynMat : DynamicWeaponFovMats)
			{
				ItemMesh->SetMaterial(i, DynMat);
				DynMat->SetScalarParameterValue("DisableWeaponFov", 0);
				i++;
			}
		}
		// failsafe
		else
		{
			for (int32 i = 0; i < AppropriateMesh->GetMaterials().Num(); i++)
			{
				UMaterialInstanceDynamic* DynMat = ItemMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(i, AppropriateMesh->GetMaterials()[i].MaterialInterface);
				if (DynMat)
				{
					DynMat->SetScalarParameterValue("DisableWeaponFov", 0);
					
					SkeletalMeshToFOVMats.Add(AppropriateMesh).FovMats.AddUnique(DynMat);
					DynamicWeaponFovMats.AddUnique(DynMat);
				}
			}
		}
	}
	
	TInlineComponentArray<USkinComponent*> SkinComponents;
	GetComponents(SkinComponents);

	{
		TInlineComponentArray<UStaticMeshComponent*> StaticMeshComponents;
		GetComponents(StaticMeshComponents);
		
		for (int32 i = 0; i < StaticMeshComponents.Num(); i++)
		{
			UStaticMesh* StaticMeshToUse = StaticMeshComponents[i]->GetStaticMesh();
			for (USkinComponent* skin : SkinComponents)
			{
				if (UStaticMesh** FoundStaticMesh = skin->StaticMeshSkinMap.Find(StaticMeshComponents[i]->GetStaticMesh()))
				{
					StaticMeshToUse = *FoundStaticMesh;
				}
			}
			
			StaticMeshComponents[i]->SetStaticMesh(StaticMeshToUse);
			
			if (const FMeshFOVMaterials* FoundFOVMats = StaticMeshToFOVMats.Find(StaticMeshToUse))
			{
				DynamicWeaponFovMats += FoundFOVMats->FovMats;
				
				uint32 y = 0;
				for (UMaterialInstanceDynamic* DynMat : FoundFOVMats->FovMats)
				{
					StaticMeshComponents[i]->SetMaterial(y, DynMat);
					DynMat->SetScalarParameterValue("DisableWeaponFov", 0);
					y++;
				}
			}
			// failsafe
			else
			{
				for (int32 y = 0; y < StaticMeshComponents[i]->GetMaterials().Num(); y++)
				{
					UMaterialInstanceDynamic* DynMat = StaticMeshComponents[i]->CreateAndSetMaterialInstanceDynamicFromMaterial(y, StaticMeshToUse->GetMaterial(y));
					if (DynMat)
					{
						DynMat->SetScalarParameterValue("DisableWeaponFov", 0);
						
						StaticMeshToFOVMats.Add(StaticMeshToUse).FovMats.AddUnique(DynMat);
						DynamicWeaponFovMats.AddUnique(DynMat);
					}
				}
			}
		}
	}

	{
		TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshComponents;
		GetComponents(SkeletalMeshComponents);
		SkeletalMeshComponents.Remove(ItemMesh);

		for (int32 i = 0; i < SkeletalMeshComponents.Num(); i++)
		{
			if (Cast<UWeaponAttachment>(SkeletalMeshComponents[i]))
				continue;
			
			USkeletalMesh* SkeletalMeshToUse = SkeletalMeshComponents[i]->SkeletalMesh;
			for (USkinComponent* skin : SkinComponents)
			{
				if (USkeletalMesh** FoundSkMesh = skin->SkeletalMeshSkinMap.Find(SkeletalMeshComponents[i]->SkeletalMesh))
				{
					SkeletalMeshToUse = *FoundSkMesh;
				}
			}

			SkeletalMeshComponents[i]->SetSkeletalMesh(SkeletalMeshToUse);

			if (const FMeshFOVMaterials* FoundFOVMats = SkeletalMeshToFOVMats.Find(SkeletalMeshToUse))
			{
				DynamicWeaponFovMats += FoundFOVMats->FovMats;
				
				uint32 y = 0;
				for (UMaterialInstanceDynamic* DynMat : FoundFOVMats->FovMats)
				{
					SkeletalMeshComponents[i]->SetMaterial(y, DynMat);
					DynMat->SetScalarParameterValue("DisableWeaponFov", 0);
					y++;
				}
			}
			// failsafe
			else
			{
				for (int32 y = 0; y < SkeletalMeshComponents[i]->GetMaterials().Num(); y++)
				{
					UMaterialInstanceDynamic* DynMat = SkeletalMeshComponents[i]->CreateAndSetMaterialInstanceDynamicFromMaterial(y, SkeletalMeshToUse->GetMaterials()[y].MaterialInterface);
					if (DynMat)
					{
						DynMat->SetScalarParameterValue("DisableWeaponFov", 0);
						
						SkeletalMeshToFOVMats.Add(SkeletalMeshToUse).FovMats.AddUnique(DynMat);
						DynamicWeaponFovMats.AddUnique(DynMat);
					}
				}
			}
		}
	}
}

void ABaseItem::TickWeaponFovShader(float DeltaTime)
{
	if (!GetOwnerCharacter())
		return;
	
	for (UMaterialInstanceDynamic* DynMat : DynamicWeaponFovMats)
	{
		if (DynMat)
		{
			DynMat->SetScalarParameterValue("DisableWeaponFov", 0);
			if (GetWorld() && GetOwnerCharacter())
			{
				AReadyOrNotLevelScript* Ls = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor());
				// wet level is inverted
				float CurrentWetLevel = 0.0f;
				bool bOutSideInRain = (GetOwnerCharacter()->IsOutside() && Ls && Ls->bRaining);
				DynMat->GetScalarParameterValue(FHashedMaterialParameterInfo("WetLevel"), CurrentWetLevel);
				DynMat->SetScalarParameterValue("WetLevel", UKismetMathLibrary::FInterpTo_Constant(CurrentWetLevel,   bOutSideInRain ? 0.0f : 1.0f, DeltaTime, 1.0f));
			}
		}
	}
}

void ABaseItem::DisableWeaponFovShader()
{
	if (!bEnabledWeaponFovShader)
		return;

	for (UMaterialInstanceDynamic* DynMat : DynamicWeaponFovMats)
	{
		if (DynMat)
		{
			DynMat->SetScalarParameterValue("DisableWeaponFov", 1);
		}
	}
	
	DynamicWeaponFovMats.Reset();
	bEnabledWeaponFovShader = false;
}

void ABaseItem::BlendOutWeaponFovShader()
{
	EnableWeaponFovShader();
	DesiredDynamicWeaponFoVBlendEffectAmount = 0.0f;
}

void ABaseItem::BlendInWeaponFovShader()
{
	EnableWeaponFovShader();
	DesiredDynamicWeaponFoVBlendEffectAmount = 1.0f;
}

void ABaseItem::InitializeFOVMaterials()
{
	{
		if (USkeletalMesh* DefaultSkMesh = GetClass()->GetDefaultObject<ABaseItem>()->GetItemMesh()->SkeletalMesh)
		{
			for (int32 i = 0; i < DefaultSkMesh->GetMaterials().Num(); i++)
			{
				UMaterialInterface* MaterialInstance = DefaultSkMesh->GetMaterials().IsValidIndex(i) ? DefaultSkMesh->GetMaterials()[i].MaterialInterface : nullptr;
				UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(MaterialInstance);

				if (MaterialInstance && !MID)
				{
					// Create and set the dynamic material instance.
					MID = UMaterialInstanceDynamic::Create(MaterialInstance, this);
				}

				if (MID)
				{
					MID->SetScalarParameterValue("DisableWeaponFov", 0);
					SkeletalMeshToFOVMats.FindOrAdd(DefaultSkMesh).FovMats.AddUnique(MID);
				}
			}
		}
	}

	{
		USkeletalMesh* AppropriateMesh = GetAppropriateSkeletalMesh();
		if (AppropriateMesh && !SkeletalMeshToFOVMats.Find(AppropriateMesh))
		{
			for (int32 i = 0; i < AppropriateMesh->GetMaterials().Num(); i++)
			{
				UMaterialInterface* MaterialInstance = AppropriateMesh->GetMaterials().IsValidIndex(i) ? AppropriateMesh->GetMaterials()[i].MaterialInterface : nullptr;
				UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(MaterialInstance);

				if (MaterialInstance && !MID)
				{
					// Create and set the dynamic material instance.
					MID = UMaterialInstanceDynamic::Create(MaterialInstance, this);
				}

				if (MID)
				{
					MID->SetScalarParameterValue("DisableWeaponFov", 0);
					SkeletalMeshToFOVMats.FindOrAdd(AppropriateMesh).FovMats.AddUnique(MID);
				}
			}
		}
	}
	
	TInlineComponentArray<USkinComponent*> SkinComponents;
	GetComponents(SkinComponents);

	{
		TInlineComponentArray<UStaticMeshComponent*> StaticMeshComponents;
		GetComponents(StaticMeshComponents);
		
		for (int32 i = 0; i < StaticMeshComponents.Num(); i++)
		{
			UStaticMesh* StaticMeshToUse = StaticMeshComponents[i]->GetStaticMesh();
			for (USkinComponent* skin : SkinComponents)
			{
				if (skin)
				{
					if (UStaticMesh** FoundStaticMesh = skin->StaticMeshSkinMap.Find(StaticMeshComponents[i]->GetStaticMesh()))
					{
						StaticMeshToUse = *FoundStaticMesh;
					}
				}
			}

			if (StaticMeshToFOVMats.Find(StaticMeshToUse))
				continue;
			
			for (int32 y = 0; y < StaticMeshComponents[i]->GetMaterials().Num(); y++)
			{
				UMaterialInstanceDynamic* DynMat = StaticMeshComponents[i]->CreateAndSetMaterialInstanceDynamicFromMaterial(y, StaticMeshToUse->GetMaterial(y));
				if (DynMat)
				{
					DynMat->SetScalarParameterValue("DisableWeaponFov", 0);

					StaticMeshToFOVMats.FindOrAdd(StaticMeshToUse).FovMats.AddUnique(DynMat);
				}
			}
		}
	}

	{
		TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshComponents;
		GetComponents(SkeletalMeshComponents);
		
		SkeletalMeshComponents.Remove(ItemMesh);
		
		for (int32 i = 0; i < SkeletalMeshComponents.Num(); i++)
		{
			if (Cast<UWeaponAttachment>(SkeletalMeshComponents[i]))
				continue;
			
			USkeletalMesh* SkeletalMeshToUse = SkeletalMeshComponents[i]->SkeletalMesh;
			for (USkinComponent* skin : SkinComponents)
			{
				if (skin)
				{
					if (USkeletalMesh** FoundSkMesh = skin->SkeletalMeshSkinMap.Find(SkeletalMeshComponents[i]->SkeletalMesh))
					{
						SkeletalMeshToUse = *FoundSkMesh;
					}
				}
			}

			if (SkeletalMeshToFOVMats.Find(SkeletalMeshToUse))
				continue;
			
			for (int32 y = 0; y < SkeletalMeshComponents[i]->GetMaterials().Num(); y++)
			{
				UMaterialInstanceDynamic* DynMat = SkeletalMeshComponents[i]->CreateAndSetMaterialInstanceDynamicFromMaterial(y, SkeletalMeshToUse->GetMaterials()[y].MaterialInterface);
				if (DynMat)
				{
					DynMat->SetScalarParameterValue("DisableWeaponFov", 0);
					
					SkeletalMeshToFOVMats.FindOrAdd(SkeletalMeshToUse).FovMats.AddUnique(DynMat);
				}
			}
		}
	}
}

void ABaseItem::DisableOrEnableAnimation()
{
	if (!IsEquipped() && bDisableAnimInstanceWhenNotEquipped)
	{
		GetItemMesh()->bNoSkeletonUpdate = true;
		GetItemMesh()->bPauseAnims = true;
	}
	else
	{
		GetItemMesh()->bNoSkeletonUpdate = false;
		GetItemMesh()->bPauseAnims = false;
	}
}

void ABaseItem::OnRep_AttachmentReplication()
{
	//Super::OnRep_AttachmentReplication();
}

// function to randomly select from motion array
/*
AnimList - The list of weapon motions
PartToPlay - The Component to play on, 0 = Gun_FP, 1 = Gun_TP, 2 = Body_FP, 3 = Body_TP
*/

UAnimMontage* ABaseItem::GetRandWeapAnimFromList(TArray<FWeaponAnim> AnimList, EAnimationType PartToPlay)
{
	UAnimMontage* EmptyResult = nullptr;

	if (AnimList.Num() > 0)
	{
		switch (PartToPlay)
		{
			case AT_Gun_FP: return AnimList[FMath::Rand() % AnimList.Num()].Gun_FP;
			case AT_Gun_TP: return AnimList[FMath::Rand() % AnimList.Num()].Gun_TP;
			case AT_Body_FP: return AnimList[FMath::Rand() % AnimList.Num()].Body_FP;
			case AT_Body_TP: return AnimList[FMath::Rand() % AnimList.Num()].Body_TP;
		}
	}

	// fall back safe
	return EmptyResult;
}

float ABaseItem::GetFreeAimLimit()
{
	float tempFreeAimLimit = FreeAimLimit;

	TArray<UWeaponAttachment*> weaponAttachments;
	GetComponents(weaponAttachments);
	for (int32 i = 0; i < weaponAttachments.Num(); i++)
	{
		UWeaponAttachment* attachment = Cast<UWeaponAttachment>(weaponAttachments[i]);
		if (attachment)
		{
			tempFreeAimLimit *= attachment->DeadzoneMultiplier;
		}
	}
	return tempFreeAimLimit;
}

float ABaseItem::GetFreeAimLimitADS()
{
	float tempFreeAimLimit = FreeAimLimitADS;

	TArray<UWeaponAttachment*> weaponAttachments;
	GetComponents(weaponAttachments);
	for (int32 i = 0; i < weaponAttachments.Num(); i++)
	{
		UWeaponAttachment* attachment = Cast<UWeaponAttachment>(weaponAttachments[i]);
		if (attachment)
		{
			tempFreeAimLimit *= attachment->DeadzoneMultiplier;
		}
	}
	return tempFreeAimLimit;
}


float ABaseItem::GetLazySpringStrength()
{
	return LazySpringStrength;
}

float ABaseItem::GetLazySpringStrengthADS()
{
	return LazySpringStrengthADS;
}

void ABaseItem::UpdateFOVShader(const float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_FOVShaderTick);

	if (bEnabledWeaponFovShader && CurrentDynamicWeaponFoVBlendEffectAmount != DesiredDynamicWeaponFoVBlendEffectAmount)
	{
		CurrentDynamicWeaponFoVBlendEffectAmount = UKismetMathLibrary::FInterpTo(CurrentDynamicWeaponFoVBlendEffectAmount, DesiredDynamicWeaponFoVBlendEffectAmount, DeltaTime, 1.0f);
		
		for (int32 i = 0; i < DynamicWeaponFovMats.Num(); i++)
		{
			if (UMaterialInstanceDynamic* DynMat = DynamicWeaponFovMats[i])
			{
				DynMat->SetScalarParameterValue("Blend", CurrentDynamicWeaponFoVBlendEffectAmount);
				DynMat->SetScalarParameterValue("DisableWeaponFov", 0);
			}
		}
	}
}

float ABaseItem::GetLowReadyRange()
{
	FVector MuzzleLoc = GetItemMesh()->GetBoneLocation("tag_muzzle");
	if (MuzzleLoc != FVector::ZeroVector)
	{
		LowReadyRange = (MuzzleLoc - GetItemMesh()->GetComponentLocation()).Size() + 5.0f;
	}

	TArray<UWeaponAttachment*> weaponAttachments;
	GetComponents(weaponAttachments);
	for (int32 i = 0; i < weaponAttachments.Num(); i++)
	{
		UWeaponAttachment* attachment = weaponAttachments[i];
		if (attachment)
		{
			FVector MuzzleAttachmentLoc = GetItemMesh()->GetSocketLocation(attachment->SocketAttachment);
			if (MuzzleAttachmentLoc != FVector::ZeroVector)
			{
				float AttachmentLowReadyRange = (MuzzleAttachmentLoc - GetItemMesh()->GetComponentLocation()).Size();
				if (AttachmentLowReadyRange > LowReadyRange)
				{
					LowReadyRange = AttachmentLowReadyRange + 10.0f;
				}
			}
			
		}
	}
	return LowReadyRange;
}

void ABaseItem::Server_SetMasterPoseComponent_Implementation(USkeletalMeshComponent* Mesh)
{
	MasterPoseRep = Mesh;
	OnRep_MasterPoseComponent();
}

void ABaseItem::OnRep_MasterPoseComponent()
{
	// important!!!
	//TP_ItemMesh->bUseBoundsFromMasterPoseComponent = true;
	//TP_ItemMesh->SetMasterPoseComponent(MasterPoseRep);

	// Alex HACK: somehow the item translation is messed up on lean, this is a temporary fix
	// we pass over the item to the mesh gear slot on the character.
	APlayerCharacter* pc = GetOwnerPlayerCharacter();
	if (pc)
	{
		if (IsLocallyControlled())
		{
			// if (!pc->GetMeshGearSlot()->SkeletalMesh)
			// {
			// 	pc->GetMeshGearSlot()->SetSkeletalMesh(ItemMesh->SkeletalMesh);
			// }
		
			ItemMesh->SetOnlyOwnerSee(true);
		}
	}
}

FPrimaryAssetId ABaseItem::GetPrimaryAssetId() const
{
	// Only considered a primary asset if it is meant to show up in the UI
	// if (CategoryFlags != 0)
	// {
	return FPrimaryAssetId("LoadoutItem", FPackageName::GetShortFName(GetPackage()->GetName()));
	// }
	
	return Super::GetPrimaryAssetId();
}

void ABaseItem::PlayFMODAudio(UFMODEvent* Event)
{
	if (!Event)
	{
		return;
	}
	
	if (IsLocallyControlled())
	{
		USoundSource* SoundSource = USoundSource::CreateFirstPersonSound(GetWorld(), Event, FTransform(ItemMesh->GetComponentLocation()), {}, false);
		if(SoundSource)
		{
			SoundSource->Attach(ItemMesh, "");
			SoundSource->Play();
		}
	}
	else
	{
		USoundSource* SoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), Event, FTransform(ItemMesh->GetComponentLocation()), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
		if(SoundSource)
		{
			SoundSource->Attach(ItemMesh, NAME_None);
			SoundSource->Play();
		}
	}
}

void ABaseItem::SetMeshUpdateRateParameters(FAnimUpdateRateParameters* AnimUpdateRateParams)
{
	AnimUpdateRateParams->BaseNonRenderedUpdateRate = 30.0f;
	AnimUpdateRateParams->BaseVisibleDistanceFactorThesholds.Empty();
	AnimUpdateRateParams->BaseVisibleDistanceFactorThesholds.Add(0.05f);
	AnimUpdateRateParams->BaseVisibleDistanceFactorThesholds.Add(0.025f);
	AnimUpdateRateParams->BaseVisibleDistanceFactorThesholds.Add(0.00f);
}

FVector ABaseItem::GetItemLocation() const
{
	return ItemMesh->GetComponentLocation();
}

FVector ABaseItem::GetItemRelativeLocation() const
{
	if (const AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter())
	{
		if (OwnerCharacter->IsLocalPlayer())
		{
			return ItemMesh->GetRelativeLocation();
		}
	}

	return ItemMesh->GetRelativeLocation();
}

#if WITH_EDITOR
void ABaseItem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == "LookupTableIdx" || PropertyChangedEvent.GetPropertyName() == "bApplyLookupData")
	{
		//ApplyLookupData(true);
	}
}
#endif

float ABaseItem::GetWeight()
{
	if (!IsDepleted())
	{
		return ItemWeight;
	}
	return 0.0f;
}

USkeletalMesh* ABaseItem::GetAppropriateSkeletalMesh()
{
	if (Skin && Skin->MeshOverride.LoadSynchronous())
	{
		return Skin->MeshOverride.LoadSynchronous();
	}
	
	if (Rep_CustomItemMeshFromAttachment)
	{
		return Rep_CustomItemMeshFromAttachment;
	}
	
	USkeletalMesh* DefaultSkMesh = GetClass()->GetDefaultObject<ABaseItem>()->GetItemMesh()->SkeletalMesh;
	
	TInlineComponentArray<USkinComponent*> SkinComps;
	GetComponents(SkinComps);
	for (USkinComponent* skin : SkinComps)
	{
		if (USkeletalMesh** FoundMesh = skin->SkeletalMeshSkinMap.Find(DefaultSkMesh))
		{
			return *FoundMesh;
		}
	}

	AReadyOrNotCharacter* OwnerChar = GetOwnerCharacter();
	if (OwnerChar)
	{
		bool bFoundMesh = false;
		USkeletalMesh* OverrideMeshTP = OwnerChar->GetTPMeshOverride(GetClass(), bFoundMesh);
		if (bFoundMesh)
		{
			return OverrideMeshTP;
		}
	}
	
	return DefaultSkMesh;
}

void ABaseItem::OnRep_Skin()
{
	if (!Skin || !Skin->CompatibleItemTags.Contains(CustomizationTag))
	{
		Skin = DefaultSkin.LoadSynchronous();
	}
	
	FSavedCustomization::ApplyItemCustomization(this, Skin);
}

void ABaseItem::Client_SetFPModelVisibility_Implementation(bool bVisibility)
{
	if (!IsLocallyControlled())
		return;
	
	bScriptedFPHidden = !bVisibility;
	GetItemMesh()->SetOwnerNoSee(!bVisibility);
	TArray<USceneComponent*> AttachedChildren = GetItemMesh()->GetAttachChildren();
	for (int32 i = 0; i < AttachedChildren.Num(); i++)
	{
		UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(AttachedChildren[i]);
		if (PrimComp)
		{
			PrimComp->SetOwnerNoSee(!bVisibility);
		}
	}
}

void ABaseItem::PlayWeaponCleaning()
{
	PlayItemAnimation(AnimationData->WeaponClearing);

	bHasBeenCleared = true;
	bNoPickup = true;
}

// ICanIssueCommandOn implementation
bool ABaseItem::CanIssueCommand_Implementation() const
{
	return IsEvidence();
}

AActor* ABaseItem::GetCommandActor_Implementation() const
{
	return const_cast<ABaseItem*>(this);
}

void ABaseItem::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (InteractInstigator)
	{
		if (bIsEvidence)
		{
			StartEvidenceCollection_COOP(InteractInstigator);
		}
		else
		{
			InteractInstigator->GetInventoryComponent()->AddInventoryItem(this);
		}
	}
}

void ABaseItem::EndInteract_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	StopEvidenceCollection_COOP();
}

void ABaseItem::OnFocusLost_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	//EndInteract_Implementation(InteractInstigator, InInteractableComponent);
}

UInteractableComponent* ABaseItem::GetInteractableComponent_Implementation() const
{
	return InteractableComponent;
}

FName ABaseItem::DetermineAnimatedIcon_Implementation() const
{
	if (GetOwner())
		return NAME_None;
		
	if (bIsEvidence)
	{
		if (bIsBeingCollected)
		{
			return "Empty";
		}
		
		if (bIsClearable && bHasBeenCleared)
		{
			return "Pickup Evidence";
		}

		return (bIsClearable ? "Clear Evidence" : "Pickup Evidence");
	}

	if (ContainsItemCategory(EItemCategory::IC_Primary))
		return *FString("Pickup Primary");

	if (ContainsItemCategory(EItemCategory::IC_Secondary))
		return *FString("Pickup Secondary");
	
	return *(FString("Pickup ") + ItemName.ToString());
}

FText ABaseItem::DetermineActionText_Implementation() const
{
	if (GetOwner())
		return FText::GetEmpty();
	
	if (bIsEvidence)
	{		
		if (bIsClearable && bHasBeenCleared)
		{
			return FText::Format(FText::FromStringTable("ActionPromptTable", "SecureEvidenceName"), ItemName);			
		}

		FString ActionPromptKey = bIsClearable ? "ClearEvidenceWithName" : "SecureEvidenceName";
		return FText::Format(FText::FromStringTable("ActionPromptTable", ActionPromptKey), ItemName);
	}

	return FText::Format(FText::FromStringTable("ActionPromptTable", "PickupWithName"), ItemName);
}

EInputEvent ABaseItem::DetermineInputEvent_Implementation() const
{
	if (GetOwner())
		return IE_Pressed;
	
	if (bIsEvidence)
	{
		return IE_Repeat;
	}
	
	return IE_Pressed;
}

bool ABaseItem::CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const
{
	if (Hit.GetActor())
	{
		if (const ADoor* Door = Cast<ADoor>(Hit.GetActor()))
		{
			if (IsEvidence() && (Door->IsDoorBroken() || !Door->IsAttachedToRoot() || Door->AllMajorDoorChunksDestroyed()))
				return true;
		}
			
		if (Hit.GetActor()->IsA(StaticClass()))
			return true;
		
		if (Hit.GetActor()->IsA(APawn::StaticClass()))
			return true;
		
		if (Hit.GetActor()->IsA(ACollectedEvidenceActor::StaticClass()))
			return true;
		
		if (Hit.GetActor()->IsA(AIncapacitatedHuman::StaticClass()))
			return true;
	}
	
	return false;
}

FSlateBrush ABaseItem::GetPingIcon_Implementation()
{
	FSlateBrush IconBrush;
	IconBrush.SetResourceObject(Visuals.ItemIcon);
	
	return IconBrush;
}

FText ABaseItem::GetPingText_Implementation()
{
	return DetermineActionText_Implementation();
}

FVector ABaseItem::GetPingLocation_Implementation()
{
	return GetItemLocation();
}

float ABaseItem::GetPingDuration_Implementation()
{
	return 10.0f;
}

bool ABaseItem::CanPing_Implementation()
{
	return true;
}

UScoringComponent* ABaseItem::GetScoringComponent_Implementation() const
{
	return ScoringComponent;
}

void ABaseItem::GatherDebugData_Implementation(TArray<FDebugData>& OutDebugData)
{
#if !UE_BUILD_SHIPPING
	OutDebugData.Empty(20);

	FNumberFormattingOptions FloatFormatOptions;
	FloatFormatOptions.MinimumFractionalDigits = 1;
	FloatFormatOptions.MaximumFractionalDigits = 1;
	
	const FString LocX = "X: " + FText::AsNumber(GetActorLocation().X, &FloatFormatOptions).ToString();
	const FString LocY = "Y: " + FText::AsNumber(GetActorLocation().Y, &FloatFormatOptions).ToString();
	const FString LocZ = "Z: " + FText::AsNumber(GetActorLocation().Z, &FloatFormatOptions).ToString();
	
	const FString RotX = "R: " + FText::AsNumber(GetActorRotation().Roll, &FloatFormatOptions).ToString();
	const FString RotY = "P: " + FText::AsNumber(GetActorRotation().Pitch, &FloatFormatOptions).ToString();
	const FString RotZ = "Y: " + FText::AsNumber(GetActorRotation().Yaw, &FloatFormatOptions).ToString();

	FString ItemCategory;
	for (const EItemCategory& Category : ItemCategories)
	{
		ItemCategory.Append(FString(ENUM_TO_STRING(EItemCategory, Category, false)) + ", ");
	}
	ItemCategory.ReplaceInline(TEXT(", "), TEXT(""));
	
	OutDebugData.Add(FDebugData("Name", FText::FromString(ItemName.ToString())));
	OutDebugData.Add(FDebugData("Item Categories", FText::FromString(ENUM_TO_STRING(EItemClass, ItemClass, false))));
	OutDebugData.Add(FDebugData("Item Class", FText::FromString(ItemCategory)));
	OutDebugData.Add(FDebugData("Location", FText::FromString("(" + LocX + " " + LocY + " " + LocZ + ")")));
	OutDebugData.Add(FDebugData("Rotation", FText::FromString("(" + RotX + " " + RotY + " " + RotZ + ")")));
	OutDebugData.Add(FDebugData("Is Evidence", FText::FromString(bIsEvidence ? "True" : "False")));
	OutDebugData.Add(FDebugData("Is Clearable", FText::FromString(bIsClearable ? "True" : "False")));
	OutDebugData.Add(FDebugData("Is Deployable", FText::FromString(bDeployable ? "True" : "False")));
	OutDebugData.Add(FDebugData("Has Been Cleared", FText::FromString(bHasBeenCleared ? "True" : "False")));
#endif
}

void ABaseItem::Secure_Implementation(AReadyOrNotCharacter* InInstigator)
{
	InInstigator->PickupEvidence(this);
}

bool ABaseItem::IsSecured_Implementation() const
{
	return !IsValid(this) || !bIsEvidence;
}

FVector ABaseItem::GetLocation_Implementation() const
{
	return GetItemLocation();
}

bool ABaseItem::CanBeSecured_Implementation() const
{
	if (bInInventory || GetOwnerCharacter())
		return false;
	
	return bIsEvidence;
}

bool ABaseItem::CanBeSecuredByTrailers_Implementation() const
{
	return CanBeSecured_Implementation();
}

bool ABaseItem::HasDoorPushAnimation() const
{
	if (!AnimationData)
	{
		return false;
	}

	return AnimationData->DoorPush.Body_FP ||
		AnimationData->DoorPush.Body_TP ||
		AnimationData->DoorPush.Gun_FP ||
		AnimationData->DoorPush.Gun_TP;
}

bool ABaseItem::IsDoorPushAnimationPlaying() const
{
	if (!HasDoorPushAnimation())
		return false;

	return IsItemAnimationPlaying(AnimationData->DoorPush, true, true);
}

bool ABaseItem::HasButtonPushAnimation() const
{
	if (!AnimationData)
	{
		return false;
	}

	return AnimationData->ButtonPush.Body_FP ||
		AnimationData->ButtonPush.Body_TP ||
		AnimationData->ButtonPush.Gun_FP ||
		AnimationData->ButtonPush.Gun_TP;
}

bool ABaseItem::IsButtonPushAnimationPlaying() const
{
	if (!HasButtonPushAnimation())
		return false;

	return IsItemAnimationPlaying(AnimationData->ButtonPush, true, true);
}

void ABaseItem::PlayDoorPushAnimation()
{
	if (!HasDoorPushAnimation())
		return;

	if (IsDoorPushAnimationPlaying())
		return;

	PlayItemAnimation(AnimationData->DoorPush, false);
}

void ABaseItem::PlayButtonPushAnimation()
{
	if (!HasButtonPushAnimation())
		return;
		
	if (IsButtonPushAnimationPlaying())
		return;

	PlayItemAnimation(AnimationData->ButtonPush, false);
}

void ABaseItem::StunTick_Implementation(EStunType StunType)
{
	if (!AnimationData)
	{
		return;
	}

	FWeaponAnim animation = AnimationData->ReactToSting;

	switch (StunType)
	{
		case EStunType::ST_Flash:
			if (AnimationData->ReactToFlash.Body_FP)
			{
				animation = AnimationData->ReactToFlash;
			}
			else if (AnimationData->ReactToPepperSpray.Body_FP)
			{
				animation = AnimationData->ReactToPepperSpray;
			}
			break;
		case EStunType::ST_Gassed:
			if (AnimationData->ReactToGas.Body_FP)
			{
				animation = AnimationData->ReactToGas;
			}
			break;
		case EStunType::ST_Tased:
			if (AnimationData->ReactToTaser.Body_FP)
			{
				animation = AnimationData->ReactToTaser;
			}
			break;
		case EStunType::ST_None:
			break;
		case EStunType::ST_Stung:
			break;
		default: ;
	}

	PlayItemAnimation(animation, false, true);
}

void ABaseItem::LastStunTick_Implementation(EStunType StunType)
{
	if (!AnimationData)
	{
		return;
	}

	FWeaponAnim animation = AnimationData->ReactToSting_End;


	switch (StunType)
	{
	case EStunType::ST_Flash:
		if (AnimationData->ReactToFlash_End.Body_FP)
		{
			animation = AnimationData->ReactToFlash_End;
		}
		else if (AnimationData->ReactToPepperSpray_End.Body_FP)
		{
			animation = AnimationData->ReactToPepperSpray_End;
		}
		break;
	case EStunType::ST_Gassed:
		if (AnimationData->ReactToGas_End.Body_FP)
		{
			animation = AnimationData->ReactToGas_End;
		}
		break;
	case EStunType::ST_Tased:
		if (AnimationData->ReactToTaser_End.Body_FP)
		{
			animation = AnimationData->ReactToTaser_End;
		}
		break;
	default: ;
	}

	PlayItemAnimation(animation, false);

	/*
	if (animation.Gun_FP)
	{
		PlayFPMontage(animation.Gun_FP);
	}
	*/

	if (animation.Body_FP)
	{
		//pcOwner->Play1PMontage(animation.Body_FP);

		// todo: delegate here
		
		if (const APlayerCharacter* OwnerPlayerCharacter = GetOwnerPlayerCharacter())
		{
			FOnMontageEnded CompleteDelegate;
			CompleteDelegate.BindUObject(this, &ABaseItem::StunnedAnimationEnd);
			OwnerPlayerCharacter->GetMesh1P()->AnimScriptInstance->Montage_SetBlendingOutDelegate(CompleteDelegate, animation.Body_FP);
		}
	}
	else
	{
		// if we have no motion instantly call back up again
		PlayDraw(!bDrawnBefore);
	}
}

void ABaseItem::StunnedAnimationEnd(UAnimMontage* animMontage, bool bInterrupted)
{
	// initially called in the player character but we need until montage end!
	PlayDraw(!bDrawnBefore);
}

FString ABaseItem::GetFriendlyName_Implementation()
{
	return ItemName.ToString();
}

UTexture2D* ABaseItem::GetFriendlyIcon_Implementation()
{
	if (Visuals.ItemIcon)
	{
		return Visuals.ItemIcon;
	}
	
	return nullptr;
}

bool ABaseItem::IsEvidence() const
{
	return bIsEvidence ? !(bIsClearable && bHasBeenCleared) : false;
}

bool ABaseItem::IsPlayingStunnedAnimation()
{
	if (!AnimationData)
		return false;

	return  IsItemAnimationPlaying(AnimationData->ReactToTaser) ||
			IsItemAnimationPlaying(AnimationData->ReactToSting) ||
			IsItemAnimationPlaying(AnimationData->ReactToPepperSpray) ||
			IsItemAnimationPlaying(AnimationData->ReactToGas) ||
			IsItemAnimationPlaying(AnimationData->ReactToFlash);
}

bool ABaseItem::IsPlayingStunnedEndAnimation()
{
	if (!AnimationData)
		return false;

	return  IsItemAnimationPlaying(AnimationData->ReactToTaser_End) ||
			IsItemAnimationPlaying(AnimationData->ReactToSting_End) ||
			IsItemAnimationPlaying(AnimationData->ReactToPepperSpray_End) ||
			IsItemAnimationPlaying(AnimationData->ReactToGas_End) ||
			IsItemAnimationPlaying(AnimationData->ReactToFlash_End);
}

void ABaseItem::OnMeshComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HitComponent->IsVisible())
	{
		return; // don't trigger this on invisible components (and therefore hear a double sound sometimes)
	}

	float NormalImpulseSize = NormalImpulse.Size();
	if (SoundData && NormalImpulseSize >= SoundData->PhysicsImpactMinimumVelocity)
	{
		auto sound = UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), SoundData->PhysicsImpact, GetActorTransform(), true);
		if (sound.Instance)
		{
			sound.Instance->setParameterByName("Material", UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()));
			sound.Instance->setParameterByName("Velocity", NormalImpulseSize);
		}
	}
}

void ABaseItem::DrawOutline()
{
	TArray<UActorComponent*> Comps;
	GetComponents(USkeletalMeshComponent::StaticClass(), Comps, false);
	for (UActorComponent* s : Comps)
	{
		if (USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(s))
		{
			Mesh->SetRenderCustomDepth(true);
			
		}
	}

	// Also Outline static mesh comps like magazine
	GetComponents(UStaticMeshComponent::StaticClass(), Comps, false);
	for (UActorComponent* s : Comps)
	{
		if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(s))
		{
			Mesh->SetRenderCustomDepth(true);
			
		}
	}
}

void ABaseItem::DisableOutline()
{
	TArray<UActorComponent*> Comps;
	GetComponents(USkeletalMeshComponent::StaticClass(), Comps, false);
	for (UActorComponent* s : Comps)
	{
		if (USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(s))
		{
			Mesh->SetRenderCustomDepth(false);
		}
	}

	GetComponents(UStaticMeshComponent::StaticClass(), Comps, false);
	for (UActorComponent* s : Comps)
	{
		if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(s))
		{
			Mesh->SetRenderCustomDepth(false);
			
		}
	}
}

void ABaseItem::OnThrownFromInventory(AReadyOrNotCharacter* Thrower, bool bMarkAsEvidence)
{
	// Already thrown, don't try to throw again
	if (!GetOwner())
		return;
		
	UReadyOrNotSignificanceManager::ForceActorRelevant(this);

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	GetItemMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	GetItemMesh()->SetSimulatePhysics(true);
	GetItemMesh()->SetCollisionProfileName("PhysicsItem");
	GetItemMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		
	SetActorEnableCollision(true);
	GetItemMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MarkAsEvidence(bMarkAsEvidence);
	SetActorTickEnabled(true);
		
	GetItemMesh()->SetNotifyRigidBodyCollision(true);
	GetItemMesh()->OnComponentHit.RemoveAll(this);
	GetItemMesh()->OnComponentHit.AddDynamic(this, &ABaseItem::OnPhysicsImpact);

	// Is weapon highlighting enabled?
	UReadyOrNotGameUserSettings* us = Cast<UReadyOrNotGameUserSettings>(GEngine->GetGameUserSettings());
	if (us && us->bHighlightWeapons && bMarkAsEvidence)
	{
		FTimerHandle Handle;
		FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &ABaseItem::DrawOutline);
		GetWorld()->GetTimerManager().SetTimer(Handle, Delegate, 2.0f, false);
	}

	if (bDeployable)
		InteractableComponent->DestroyComponent();
}

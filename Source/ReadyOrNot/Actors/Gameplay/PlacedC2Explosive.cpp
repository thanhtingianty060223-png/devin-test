// Void Interactive, 2021

#include "PlacedC2Explosive.h"

#include "ReadyOrNotAIConfig.h"
#include "ReadyOrNotPlayerCameraManager.h"

#include "Actors/Door.h"
#include "Actors/Items/Detonator.h"
#include "Characters/CyberneticController.h"
#include "Commander/RosterManager.h"

#include "Components/InteractableComponent.h"
#include "Components/MirrorPortalComponent.h"
#include "Components/MoraleComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"

APlacedC2Explosive::APlacedC2Explosive()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	RootComponent = MeshComp;
	
	ExplosionComp = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Explosion"));
	ExplosionComp->SetupAttachment(MeshComp, TEXT("ExplosionSocket"));
	ExplosionComp->bAutoActivate = false;

	PerceptionStimuliComp = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionComp"));

	C2InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Interactable Component"));
	C2InteractableComponent->ActionSlot1.Init("Use", IE_Repeat, FText::FromStringTable("ActionPromptTable", "RemoveC2"));
	C2InteractableComponent->ActionSlot1.bCheckForDisallowedItems = false;
	C2InteractableComponent->ActionSlot2.bUseCustomActionText = true;
	C2InteractableComponent->ActionSlot2.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "RemovingC2");
	C2InteractableComponent->ActionSlot2.bAnimate = true;
	C2InteractableComponent->ActionSlot2.bLoopAnimation = true;
	C2InteractableComponent->SetupAttachment(RootComponent);
	
	AudioComponent = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("FMOD Audio Component"));
	AudioComponent->bAutoActivate = false;
	AudioComponent->bAutoDestroy = false;
	AudioComponent->bStopWhenOwnerDestroyed = false;
	AudioComponent->SetupAttachment(MeshComp, TEXT("ExplosionSocket"));
	AudioComponent->SetRelativeLocation(FVector::ZeroVector);
}

void APlacedC2Explosive::BeginPlay()
{
	Super::BeginPlay();

	PerceptionStimuliComp->RegisterForSense(UAISense_Sight::StaticClass());
	PerceptionStimuliComp->RegisterWithPerceptionSystem();
	
	RadialForce = GetWorld()->SpawnActor<ARadialForceActor>(ARadialForceActor::StaticClass(), GetActorTransform());
	
	if (ExplosionScreenShake)
		ExplosionScreenShakeInst = NewObject<UCameraShakeBase>(this, ExplosionScreenShake);
}

void APlacedC2Explosive::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (bIsBeingRemoved && IsBeingRemovedBy)
	{
		IsBeingRemovedBy->UnlockAim();
		IsBeingRemovedBy->UnlockMovement();
	}
}

void APlacedC2Explosive::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	C2InteractableComponent->bShowIconWhenActionsLocked = true;
	C2InteractableComponent->UseActor = this;

	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		C2InteractableComponent->ActionSlot1.bCondition = CanRemoveC2(PlayerCharacter) && PlayerCharacter->PendingC2Removal == nullptr;
		C2InteractableComponent->ActionSlot2.bCondition = PlayerCharacter->PendingC2Removal != nullptr;
	
		if (PlayerCharacter->PendingC2Removal != this)
		{
			C2InteractableComponent->CurrentProgress = 0.0f;
			C2InteractableComponent->SetAnimatedIconName("RemoveC2Explosive");
			return;
		}
		
		// Removing C2 progress
		float TimeRemaining;
		if (PlayerCharacter->IsTableMontagePlayingWithTimeRemaining("fp_remove_c2", TimeRemaining))
		{
			C2InteractableComponent->SetAnimatedIconName("Empty");
			C2InteractableComponent->CurrentProgress = FMath::GetMappedRangeValueClamped(FVector2D(0.8f, 1.3f), FVector2D(1.0f, 0.0f), TimeRemaining);
		}
	}
}

void APlacedC2Explosive::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlacedC2Explosive, TargetItem);
	DOREPLIFETIME(APlacedC2Explosive, PlacementHit);
	DOREPLIFETIME(APlacedC2Explosive, PlacedByController);
	DOREPLIFETIME(APlacedC2Explosive, bIsBeingRemoved);
	DOREPLIFETIME(APlacedC2Explosive, IsBeingRemovedBy);
}

void APlacedC2Explosive::Server_DetonateC2_Implementation()
{
	// if we were removed via multitool don't do anything
	if (bRemovedViaMultitool)
	{
		return;
	}

	if (!TargetItem)
		return;

	if (HasAuthority())
	{
		// Run the multicast elements
		Multicast_OnC2Detonated();
		Multicast_OnC2Detonated_Implementation();
	}
	else
	{
		Multicast_OnC2Detonated_Implementation();
	}

	TargetItem->TakeDamage(DamageToInflict, FDamageEvent(DamageType), Cast<AReadyOrNotCharacter>(GetOwner()) ? Cast<AReadyOrNotCharacter>(GetOwner())->GetController() : nullptr, this);

	// Do damage in a radius
	//TArray<AActor*> IgnoreActors;
	//UGameplayStatics::ApplyRadialDamageWithFalloff(this, DamageToInflict, MinDamageToInflict, MeshComp->GetComponentLocation(), DamageInnerRadius, DamageOuterRadius, 1.0f, DamageType, IgnoreActors, this, Cast<APlayerCharacter>(GetOwner()) ? Cast<APlayerCharacter>(GetOwner())->GetController() : nullptr, ECC_WorldStatic);

	// After some time has passed, destroy the actor.
	FTimerHandle KillActorHandle;
	GetWorldTimerManager().SetTimer(KillActorHandle, this, &APlacedC2Explosive::PostExplosionKill, ExplosionPostKillTime, false);

	ICanPlaceC2On::Execute_OnC2Detonated(TargetItem, this);

	// Ensure that if there was a player removing the C2 on detonation, the interaction is ended
	if (bIsBeingRemoved && IsBeingRemovedBy)
	{
		Execute_EndInteract(this, IsBeingRemovedBy, C2InteractableComponent);
	}
}

void APlacedC2Explosive::Multicast_OnC2Detonated_Implementation()
{
	if (bDetonated)
		return;
	
	// Hide the mesh, don't hide the children
	MeshComp->SetHiddenInGame(true, false);

	// Fire off the explosion effect
	ADoor* Door = Cast<ADoor>(TargetItem);
	if (Door)
	{
		if (Door->GetC2ExplosionParticle())
		{
			ExplosionComp->SetTemplate(Door->GetC2ExplosionParticle());
		}

		// Explode On Far Side of Door
		UGameplayStatics::ApplyRadialDamageWithFalloff(GetWorld(), 100.0f, 0.0f, Door->IsPointInFrontOfDoor(GetActorLocation()) ? Door->GetBackMirrorPoint()->GetComponentLocation() : Door->GetFrontMirrorPoint()->GetComponentLocation(), 100.0f, 300.0f, 1.0f, StunDamageType, { this }, this, PlacedByController, ECC_PROJECTILE);

		//DrawDebugSphere(GetWorld(), Door->IsPointInFrontOfDoor(GetActorLocation()) ? Door->GetBackMirrorPoint()->GetComponentLocation() : Door->GetFrontMirrorPoint()->GetComponentLocation(), 100.0f, 10.0f, FColor::Red, false, 60.0f, 0.0f, 1.0f);
		//DrawDebugSphere(GetWorld(), Door->IsPointInFrontOfDoor(GetActorLocation()) ? Door->GetBackMirrorPoint()->GetComponentLocation() : Door->GetFrontMirrorPoint()->GetComponentLocation(), 300.0f, 10.0f, FColor::Orange, false, 60.0f, 0.0f, 1.0f);

		if(Door->IsDestructible())
		{
			Door->DestroyAllChunks(FVector(0,0,0), 0, false);
		}
		
		RadialForce->SetLifeSpan(5.0f);
		RadialForce->GetForceComponent()->DestructibleDamage = 100.0f;
		RadialForce->GetForceComponent()->Radius = 1000.0f;
		RadialForce->GetForceComponent()->ImpulseStrength = 10000.0f;
		RadialForce->GetForceComponent()->Falloff = RIF_Linear;
		RadialForce->GetForceComponent()->AddCollisionChannelToAffect(ECC_WorldDynamic);
		RadialForce->GetForceComponent()->AddCollisionChannelToAffect(ECC_PhysicsBody);

		const float Damage = AI_CONFIG_GET_FLOAT("C2DetonateMorale.Damage");
		const float InnerRadius = AI_CONFIG_GET_FLOAT("C2DetonateMorale.DamageInnerRadius");
		const float OuterRadius = AI_CONFIG_GET_FLOAT("C2DetonateMorale.DamageOuterRadius");
		const EEasingFunc::Type Curve = UReadyOrNotFunctionLibrary::StringToEasingFunc(AI_CONFIG_GET_STRING("C2DetonateMorale.DamageFalloffCurve"));

		const float DamageMultiplier = FMath::Max(URosterManager::GetSquadTraitValue("Breacher", GetWorld()), 1.0f);
		const float FinalDamage = Damage * DamageMultiplier;
		
		UMoraleComponent::ApplyRadialMoraleDamageWithFalloff(this, GetActorLocation(), FinalDamage, InnerRadius, OuterRadius, FMoraleDamageTraceParameters(), {ETeamType::TT_SUSPECT, ETeamType::TT_CIVILIAN}, Curve, "C2 Detonation");
		
		FTimerHandle Tmp;
		GetWorld()->GetTimerManager().SetTimer(Tmp, RadialForce, &ARadialForceActor::FireImpulse, FMath::FRandRange(0.1f, 0.3f), false);
		if (PlacedByController)
		{
			UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 4.0f, PlacedByController->GetPawn(), 200.0f, "ExplosionClose");
			UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 3.0f, PlacedByController->GetPawn(), 500.0f, "ExplosionMedium");
			UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 2.0f, PlacedByController->GetPawn(), 2000.0f, "ExplosionFar");
		}
	}

	ExplosionComp->Activate(true);

	// Do the screen shake
	if (bUseScreenShake)
	{
		AReadyOrNotPlayerCameraManager::PlayWorldCameraShake(GetWorld(), ExplosionScreenShakeInst, GetActorLocation(), 0.0f, CameraShakeRadius);
	}

	UFMODEvent* DoorExplosionEvent = Door->GetC2DetonationEvent();
	UFMODEvent* ExplosionEvent = DoorExplosionEvent ? DoorExplosionEvent : FMODC2ExplosionAudio;
	if (ExplosionEvent)
	{
		USoundSource* SoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), ExplosionEvent, GetActorTransform(), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
		if (SoundSource)
		{
			SoundSource->Attach(Door->GetDoorMesh(), NAME_None);
			SoundSource->bIsDoorAttachedSound = true;
			SoundSource->Play();
		}
	}
	
	bDetonated = true;
}

void APlacedC2Explosive::PostExplosionKill()
{
	Destroy();
}

bool APlacedC2Explosive::CanRemoveC2(APlayerCharacter* PlayerCharacter) const
{
	return !bDetonated;
}

void APlacedC2Explosive::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InteractableComponent)
{
	if (bIsBeingRemoved)
		return;

	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		bIsBeingRemoved = true;
		IsBeingRemovedBy = InteractCharacter;
		InteractCharacter->PendingC2Removal = this;

		UAnimMontage* C2RemovalMontage = InteractCharacter->GetMontageFromTable("fp_remove_c2");
		InteractCharacter->LockAim();
		InteractCharacter->LockMovement();

		if (HasAuthority())
		{
			GetWorld()->GetTimerManager().SetTimer(TH_Removal, this, &APlacedC2Explosive::RemoveFromTarget, C2RemovalMontage->Notifies[0].GetTriggerTime(), false);
			//InteractCharacter->Client_OnBeginRemoveC2(this);
		}

		// Only run the play montage if we're a client, or we're a listen server dealing with our own character
		// Play montage is networked which will cause client to restart their remove animation if called from the server
		if (!HasAuthority() || InteractCharacter->IsLocallyControlled())
		{
			InteractCharacter->PlayMontageFromTable("fp_remove_c2");
		}
	}
}

void APlacedC2Explosive::EndInteract_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InteractableComponent)
{
	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		bIsBeingRemoved = false;
		IsBeingRemovedBy = nullptr;
		InteractCharacter->PendingC2Removal = nullptr;
		//InteractCharacter->Client_OnEndRemoveC2();
		InteractCharacter->StopFPMontageFromTable("fp_remove_c2", 0.25f);
		InteractCharacter->UnlockAim();
		InteractCharacter->UnlockMovement();
		GetWorld()->GetTimerManager().ClearTimer(TH_Removal);
	}
}

bool APlacedC2Explosive::CanInteract_Implementation() const
{
	return !bIsBeingRemoved;
}

bool APlacedC2Explosive::CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const
{
	ACharacter* Character = UGameplayStatics::GetPlayerCharacter(this, 0);
	
	if (Character && Hit.GetActor())
	{
		return Hit.GetActor()->IsA(ADoor::StaticClass()) && FVector::DotProduct(GetActorForwardVector().RotateAngleAxis(90.0f, FVector::UpVector), (Character->GetActorLocation() - GetActorLocation()).GetSafeNormal()) > 0.0f;
	}
	
	return false;
}

void APlacedC2Explosive::DestroyAfterRemoval()
{
	Destroy();
}

void APlacedC2Explosive::RemoveFromTarget()
{
	if (TargetItem)
	{
		ICanPlaceC2On::Execute_OnC2Removed(TargetItem, this);
	}

	bRemovedViaMultitool = true;

	for (TActorIterator<ADetonator> It(GetWorld()); It; ++It)
	{
		ADetonator* Detonator = *It;
		
		Detonator->PlacedCharges.Remove(this);
		Detonator->PlacedChargesCount = Detonator->PlacedCharges.Num(); 
	}

	if (APlayerCharacter* PC = Cast<APlayerCharacter>(IsBeingRemovedBy))
		PC->Client_OnEndRemoveC2();

	ReturnToInventory();
	
	DestroyAfterRemoval();
}

/** Return the C2 to the character that placed it */
void APlacedC2Explosive::ReturnToInventory()
{
	AReadyOrNotCharacter* ReturnToActor = Cast<AReadyOrNotCharacter>(PlacedByController->GetPawn());
	if (!IsValid(ReturnToActor))
		return;
	
	ABaseItem* SpawnedItem = nullptr;
	// Spawning tactical item should replicate. Grenade count var doesn't matter here atm as bodysocket for almost all items seems wrong anyway
	// TODO: Fix how item bodysocket is set when spawning player inventory
	int32 GrenadeCount = 0;
	UBpGameplayHelperLib::SpawnTacticalItem(&SpawnedItem, ItemInventoryClass, ReturnToActor, true, GrenadeCount);
	ReturnToActor->GetInventoryComponent()->GetSpawnedGear().TacticalDevices.Add(SpawnedItem);
}
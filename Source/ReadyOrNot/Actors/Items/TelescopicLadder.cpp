//Void Interactive, 2017

#include "TelescopicLadder.h"

#include "Actors/LadderSnapZone.h"
#include "Characters/PlayerCharacter.h"

#include "Net/UnrealNetwork.h"

#include "Info/ReadyOrNotSignificanceManager.h"

#include "AIModule/Classes/Navigation/NavLinkProxy.h"
#include "AI/Navigation/NavLinkDefinition.h"

#include "NavigationSystem.h"
#include <NavLinkCustomComponent.h>

ATelescopicLadder::ATelescopicLadder()
{
	LadderVerticalIconPoint = CreateDefaultSubobject<USceneComponent>(TEXT("VerticalIconPoint"));
	LadderVerticalIconPoint->SetupAttachment(ItemMesh);

	LadderHorizontalIconPoint = CreateDefaultSubobject<USceneComponent>(TEXT("HorizontalIconPoint"));
	LadderHorizontalIconPoint->SetupAttachment(ItemMesh);

	LadderBottomMountPoint = CreateDefaultSubobject<USceneComponent>(TEXT("LadderBottomMountPoint"));
	LadderBottomMountPoint->SetupAttachment(ItemMesh);

	LadderTopMountPoint = CreateDefaultSubobject<USceneComponent>(TEXT("LadderTopMountPoint"));
	LadderTopMountPoint->SetupAttachment(ItemMesh);

	bReplicates = true;
}

void ATelescopicLadder::BeginPlay()
{
	Super::BeginPlay();
	
	OriginalActorTransform = GetActorTransform();
}

void ATelescopicLadder::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDropping)
	{
		ItemMesh->SetPhysicsAsset(DroppedPhysics, false);
	}
	else
	{
		ItemMesh->SetOwnerNoSee(false);
		ItemMesh->SetPhysicsAsset(PlacedPhysics, false);
	}

	if (!bFrameOne)
	{
		LocationFrameTwo = ItemMesh->GetComponentLocation();
	}
	else
	{
		LocationFrameOne = ItemMesh->GetComponentLocation();
	}

	if (!GetOwner())
	{
		DestroyGhostLadder();
	}

	//GEngine->AddOnScreenDebugMessage(9568, 1.0f, FColor::Yellow, "Ladder Actor Transform: " + GetActorTransform().ToString());
	//GEngine->AddOnScreenDebugMessage(9569, 1.0f, FColor::Yellow, "Ladder TP_ItemMesh Transform: " + ItemMesh->GetComponentTransform().ToString());
	//GEngine->AddOnScreenDebugMessage(9570, 1.0f, FColor::Yellow, "Ladder ItemMesh Transform: " + ItemMesh->GetComponentTransform().ToString()); 
	RollDegrees = ItemMesh->GetSocketTransform("Telescopic_Ladder").GetRotation().Rotator().Roll - 90.0f;
	//GEngine->AddOnScreenDebugMessage(9571, 120.0f, FColor::Red, "Placement Rot: " + FString::SanitizeFloat(RollDegrees));
	if (bDeployed)
	{
		DestroyGhostLadder();
		MovementDelta = (LocationFrameOne - LocationFrameTwo).Size() * DeltaSeconds;
		//GEngine->AddOnScreenDebugMessage(9567, 1.0f, FColor::Yellow, "Ladder Movement Delta: " + FString::SanitizeFloat(MovementDelta));

		if (MovementDelta < 0.0001f)
		{
			TimeSinceNotMoving += DeltaSeconds;
		}

		if (TimeSinceNotMoving > 0.5f)
		{
			bFreezeFrame = true;
			
		
			if (!CurrentSnapZone)
			{
				
				
				if (GetLocalRole() == ROLE_Authority)
				{

					FreezeTransform = ItemMesh->GetSocketTransform("Telescopic_Ladder");
					FActorSpawnParameters SpawnParams;
					SpawnParams.bNoFail = true;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					ALadderSnapZone* ls = GetWorld()->SpawnActor<ALadderSnapZone>(LadderSnapZoneBP, ItemMesh->GetComponentLocation(), ItemMesh->GetComponentRotation(), SpawnParams);
					if (ls)
					{
						ls->SetReplicates(true);
						ls->SetReplicateMovement(true);
						Server_DeployLadderAtZone(ls);
						CurrentSnapZone = ls;
					}
					if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
					{
						ANavLinkProxy* navproxy = GetWorld()->SpawnActorDeferred<ANavLinkProxy>(ANavLinkProxy::StaticClass(), FTransform());
						if (navproxy)
						{
							navproxy->FinishSpawning(navproxy->GetActorTransform());

							TArray<FNavigationLink> LinkPoints = { FNavigationLink() };
							ANavigationData* NavData = NavSys->GetNavDataForProps(APlayerCharacter::StaticClass()->GetDefaultObject<APlayerCharacter>()->GetNavAgentPropertiesRef());
							
							TSubclassOf<UNavigationQueryFilter> FilterClass = UNavigationQueryFilter::StaticClass();

							const FVector LadderLocation = ItemMesh->GetSocketTransform("Telescopic_Ladder").GetLocation();
							const FVector Joint3Location = ItemMesh->GetSocketTransform("Skeleton_Joint-(3)").GetLocation();
							FNavLocation RelativeStart(LadderLocation);
							FNavLocation RelativeEnd(Joint3Location);
							
							NavSys->ProjectPointToNavigation(LadderLocation, RelativeStart, FVector(300.0f), NavData, NavData ? UNavigationQueryFilter::GetQueryFilter(*NavData, this, FilterClass) : nullptr);
							NavSys->ProjectPointToNavigation(Joint3Location, RelativeEnd, FVector(300.0f), NavData,  NavData ? UNavigationQueryFilter::GetQueryFilter(*NavData, this, FilterClass) : nullptr);
							
							// Deprecated functions
							// Start: UNavigationSystemV1::ProjectPointToNavigation(GetWorld(), TP_ItemMesh->GetSocketTransform("Telescopic_Ladder").GetLocation(), NavData, FilterClass, FVector(300.0f));
							// End: UNavigationSystemV1::ProjectPointToNavigation(GetWorld(), TP_ItemMesh->GetSocketTransform("Skeleton_Joint-(3)").GetLocation(), NavData, FilterClass, FVector(300.0f))

							navproxy->GetSmartLinkComp()->SetLinkData(RelativeStart.Location, RelativeEnd.Location, ENavLinkDirection::BothWays);
							navproxy->SetSmartLinkEnabled(true);
							navproxy->GetSmartLinkComp()->SetNavigationRelevancy(true);
							
							NavSys->UpdateActorInNavOctree(*navproxy);
							NavSys->UpdateComponentInNavOctree(*navproxy->GetSmartLinkComp());
						}
					}
				}
				
			}

		}
		else
		{
			bFreezeFrame = false;
		}
		bFrameOne = !bFrameOne;
	}
	else
	{
		if (!GhostLadderActor)
		{
			APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
			if (pc)
			{
				if (pc->GetEquippedItem() == this)
				{
					SpawnGhostLadder();
				}
				else
				{
					DestroyGhostLadder();
				}
			}
			else
			{
				DestroyGhostLadder();
			}
			
		}
		else
		{
			FTransform pt = GetPlacementTransform();
			float dist = (GhostLadderActor->GetActorLocation() - pt.GetLocation()).Size();
			if (dist > 400.0f)
			{
				GhostLadderActor->SetActorLocation(pt.GetLocation());
			}
			GhostLadderActor->SetActorLocation(FMath::VInterpTo(GhostLadderActor->GetActorLocation(), pt.GetLocation(), DeltaSeconds, 25.0f));
			GhostLadderActor->SetActorRotation(FMath::RInterpTo(GhostLadderActor->GetActorRotation(), pt.GetRotation().Rotator(), DeltaSeconds, 25.0f));
		}
	}

	if (CurrentSnapZone)
	{
		UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(this);
		FRotator NewRot = FreezeTransform.GetRotation().Rotator();
		NewRot.Roll -= 90.0f;
		NewRot.Pitch = 0.0f;
		if (FMath::Abs(RollDegrees) < MaxRollDegreesBeforeUnwalkable)
		{
			CurrentSnapZone->Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
			ItemMesh->SetMaterial(1, CurrentSnapZone->InvalidPlacementMaterial);
		}
		else
		{
			FWalkableSlopeOverride SlopeOverride;
			SlopeOverride.SetWalkableSlopeAngle(90.0f);
			SlopeOverride.SetWalkableSlopeBehavior(WalkableSlope_Increase);
			CurrentSnapZone->Collision->SetWalkableSlopeOverride(SlopeOverride);
		}
		CurrentSnapZone->SetActorLocation(FreezeTransform.GetLocation());
		CurrentSnapZone->SetActorRotation(NewRot);
		//Server_DeployLadderAtZone_Implementation(CurrentSnapZone);
	}
	else
	{
		ItemMesh->SetMaterial(1, GetDefault<ATelescopicLadder>()->ItemMesh->GetMaterial(1));
	}

	if (CurrentSnapZone)
	{
		for (TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
		{
			APlayerCharacter* pc = *It;
			if (pc->GetMovementBaseActor(pc) == CurrentSnapZone || pc->GetMovementBaseActor(pc) == this)
			{
				bNoPickup = true;
				break;
			}
			else
			{
				bNoPickup = false;
			}
		}
	}
	
}

void ATelescopicLadder::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATelescopicLadder, bDeployed);
	DOREPLIFETIME(ATelescopicLadder, bDeployedHorizontal);
	DOREPLIFETIME(ATelescopicLadder, CurrentSnapZone);
	DOREPLIFETIME(ATelescopicLadder, RetractedRungCount);
	DOREPLIFETIME(ATelescopicLadder, bMounted);
	DOREPLIFETIME(ATelescopicLadder, FreezeTransform);
	DOREPLIFETIME(ATelescopicLadder, LastTransform);
}

void ATelescopicLadder::OnItemPrimaryUse()
{
	Server_PlaceLadder();
	Server_PlaceLadder_Implementation();
}

FTransform ATelescopicLadder::GetPlacementTransform()
{
	if (GetLocalRole() < ROLE_Authority)
		return LastTransform;

	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (!pc)
	{
		return LastTransform;
	}

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(pc);
	CollisionQueryParams.AddIgnoredActor(this);
	for (ABaseItem* bi : pc->GetInventoryComponent()->GetInventoryItems())
	{
		CollisionQueryParams.AddIgnoredActor(bi);
	}
	CollisionQueryParams.AddIgnoredActor(GhostLadderActor);
	FCollisionResponseParams CollisionResponseParams;

	// Get a point in front of the place, place ladder facing straight up
	FVector ForwardPosition = pc->GetFirstPersonCameraComponent()->GetComponentLocation() + pc->GetControlRotation().Vector() * 125.0f;
	ForwardPosition.Z = pc->GetActorLocation().Z - 25.0f;
	FHitResult ForwardTrace;
	GetWorld()->LineTraceSingleByChannel(ForwardTrace, pc->GetActorLocation(), ForwardPosition, ECollisionChannel::ECC_Visibility, CollisionQueryParams, CollisionResponseParams);
	if (ForwardTrace.bBlockingHit)
	{
		ForwardPosition = ForwardTrace.ImpactPoint;
	}
	//DrawDebugSphere(GetWorld(), ForwardPosition, 30.0f, 10, FColor::Yellow, true, 30.0f, 0.0f, 1.0f);
	// Do a trace down from this position to place on the floor
	FVector DownPosition = ForwardPosition + pc->GetActorUpVector() * -320.0f;

	GetWorld()->LineTraceSingleByChannel(GroundTrace, ForwardPosition, DownPosition, ECollisionChannel::ECC_Visibility, CollisionQueryParams, CollisionResponseParams);
	PlacementRotation = FRotator::ZeroRotator;
	PlacementRotation.Yaw = pc->GetActorRotation().Yaw - 90.0f;

	//DrawDebugSphere(GetWorld(), GroundTrace.ImpactPoint, 30.0f, 10, FColor::Green, false, 30.0f, 0.0f, 1.0f);
	LastTransform = FTransform(PlacementRotation, ForwardPosition);
	return LastTransform;
}

void ATelescopicLadder::SpawnGhostLadder()
{
	if (!bShowGhostLadder)
		return;

	DestroyGhostLadder();
	GhostLadderActor = GetWorld()->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass());
	if (GhostLadderActor)
	{
		GhostLadderActor->SetReplicates(true);
		GhostLadderActor->SetReplicateMovement(true);
		GhostLadderActor->GetSkeletalMeshComponent()->SetSkeletalMesh(ItemMesh->SkeletalMesh);
		GhostLadderActor->GetSkeletalMeshComponent()->SetMaterial(1, GhostLadderMaterial);
		if (bShowCollapsedLadder)
		{
			GhostLadderActor->GetSkeletalMeshComponent()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			GhostLadderActor->GetSkeletalMeshComponent()->SetAnimation(CollapsedLadderAnim);
		}
	}
}

void ATelescopicLadder::DestroyGhostLadder()
{
	if (GhostLadderActor)
	{
		GhostLadderActor->Destroy();
		GhostLadderActor = nullptr;
	}
}

void ATelescopicLadder::Server_DeployLadderAtZone_Implementation(ALadderSnapZone* NewSnapZone)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());

	if (!NewSnapZone)
		return;


	if (GetLocalRole() == ROLE_Authority)
	{
		// just call this like this in here because OnHolsterComplete is not suitable for placing down a ladder as a client, 
		// just call the draw function o nthe next gun and clear the pointers
		if (pc)
		{
			ABaseItem* NextEquippedItem = pc->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Primary);
			if (NextEquippedItem)
			{
				pc->GetInventoryComponent()->PutItemInHands(NextEquippedItem);
			}


		}	
		Multicast_DeployLadderAtZone(NewSnapZone);
	}

	// Deploy it!
	bDeployed = true;
	CurrentSnapZone = NewSnapZone;
	bDeployedHorizontal = CurrentSnapZone->bHorizontal;
	// Reset owner
	SetOwner(nullptr);

	// Adjust the position
	AttachToActor(CurrentSnapZone, FAttachmentTransformRules::KeepWorldTransform);
	//SetActorLocation(CurrentSnapZone->GhostLadder->GetComponentLocation());
	//SetActorRotation(CurrentSnapZone->GhostLadder->GetComponentRotation());
	ItemMesh->SetOnlyOwnerSee(false);
	ItemMesh->SetVisibility(true);
	ItemMesh->AttachToComponent(CurrentSnapZone->GhostLadder, FAttachmentTransformRules::SnapToTargetIncludingScale);
	ItemMesh->SetSimulatePhysics(false);

	// Update properties on the snap zone
	CurrentSnapZone->EnableCollision();
	CurrentSnapZone->Multicast_StopShowingGhostMesh();
	CurrentSnapZone->Multicast_StopShowingGhostMesh_Implementation();
	CurrentSnapZone->AttachedLadder = this;
	
	RetractedRungCount = CurrentSnapZone->MaxRetractedRungCount;
}

void ATelescopicLadder::Multicast_DeployLadderAtZone_Implementation(class ALadderSnapZone* NewSnapZone)
{
	UFMODBlueprintStatics::PlayEventAttached(CollideSoundEvent, ItemMesh, NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);
	if (GetLocalRole() < ROLE_Authority)
	{
		Server_DeployLadderAtZone_Implementation(NewSnapZone);
	}
}

void ATelescopicLadder::OnRep_Deployed()
{
// 	if (bDeployed)
// 	{
// 		Server_DeployLadderAtZone_Implementation(CurrentSnapZone);
// 	}
// 	else
// 	{
// 		Server_RemoveLadder_Implementation();
// 	}
}

void ATelescopicLadder::Client_OnItemPickedUp_Implementation(AActor* NewOwner, bool bEquipped)
{
	ResetLadderState();
	Super::Client_OnItemPickedUp_Implementation(NewOwner, bEquipped);

	UFMODBlueprintStatics::PlayEventAttached(PickupSoundEvent, ItemMesh, NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);
	SetOwner(NewOwner);
	bEasyPickup = false;
}

void ATelescopicLadder::Reset()
{
	ResetLadderState();

	Super::Reset();
}

void ATelescopicLadder::ResetLadderState()
{
	ItemMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	ItemMesh->SetSimulatePhysics(false);

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	if (CurrentSnapZone)
		CurrentSnapZone->SetActorTransform(OriginalActorTransform);
	
	Server_RemoveLadder();
	Server_RemoveLadder_Implementation();
	
	bDeployed = false;
	bMounted = false;
	bDropping = false;
	bDeployedHorizontal = false;
	bWallFound = false;
	bShowCollapsedLadder = true;
	bFreezeFrame = false;
	bFrameOne = true;

	DestroyGhostLadder();

	SetOwner(nullptr);

	ResetLadderTransform();
}

void ATelescopicLadder::ResetLadderTransform()
{
	FreezeTransform = OriginalActorTransform;

	SetActorLocation(OriginalActorTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(OriginalActorTransform.GetRotation(), ETeleportType::TeleportPhysics);
	
	ItemMesh->SetWorldLocation(OriginalActorTransform.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	ItemMesh->SetRelativeRotation(OriginalActorTransform.GetRotation());
}

void ATelescopicLadder::Server_PlaceLadder_Implementation()
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (!pc)
	{
		return;
	}


	if (GetLocalRole() == ROLE_Authority)
	{
		// just call this like this in here because OnHolsterComplete is not suitable for placing down a ladder as a client, 
		// just call the draw function o nthe next gun and clear the pointers
		if (pc)
		{
			ABaseItem* NextEquippedItem = pc->GetInventoryComponent()->GetInventoryItemOfType(EItemCategory::IC_Primary);
			if (NextEquippedItem)
			{
				pc->GetInventoryComponent()->PutItemInHands(NextEquippedItem);
			}


		}

		Multicast_PlaceLadder();
	}

	UFMODBlueprintStatics::PlayEventAttached(PlacementSoundEvent, ItemMesh, NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);
	// Deploy it!
	bDeployed = true;
	// Reset owner
	SetOwner(nullptr);

	ItemMesh->SetOnlyOwnerSee(false);
	ItemMesh->SetVisibility(true);

	// finally physics it up and give it a push
	if (HasAuthority())
	{
		ItemMesh->SetSimulatePhysics(true);
	}
	// 	SetActorLocation(GroundTrace.ImpactPoint);
	// 	SetActorRotation(PlacementRotation);

	ItemMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	ItemMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);

	if (GhostLadderActor)
	{
		ItemMesh->SetWorldLocation(GhostLadderActor->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		ItemMesh->SetWorldRotation(GhostLadderActor->GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		FTransform pt = GetPlacementTransform();
		ItemMesh->SetWorldLocation(pt.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		ItemMesh->SetWorldRotation(pt.GetRotation().Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
	}


	ItemMesh->AddImpulse(pc->ReplicatedControlRotation.Vector() * 50.0f, NAME_None, true);
}

void ATelescopicLadder::Multicast_PlaceLadder_Implementation()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		Server_PlaceLadder_Implementation();
	}
}

void ATelescopicLadder::Server_RemoveLadder_Implementation()
{
	
	TimeSinceNotMoving = 0.0f;
	bNoPickup = false;
	bDeployed = false;

	CurrentSnapZone = nullptr;
	for (TActorIterator<ALadderSnapZone> It(GetWorld()); It; ++It)
	{
		ALadderSnapZone* ls = *It;
		if (ls->AttachedLadder == this)
		{
			ls->AttachedLadder = nullptr;
		}
		if (ls->AttachedLadder == nullptr)
		{
			ls->Destroy();
		}
	}

	if (bDeployedHorizontal)
	{
		bDeployedHorizontal = false;
	}

	// Make this thing easier to pick up
	bEasyPickup = true;
}

void ATelescopicLadder::OnRep_CurrentSnapZone()
{
	Server_DeployLadderAtZone_Implementation(CurrentSnapZone);
}

void ATelescopicLadder::OnRep_AttachmentRep()
{
	if (bDeployed)
		return;

	Super::OnRep_AttachmentRep();
}

USceneComponent* ATelescopicLadder::GetClosestMountPoint(FVector Location)
{
	float DistTop = (LadderTopMountPoint->GetComponentLocation() - Location).Size();
	float DistBottom = (LadderBottomMountPoint->GetComponentLocation() - Location).Size();

	if (DistTop < DistBottom)
	{
		return LadderTopMountPoint;
	}
	else
	{
		return LadderBottomMountPoint;
	}
}

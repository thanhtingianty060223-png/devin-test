// Copyright Void Interactive, 2021

#include "TrapActorAttachedToDoor.h"

#include "CableComponent.h"
#include "Actors/Door.h"

#include "Components/DestructibleDoorChunkComponent.h"
#include "Components/ScoringComponent.h"
#include "Components/SplineComponent.h"

#include "Components/SplineMeshComponent.h"

ATrapActorAttachedToDoor::ATrapActorAttachedToDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.0167f;

	bInitializeTrapOnBeginPlay = false;

	ScoringComponent->bAutoAddToScorePool = false;
	
	CurveStrength = 40.0f;

	bReplicates = true;
}

void ATrapActorAttachedToDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATrapActorAttachedToDoor, MappedSplineLocation);
	DOREPLIFETIME(ATrapActorAttachedToDoor, bChunk1Destroyed);
	DOREPLIFETIME(ATrapActorAttachedToDoor, bChunk2Destroyed);
	DOREPLIFETIME(ATrapActorAttachedToDoor, bSubdoorChunk1Destroyed);
	DOREPLIFETIME(ATrapActorAttachedToDoor, bSubdoorChunk2Destroyed);
	DOREPLIFETIME(ATrapActorAttachedToDoor, AttachedToDoor);
}

void ATrapActorAttachedToDoor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!CanDisarmTrap() && TimeSinceTrapTriggered > 5.0f)
	{
		SetActorTickInterval(1.0f);

		return;
	}
	
	if (!AttachedToDoor)
	    return;
	
	if (HasAuthority())
	{
		// Dont bother animating the trap wire if all bottom door chunks are broken/not visible
		if (AttachedToDoor->GetSubDoor())
		{
			if (AttachedToDoor->AllBottomDoorChunksBroken() && AttachedToDoor->GetSubDoor()->AllBottomDoorChunksBroken())
				return;

			if (AttachedToDoor->IsClosed() && AttachedToDoor->GetSubDoor()->IsClosed())
				return;
		}
		else
		{
			if (AttachedToDoor->AllBottomDoorChunksBroken())
				return;

			if (AttachedToDoor->IsClosed())
				return;
		}

		if (AttachedToDoor->GetLastDoorUser() && AttachedToDoor->GetLastDoorUser()->IsSuspect())
		{
			MappedSplineLocation = 0.0f;
		}
		else
		{
			// Spline needs min 3 points (single doors) or 4 points (double doors) for trap wire to work. start, mid, end
			const int32 RequiredSplinePoints = (AttachedToDoor->GetSubDoor() ? 3 : 2);
			if (SplineComponent->GetNumberOfSplinePoints() > RequiredSplinePoints)
			{
				const bool bTrapInFront = AttachedToDoor->IsPointInFrontOfDoorway(GetActorLocation());

				const float TargetWirePosition = (bTrapInFront ? WireYPosition : -WireYPosition);

				bChunk1Destroyed = AttachedToDoor->IsDestructible() ? (AttachedToDoor->GetChunkComponents()[1]->IsDestroyed() || !AttachedToDoor->GetChunkComponents()[1]->IsVisible()) : false;
				bChunk2Destroyed = AttachedToDoor->IsDestructible() ? (AttachedToDoor->GetChunkComponents()[2]->IsDestroyed() || !AttachedToDoor->GetChunkComponents()[2]->IsVisible()) : false;
				
				bSubdoorChunk1Destroyed = false;
				bSubdoorChunk2Destroyed = false;

				float DoorOpenAmount = 0.0f;
				float TargetDoorAngle = (bTrapInFront ? -15.0f : 15.0f);
				
				if (((bTrapInFront && AttachedToDoor->GetOpenAmount() < 0.0f) || (!bTrapInFront && AttachedToDoor->GetOpenAmount() > 0.0f)) && !(bChunk1Destroyed && bChunk2Destroyed))
				{
					DoorOpenAmount = AttachedToDoor->GetOpenAmount();
				}

				if (AttachedToDoor->GetSubDoor())
				{
					bSubdoorChunk1Destroyed = AttachedToDoor->GetSubDoor()->IsDestructible() ? (AttachedToDoor->GetSubDoor()->GetChunkComponents()[1]->IsDestroyed() || !AttachedToDoor->GetSubDoor()->GetChunkComponents()[1]->IsVisible()) : false;
					bSubdoorChunk2Destroyed = AttachedToDoor->GetSubDoor()->IsDestructible() ? (AttachedToDoor->GetSubDoor()->GetChunkComponents()[2]->IsDestroyed() || !AttachedToDoor->GetSubDoor()->GetChunkComponents()[2]->IsVisible()) : false;

					if (!(bSubdoorChunk1Destroyed && bSubdoorChunk2Destroyed))
					{
						if ((bTrapInFront && AttachedToDoor->GetSubDoor()->GetOpenAmount() > 0.0f) || (!bTrapInFront && AttachedToDoor->GetSubDoor()->GetOpenAmount() < 0.0f))
						{
							const float Abs_MainDoorOpenAmount = FMath::Abs(AttachedToDoor->GetOpenAmount());
							const float Abs_SubDoorOpenAmount = FMath::Abs(AttachedToDoor->GetSubDoor()->GetOpenAmount());
							const bool bOpeningAwayFromTrapDirection = bTrapInFront ? AttachedToDoor->GetOpenAmount() > 0.0f : AttachedToDoor->GetOpenAmount() < 0.0f;
							const float CurrentMaxOpenAmount = bOpeningAwayFromTrapDirection ? Abs_SubDoorOpenAmount : FMath::Max(Abs_MainDoorOpenAmount, Abs_SubDoorOpenAmount);
							
							DoorOpenAmount = (bTrapInFront ? -CurrentMaxOpenAmount : CurrentMaxOpenAmount);
						}
					}
				}

				if (bTrapInFront)
				{
					if ((AttachedToDoor->GetOpenAmount() < 0.0f) || (AttachedToDoor->GetSubDoor() && AttachedToDoor->GetSubDoor()->GetOpenAmount() > 0.0f))
					{
						TargetDoorAngle = -15.0f;
					}
				}
				else
				{
					if ((AttachedToDoor->GetOpenAmount() > 0.0f) || (AttachedToDoor->GetSubDoor() && AttachedToDoor->GetSubDoor()->GetOpenAmount() < 0.0f))
					{
						TargetDoorAngle = 15.0f;
					}
				}
				
				MappedSplineLocation = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, TargetDoorAngle), FVector2D(0.0f, TargetWirePosition), DoorOpenAmount);
			}
		}
	}
	
	if (LOCAL_PLAYER)
	{
		const float DistanceToLocalPlayer = FVector::Distance(LocalPlayer->GetActorLocation(), GetActorLocation());

		if (DistanceToLocalPlayer < 3000.0f /*&& AttachedToDoor->IsOpen()*/)
		{
			const FVector CurrentLocation_1 = SplineComponent->GetLocationAtSplinePoint(1, ESplineCoordinateSpace::Local);
			const FVector CurrentLocation_2 = SplineComponent->GetLocationAtSplinePoint(2, ESplineCoordinateSpace::Local);
			if (AttachedToDoor && AttachedToDoor->GetSubDoor())
			{
				if ((!bChunk1Destroyed && bChunk2Destroyed) || (!bSubdoorChunk1Destroyed && bSubdoorChunk2Destroyed))
				{
					CurveStrength = 40.0f;
				}
				else
				{
					CurveStrength = 80.0f;
				}

				SplineComponent->SetLocationAtSplinePoint(1, FVector(SplineComponent->GetSplineLength()/2.0f - 5.0f, MappedSplineLocation, CurrentLocation_1.Z), ESplineCoordinateSpace::Local);
				SplineComponent->SetLocationAtSplinePoint(2, FVector(SplineComponent->GetSplineLength()/2.0f + 5.0f, MappedSplineLocation, CurrentLocation_2.Z), ESplineCoordinateSpace::Local);
			}
			else
			{
				if (!bChunk1Destroyed && bChunk2Destroyed)
				{
					CurveStrength = 40.0f;

					SplineComponent->SetLocationAtSplinePoint(1, FVector(SplineComponent->GetSplineLength()/2.75f, MappedSplineLocation/2.0f, CurrentLocation_1.Z), ESplineCoordinateSpace::Local);
				}
				else
				{
					CurveStrength = 80.0f;

					SplineComponent->SetLocationAtSplinePoint(1, FVector(CurrentLocation_1.X, MappedSplineLocation, CurrentLocation_1.Z), ESplineCoordinateSpace::Local);
				}
			}

			// Force linear interp mode, for smoother icon tracking on wire
			//for (int32 i = 0; i < SplineComponent->GetNumberOfSplinePoints(); i++)
			//{
			//	SplineComponent->SetSplinePointType(i, ESplinePointType::Linear);
			//}

			SplineComponent->SetVisibility(CanDisarmTrap());

			int32 i = 0;
			for (USplineMeshComponent* SplineMeshComponent : CableMeshComponents)
			{
				const float Distance = i * 50.0f;

				const FVector StartLocation = SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
				const FVector EndLocation = SplineComponent->GetLocationAtDistanceAlongSpline(Distance + 50.0f, ESplineCoordinateSpace::Local);
				const FVector StartTangent = SplineComponent->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local) * CurveStrength;
				const FVector EndTangent = SplineComponent->GetDirectionAtDistanceAlongSpline(Distance + 50.0f, ESplineCoordinateSpace::Local) * CurveStrength;

				SplineMeshComponent->SetVisibility(CanDisarmTrap());
				SplineMeshComponent->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent);
				i++;
			}

			if (AttachedToDoor)
			{
				const bool bTrapInFront = AttachedToDoor->IsPointInFrontOfDoorway(GetActorLocation());
				const bool bCanMoveY = AttachedToDoor->IsOpen_Backward() && bTrapInFront || AttachedToDoor->IsOpen_Forward() && !bTrapInFront;

				if (bCanMoveY && !AttachedToDoor->AllBottomDoorChunksBroken())
				{
					CutCableComponent1->EndLocation.Y = 30.0f;
					CutCableComponent2->SetRelativeLocation(FVector(CutCableComponent2->GetRelativeLocation().X, 30.0f, CutCableComponent2->GetRelativeLocation().Z));
				}
				else
				{
					CutCableComponent1->EndLocation.Y = 0.0f;
					CutCableComponent2->SetRelativeLocation(FVector(CutCableComponent2->GetRelativeLocation().X, 0.0f, CutCableComponent2->GetRelativeLocation().Z));
				}
			}
			else
			{
				CutCableComponent1->EndLocation.Y = 0.0f;
				CutCableComponent2->SetRelativeLocation(FVector(CutCableComponent2->GetRelativeLocation().X, 0.0f, CutCableComponent2->GetRelativeLocation().Z));
			}
		}
	}
}

void ATrapActorAttachedToDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

#if WITH_EDITOR
void ATrapActorAttachedToDoor::PostEditUndo()
{
	Super::PostEditUndo();
	
	GenerateCableMesh();
}

void ATrapActorAttachedToDoor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == "AttachedToDoor")
	{
		if (AttachedToDoor)
		{
			AttachedToDoor->AttachTrap(this, true);
		}
	}
}
#endif

bool ATrapActorAttachedToDoor::ShouldConsiderTrapCollisionFor(AReadyOrNotCharacter* InCharacter)
{
	if (!InCharacter)
		return false;

	// If door was somehow deleted, always consider for collision
	if (!AttachedToDoor)
		return true;
	
	const bool bSameSideAsTrap = AttachedToDoor->IsActorSameSideAsTrap(InCharacter);
	return Super::ShouldConsiderTrapCollisionFor(InCharacter) && (bSameSideAsTrap || (!bSameSideAsTrap && AttachedToDoor->IsOpen()));
}

void ATrapActorAttachedToDoor::GenerateCableMesh()
{
	for (TObjectIterator<USplineMeshComponent> It; It; ++It)
	{
		USplineMeshComponent* SplineMeshComponent = *It;
		if (SplineMeshComponent && SplineMeshComponent->GetOwner() == this)
		{
			DESTROY_COMPONENT(SplineMeshComponent)
		}
	}
	
	const float CableLength = SplineComponent->GetSplineLength() / 50.0f;
	const int32 NumOfMeshesNeeded = ((CableLength < 0.0f ? -1 : 1) * FMath::FloorToInt(FMath::Abs(CableLength))) + 1;

	CableMeshComponents.Empty(NumOfMeshesNeeded);

	for (int32 i = 0; i < NumOfMeshesNeeded; i++)
	{
		const float Distance = i * 50.0f;

		const FVector StartLocation = SplineComponent->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
		const FVector EndLocation = SplineComponent->GetLocationAtDistanceAlongSpline(Distance + 50.0f, ESplineCoordinateSpace::Local);
		const FVector StartTangent = SplineComponent->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local) * CurveStrength;
		const FVector EndTangent = SplineComponent->GetDirectionAtDistanceAlongSpline(Distance + 50.0f, ESplineCoordinateSpace::Local) * CurveStrength;
		
		USplineMeshComponent* SplineMeshComponent = NewObject<USplineMeshComponent>(this);
		SplineMeshComponent->RegisterComponent();
		SplineMeshComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		SplineMeshComponent->SetMobility(EComponentMobility::Movable);
		SplineMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SplineMeshComponent->SetStaticMesh(CableMesh);
		SplineMeshComponent->SetVisibility(true);
		
		SplineMeshComponent->SetRelativeLocation(CableTransform.GetLocation());
		SplineMeshComponent->SetRelativeRotation(CableTransform.GetRotation());
		SplineMeshComponent->SetWorldScale3D(CableTransform.GetScale3D());

		if (CableMaterial)
			SplineMeshComponent->SetMaterial(0, CableMaterial);
		
		SplineMeshComponent->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent);

		CableMeshComponents.Add(SplineMeshComponent);
	}
}

bool ATrapActorAttachedToDoor::CanCutWire() const
{
	if (!AttachedToDoor)
		return false;
	
	if (APlayerCharacter* LocalPlayer = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		const bool bTrapInFront = AttachedToDoor->IsPointInFrontOfDoorway(GetActorLocation());
		const bool bPlayerInFront = AttachedToDoor->IsPointInFrontOfDoorway(LocalPlayer->GetActorLocation());

		return (AttachedToDoor->IsOpen() || (bTrapInFront == bPlayerInFront)) || AttachedToDoor->AnyBottomDoorChunksBroken() || (AttachedToDoor->GetSubDoor() && (AttachedToDoor->GetSubDoor()->IsOpen() || AttachedToDoor->GetSubDoor()->AnyBottomDoorChunksBroken()));
	}

	return false;
}

void ATrapActorAttachedToDoor::OnTrapDisarmed_Implementation(AReadyOrNotCharacter* DisarmedBy)
{
	if (!IsValid(AttachedToDoor))
		return;

	if (AttachedToDoor->IsOpen() && DisarmedBy && DisarmedBy->IsA(ACyberneticCharacter::StaticClass()))
	{
		CutCable(0.1f);
	}

	Super::OnTrapDisarmed_Implementation(DisarmedBy);
	
	for (USplineMeshComponent* SplineMeshComponent : CableMeshComponents)
	{
		if (SplineMeshComponent)
		{
			SplineMeshComponent->SetVisibility(false);
		}
	}
}

void ATrapActorAttachedToDoor::OnTrapTriggered_Implementation(AReadyOrNotCharacter* TriggeredBy)
{
	Super::OnTrapTriggered_Implementation(TriggeredBy);

	if (TriggeredBy)
	{
		for (USplineMeshComponent* SplineMeshComponent : CableMeshComponents)
		{
			if (SplineMeshComponent)
			{
				SplineMeshComponent->SetVisibility(false);
			}
		}
	}
}

bool ATrapActorAttachedToDoor::CanDisarmTrap() const
{
	return AttachedToDoor && (TrapStatus == ETrapState::TS_Live || (TrapStatus == ETrapState::TS_Activated && bCanUseMultitoolWhenActivated));
}

void ATrapActorAttachedToDoor::BeginPlay()
{
	Super::BeginPlay();

	GenerateCableMesh();
	
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllDoorTrapActors.AddUnique(this);
	}
}

void ATrapActorAttachedToDoor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllDoorTrapActors.Remove(this);
	}
}

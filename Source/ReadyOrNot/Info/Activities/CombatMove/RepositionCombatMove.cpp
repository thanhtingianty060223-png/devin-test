// Copyright Void Interactive, 2023

#include "RepositionCombatMove.h"

#include "Characters/CyberneticController.h"

#include "Actors/Door.h"
#include "Actors/StackUpActor.h"
#include "NavAreas/NavArea_Default.h"
#include "Navigation/ReadyOrNotNavAreas.h"
#include "Navigation/ReadyOrNotNavQueries.h"

URepositionCombatMove::URepositionCombatMove()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "Reposition");
	
	bAbortMoveWhenActivityFinished = false;
	bAbortIfNotMovingForAWhile = false;
}

void URepositionCombatMove::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);
	
	GetCharacter()->ReasonsToSprint.AddUnique("repositioning");
}

void URepositionCombatMove::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	DeactivateDoorBlocker();
	
	TheDoor = nullptr;
	
	GetCharacter()->ReasonsToSprint.Remove("repositioning");
}

void URepositionCombatMove::FinishedActivity_NoOwner(bool bSuccess)
{
	Super::FinishedActivity_NoOwner(bSuccess);

	DeactivateDoorBlocker();
	
	TheDoor = nullptr;
}

void URepositionCombatMove::RequestCombatMove(const float DeltaTime)
{
	Super::RequestCombatMove(DeltaTime);

	if (Location != FVector::ZeroVector && HasReachedLocation(50.0f))
	{
		DeactivateDoorBlocker();
		
		FinishCombatMove(true);
		return;
	}

	if (Location != FVector::ZeroVector)
		return;
	
	FName CurrentRoom = NAME_None;
	if (AThreatAwarenessActor* TAA = OwningController->GetTargetingComp()->GetNearestThreat())
		CurrentRoom = TAA->OwningRoom;

	if (CurrentRoom == NAME_None)
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "Not in a room";
		#endif
		
		FinishCombatMove(false);
		return;
	}
	
	FRoom* Room = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Ref(CurrentRoom);
	if (!Room)
	{
		#if !UE_BUILD_SHIPPING
		UnableToCombatReason = "No Room Data";
		#endif
		
		FinishCombatMove(false);
		return;
	}

	ADoor* Door = Cast<ADoor>(OwningController->GetFocusActor());

	if (Door)
	{
		if (TryRepositionFromDoor(Door, CurrentRoom, false))
		{
			return;
		}
	}

	if (!Door)
	{
		for (ADoor* D : Room->AdditionalRootDoors)
		{
			if (D)
			{
				if (TryRepositionFromDoor(D, CurrentRoom, true))
				{
					break;
				}
			}
		}
	}
}

TSubclassOf<UNavigationQueryFilter> URepositionCombatMove::GetNavigationQueryOverride()
{
	return UNavigationQueryFilter::StaticClass();
}

bool URepositionCombatMove::TryRepositionFromDoor(ADoor* Door, FName CurrentRoom, bool bMustBeInThreshold)
{
	// is standing in front of a door? if so, move out of way
	
	FVector BestLocation = FVector::ZeroVector;
	
	FTransform Transform;
	Transform.SetLocation(Door->GetDoorMidLocation());
	Transform.SetRotation(Door->GetActorRotation().Quaternion());

	const float DoorWidth = Door->GetDoorSize().Y;
	
	FVector Extent = FVector(800.0f, DoorWidth, 120.0f);
	
	//DrawDebugBox(GetWorld(), Transform.GetLocation(), Extent, Transform.GetRotation(), FColor::Cyan, false, 0.1f);
	
	const bool bInsideDoorThreshold = UKismetMathLibrary::IsPointInBoxWithTransform(GetCharacter()->GetActorLocation(), Transform, Extent);

	if (!bMustBeInThreshold || (bInsideDoorThreshold && bMustBeInThreshold))
	{
		EDoorRoomPosition RoomPosition;

		const bool bFront = Door->IsActorInFrontOfDoorway(GetCharacter());
		if (bFront)
		{
			RoomPosition = Door->BackRoomPosition;
		}
		else
		{
			RoomPosition = Door->FrontRoomPosition;
		}

		if (RoomPosition == EDoorRoomPosition::CornerRight ||
			RoomPosition == EDoorRoomPosition::HallwayRight)
		{
			uint8 NumClearPoints = 0;

			const TArray<FClearPoint>* ClearPoints = bFront ? &Door->FrontLeftClearPoints : &Door->BackRightClearPoints;
			
			for (const FClearPoint& P : *ClearPoints)
			{
				if (NumClearPoints == 10) // cap it
					break;
				
				if (P.Direction == EClearDirection::Right)
					NumClearPoints++;
			}
				
			if (NumClearPoints > 2)
			{
				uint8 HalfwayIndex = FMath::DivideAndRoundUp(NumClearPoints, (uint8)2);

				if (HalfwayIndex > 0)
				{
					const FClearPoint& Point = (*ClearPoints)[HalfwayIndex];
					BestLocation = Point.Location;
				}
			}
		}
		else if (RoomPosition == EDoorRoomPosition::CornerLeft ||
				RoomPosition == EDoorRoomPosition::HallwayLeft)
		{
			uint8 NumClearPoints = 0;
			
			const TArray<FClearPoint>* ClearPoints = bFront ? &Door->FrontRightClearPoints : &Door->BackLeftClearPoints;
			
			for (const FClearPoint& P : *ClearPoints)
			{
				if (NumClearPoints == 10) // cap it
					break;
				
				if (P.Direction == EClearDirection::Right)
					NumClearPoints++;
			}
			
			if (NumClearPoints > 2)
			{
				uint8 HalfwayIndex = FMath::DivideAndRoundUp(NumClearPoints, (uint8)2);

				if (HalfwayIndex > 0)
				{
					const FClearPoint& Point = (*ClearPoints)[HalfwayIndex];
					BestLocation = Point.Location;
				}
			}
		}
		else if (RoomPosition == EDoorRoomPosition::Center)
		{
			const TArray<FClearPoint>* ClearPoints = nullptr;
			const TArray<FClearPoint>* RightClearPoints = bFront ? &Door->FrontRightClearPoints : &Door->BackRightClearPoints;
			const TArray<FClearPoint>* LeftClearPoints = bFront ? &Door->FrontLeftClearPoints : &Door->BackLeftClearPoints;

			uint8 NumClearPoints = 0;
			uint8 NumRightClearPoints_Right = 0;
			uint8 NumRightClearPoints_Left = 0;
			
			for (const FClearPoint& P : *RightClearPoints)
			{
				if (NumRightClearPoints_Right == 10) // cap it
					break;
				
				if (P.Direction == EClearDirection::Right)
					NumRightClearPoints_Right++;
			}
			
			for (const FClearPoint& P : *LeftClearPoints)
			{
				if (NumRightClearPoints_Left == 10) // cap it
					break;
				
				if (P.Direction == EClearDirection::Right)
					NumRightClearPoints_Left++;
			}
			
			int8 Diff = FMath::Abs((int8)NumRightClearPoints_Right - (int8)NumRightClearPoints_Left);
			if (Diff < 5)
			{
				if (Door->IsActorRightOfDoorway(GetCharacter()))
				{
					ClearPoints = RightClearPoints;
					NumClearPoints = NumRightClearPoints_Right;
				}
				else
				{
					ClearPoints = LeftClearPoints;
					NumClearPoints = NumRightClearPoints_Left;
				}
			}
			else
			{
				if (NumRightClearPoints_Right > NumRightClearPoints_Left)
				{
					ClearPoints = RightClearPoints;
					NumClearPoints = NumRightClearPoints_Right;
				}
				else if (NumRightClearPoints_Left > NumRightClearPoints_Right)
				{
					ClearPoints = LeftClearPoints;
					NumClearPoints = NumRightClearPoints_Left;
				}
				else
				{
					// calculate total distance
					float RightDistance = 0.0f;
					float LeftDistance = 0.0f;
					for (uint8 i = 0; i < NumRightClearPoints_Right; i++)
					{
						RightDistance += FVector::Distance((*RightClearPoints)[i].Location, (*RightClearPoints)[i+1].Location);
					}
					
					for (uint8 i = 0; i < NumRightClearPoints_Left; i++)
					{
						LeftDistance += FVector::Distance((*LeftClearPoints)[i].Location, (*LeftClearPoints)[i+1].Location);
					}

					if (FMath::IsNearlyEqual(RightDistance, LeftDistance, 20.0f)) // if small difference, just choose the side we're on
					{
						if (Door->IsActorRightOfDoorway(GetCharacter()))
						{
							ClearPoints = RightClearPoints;
							NumClearPoints = NumRightClearPoints_Right;
						}
						else
						{
							ClearPoints = LeftClearPoints;
							NumClearPoints = NumRightClearPoints_Left;
						}
					}
					else if (RightDistance > LeftDistance)
					{
						ClearPoints = RightClearPoints;
						NumClearPoints = NumRightClearPoints_Right;
					}
					else if (LeftDistance > RightDistance)
					{
						ClearPoints = LeftClearPoints;
						NumClearPoints = NumRightClearPoints_Left;
					}
				}
			}
			
			if (NumClearPoints <= 2)
			{
				NumClearPoints = ClearPoints->Num()-((float)ClearPoints->Num()*0.20f); // minus 20%, so it skews to the lower half of clear points (near the door)
			}
			
			if (NumClearPoints > 2)
			{
				uint8 HalfwayIndex = FMath::DivideAndRoundUp(NumClearPoints, (uint8)2);

				if (HalfwayIndex > 0)
				{
					const FClearPoint& Point = (*ClearPoints)[HalfwayIndex];
					BestLocation = Point.Location;
				}
			}
		}
		else if (RoomPosition == EDoorRoomPosition::Hallway)
		{
			const TArray<FClearPoint>* ClearPoints = nullptr;
			if (Door->IsActorRightOfDoorway(GetCharacter()))
				ClearPoints = bFront ? &Door->FrontRightClearPoints : &Door->BackRightClearPoints;
			else
				ClearPoints = bFront ? &Door->FrontLeftClearPoints : &Door->BackLeftClearPoints;

			uint8 NumClearPoints = 0;
			
			uint8 NumRightClearPoints = 0;
			for (const FClearPoint& P : *ClearPoints)
			{
				if (P.Direction == EClearDirection::Forward)
					NumClearPoints++;
				else if (P.Direction == EClearDirection::Right)
					NumRightClearPoints++;
			}
			
			if (NumClearPoints > 2)
			{
				uint8 HalfwayIndex = FMath::DivideAndRoundUp(NumClearPoints, (uint8)2);

				if (HalfwayIndex > 0)
				{
					const FClearPoint& Point = (*ClearPoints)[NumRightClearPoints+HalfwayIndex];
					BestLocation = Point.Location;
				}
			}
		}
		
		//if (RoomPosition != EDoorRoomPosition::Hallway && RoomPosition != EDoorRoomPosition::HallwayLeft && RoomPosition != EDoorRoomPosition::HallwayRight) // can't really move out of way of a hallway
		/*
		{
			const bool bIsOneSidedPosition = (Door->FrontRoomPosition == EDoorRoomPosition::CornerLeft ||
											 Door->FrontRoomPosition == EDoorRoomPosition::CornerRight ||
											 Door->FrontRoomPosition == EDoorRoomPosition::HallwayLeft ||
											 Door->FrontRoomPosition == EDoorRoomPosition::HallwayRight) &&
												 
											 (Door->BackRoomPosition == EDoorRoomPosition::CornerLeft ||
											 Door->BackRoomPosition == EDoorRoomPosition::CornerRight ||
											 Door->BackRoomPosition == EDoorRoomPosition::HallwayLeft ||
											 Door->BackRoomPosition == EDoorRoomPosition::HallwayRight);

			if (bIsOneSidedPosition)
			{
				const TArray<AStackUpActor*>* StackUpActors = nullptr;
				
				if (RoomPosition == EDoorRoomPosition::CornerLeft ||
					RoomPosition == EDoorRoomPosition::HallwayLeft)
				{
					StackUpActors = bFront ? &Door->FrontRightStackUpPoints : &Door->BackRightStackUpPoints;
				}
				else if (RoomPosition == EDoorRoomPosition::CornerRight ||
						 RoomPosition == EDoorRoomPosition::HallwayRight)
				{
					StackUpActors = bFront ? &Door->FrontLeftStackUpPoints : &Door->BackLeftStackUpPoints;
				}

				if (StackUpActors)
				{
					if (const AStackUpActor* ClosestStackUp = UReadyOrNotFunctionLibrary::FindClosestActorFromLocation<AStackUpActor>(GetCharacter()->GetActorLocation(), *StackUpActors))
					{
						BestLocation = ClosestStackUp->GetActorLocation();
					}
				}
			}

			float Offset = 200.0f;
			
			if (BestLocation == FVector::ZeroVector)
			{
				if (Door->IsActorRightOfDoorway(GetCharacter()))
					BestLocation = GetCharacter()->GetActorLocation() + Door->GetActorRightVector() * Offset;
				else
					BestLocation = GetCharacter()->GetActorLocation() + Door->GetActorRightVector() * -Offset;
			}

			{
				bool bBlah = false;
				
				for (const ADoor* OtherDoor : Room->AdditionalRootDoors)
				{
					if (Door != OtherDoor)
					{
						FTransform OtherTransform;
						Transform.SetLocation(OtherDoor->GetDoorMidLocation());
						Transform.SetRotation(OtherDoor->GetActorRotation().Quaternion());

						const float OtherDoorWidth = OtherDoor->GetDoorSize().Y;
						
						FVector OtherExtent = FVector(700.0f, OtherDoorWidth, 120.0f);
						bBlah = UKismetMathLibrary::IsPointInBoxWithTransform(BestLocation, OtherTransform, OtherExtent);
						if (bBlah)
						{
							Offset += 100.0f;
							Offset = -Offset;
							break;
						}
					}
				}
				
				uint8 i = 0;
				while (bBlah && i < 5)
				{
					if (Door->IsPointRightOfDoorway(BestLocation))
						BestLocation = GetCharacter()->GetActorLocation() + Door->GetActorRightVector() * Offset;
					else
						BestLocation = GetCharacter()->GetActorLocation() + Door->GetActorRightVector() * -Offset;

					for (const ADoor* OtherDoor : Room->AdditionalRootDoors)
					{
						if (Door != OtherDoor)
						{
							FTransform OtherTransform;
							Transform.SetLocation(OtherDoor->GetDoorMidLocation());
							Transform.SetRotation(OtherDoor->GetActorRotation().Quaternion());

							const float OtherDoorWidth = OtherDoor->GetDoorSize().Y;
							
							FVector OtherExtent = FVector(700.0f, OtherDoorWidth, 120.0f);
							if (!UKismetMathLibrary::IsPointInBoxWithTransform(BestLocation, OtherTransform, OtherExtent))
							{
								bBlah = false;
								break;
							}
						}
					}
					Offset += 100.0f;
					i++;
				}
			}

			if (BestLocation == FVector::ZeroVector)
			{
				FFindCoverQuery CoverSearchQuery = {};
				CoverSearchQuery.World = GetWorld();
				CoverSearchQuery.InstigatorTransform = Door->GetDoorway()->GetComponentTransform();
				CoverSearchQuery.OurTransform = GetCharacter()->GetActorTransform();
				CoverSearchQuery.SearchMode = ECoverSearchMode::NonWallOnly;
				CoverSearchQuery.CoverStance = ECoverStance::Both;
				CoverSearchQuery.SearchExtent = 1500.0f;
				CoverSearchQuery.SearchDangerRadiusFromInstigator = 200.0f;

				CoverQueryTests::UpdateTests();
				
				const TArray<FCoverQueryTest>& CoverQueries = {CoverQueryTests::SearchMode,
																CoverQueryTests::HeightDifference,
																CoverQueryTests::LineOfSight,
																CoverQueryTests::CoverBehindInstigator,
																CoverQueryTests::Distance,
																CoverQueryTests::DirectionMatch,
																CoverQueryTests::SufficientCover};
				
				const FVector Location_Offset = CoverSearchQuery.OurTransform.GetLocation() - FVector::UpVector * 100.0f;
				const FVector Origin = Location_Offset + (CoverSearchQuery.OurTransform.GetLocation() - CoverSearchQuery.InstigatorTransform.GetLocation()).GetSafeNormal2D() * (1500.0f/2);
				
				FTransform BoundsTransformTest;
				BoundsTransformTest.SetLocation(Origin);
				BoundsTransformTest.SetRotation(CoverSearchQuery.OurTransform.GetRotation());
				BoundsTransformTest.SetScale3D(FVector::OneVector);

				TArray<FCoverData> FoundCoverPoints;

				COVER_SYSTEM->GetAllCoverPointsInArea(FoundCoverPoints, Origin, CoverSearchQuery.SearchExtent);
				
				const FFindCoverResult& Result = FFindCoverTask::FindBestCover(FoundCoverPoints, CoverSearchQuery, CoverQueries, false);
				if (IsCoverResultValid(Result))
				{
					BestLocation = Result.BestCover->CoverLocation;
				}
			}
		}
		*/

		if (BestLocation == FVector::ZeroVector)
		{
			#if !UE_BUILD_SHIPPING
			UnableToCombatReason = "Couldn't find a good location to reposition";
			#endif
				
			FinishCombatMove(false);
			return false;
		}
				
		//DrawDebugBox(GetWorld(), BestLocation, FVector(15.0f), FColor::White, false, 0.1f);
			
		const FRoom OtherRoom = UReadyOrNotFunctionLibrary::GetRoomDataForLocation(BestLocation);
		if (OtherRoom.Name != CurrentRoom)
		{
			#if !UE_BUILD_SHIPPING
			UnableToCombatReason = "Reposition location leads to another room";
			#endif
				
			FinishCombatMove(false);
			return false;
		}

		SetLocation(BestLocation);

		Door->SetDoorBlockerAreaClass(UNavArea_NoSuspects::StaticClass());
		Door->ActivateDoorBlocker();
		Door->CurrentActivities.AddUnique(this);
		TheDoor = Door;

		return true;
	}

	return false;
}

void URepositionCombatMove::ResetData()
{
	Super::ResetData();
	
	TheDoor = nullptr;
}

void URepositionCombatMove::OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType, FNavPathSharedPtr NavPath)
{
	Super::OnPathFound(PathId, ResultType, NavPath);
}

void URepositionCombatMove::DeactivateDoorBlocker()
{
	if (TheDoor)
	{
		TheDoor->DeactivateDoorBlocker();
		TheDoor->CurrentActivities.Remove(this);
		TheDoor = nullptr;
	}
}

// Void Interactive, 2020

#include "FindCoverTask.h"

#include "CoverSystem.h"

#include "NavigationPath.h"
#include "NavigationSystem.h"

#include "ReadyOrNotRecastNavMesh.h"
#include "Actors/WorldDataGenerator.h"

#include "Characters/CyberneticController.h"

#include "GenericPlatform/GenericPlatformProcess.h"

#if UE_BUILD_SHIPPING
#define TEST_FAILED(...) return false;
#else
#define TEST_FAILED(...) FailureReason = __VA_ARGS__; return false
#endif

namespace CoverQueryTests
{
	FCoverQueryTest SearchMode = FCoverQueryTest(ECoverQueryTestPurpose::FilterOnly);
	FCoverQueryTest HeightDifference = FCoverQueryTest(ECoverQueryTestPurpose::FilterOnly);
	FCoverQueryTest LineOfSight = FCoverQueryTest(ECoverQueryTestPurpose::FilterOnly);
	FCoverQueryTest CoverBehindInstigator = FCoverQueryTest(ECoverQueryTestPurpose::FilterAndScore);
	FCoverQueryTest SufficientCover = FCoverQueryTest(ECoverQueryTestPurpose::FilterAndScore);
	FCoverQueryTest Distance = FCoverQueryTest(ECoverQueryTestPurpose::FilterAndScore);
	FCoverQueryTest DirectionMatch = FCoverQueryTest(ECoverQueryTestPurpose::FilterAndScore);
	FCoverQueryTest Room = FCoverQueryTest(ECoverQueryTestPurpose::FilterOnly);

	void UpdateTests()
	{
		SearchMode.TestFunction = COVER_QUERY_TEST_LAMBDA_SIG
		{
			// If searching for StandOnly cover points, skip cover points who are not a stand only cover
			if (InQuery.CoverStance == ECoverStance::StandOnly && !(CoverPoint->CrouchCoverType == ECrouchCoverType::Wall && CoverPoint->StandCoverType > EStandCoverType::Wall))
			{
				TEST_FAILED("Not Stand Only");
			}
			
			// If searching for CrouchOnly cover points, skip cover points who are not a crouch only cover
			if (InQuery.CoverStance == ECoverStance::CrouchOnly && !CoverPoint->bIsCrouchOnlyCover)
			{
				TEST_FAILED("Not Crouch Only");
			}
			
			const ECrouchCoverType CrouchCoverType = CoverPoint->CrouchCoverType;
			const EStandCoverType StandCoverType = CoverPoint->StandCoverType;

			const bool bIsWall = CrouchCoverType == ECrouchCoverType::Wall && StandCoverType == EStandCoverType::Wall;

			if (InQuery.SearchMode == ECoverSearchMode::WallOnly && !bIsWall)
			{
				TEST_FAILED("Not Wall Only");
			}

			if (InQuery.SearchMode == ECoverSearchMode::NonWallOnly && bIsWall)
			{
				TEST_FAILED("Wall Only");
			}

			return true;
		};

		HeightDifference.TestFunction = COVER_QUERY_TEST_LAMBDA_SIG
		{
			// Minus 1m to get the middle height of search box
			const float MaxZ = FMath::Max(InQuery.OurTransform.GetLocation().Z - 100.0f, CoverPoint->CoverLocation.Z);
			const float MinZ = FMath::Min(InQuery.OurTransform.GetLocation().Z - 100.0f, CoverPoint->CoverLocation.Z);
				
			const float ZHeightDifference = MaxZ - MinZ;

			if (ZHeightDifference > 200.0f)
			{
				TEST_FAILED(FString::Printf(TEXT("Height Difference Too Great (> 2m): %.2fm"), ZHeightDifference/100.0f));
			}
			
			return true;
		};

		LineOfSight.TestFunction = COVER_QUERY_TEST_LAMBDA_SIG
		{
			FHitResult HitResult;
			
			const bool bHasLOSToCover = !InQuery.World->GetWorld()->LineTraceSingleByChannel(HitResult, InQuery.InstigatorTransform.GetLocation(), CoverPoint->CoverLocation, ECC_Visibility, InQuery.CollisionQueryParams);

			if (bHasLOSToCover)
			{
				TEST_FAILED("Instigator Has LOS to Cover");
			}
			
			return true;
		};

		CoverBehindInstigator.TestFunction = COVER_QUERY_TEST_LAMBDA_SIG
		{
			const FVector DirectionToInstigatorFromCover = (CoverPoint->CoverLocation - InQuery.InstigatorTransform.GetLocation()).GetSafeNormal2D();
			const float CoverThresholdDotProduct = FVector::DotProduct(DirectionToInstigatorFromCover, (InQuery.OurTransform.GetLocation() - InQuery.InstigatorTransform.GetLocation()).GetSafeNormal2D());

			// Cover point is behind the instigator, disregard
			if (CoverThresholdDotProduct < 0.1f)
			{
				//TEST_FAILED(FString::Printf(TEXT("CoverThresholdDotProduct < 0.1: %.2f"), CoverThresholdDotProduct));
				TEST_FAILED("Cover Point Behind Instigator");
			}

			OutScore += FMath::GetMappedRangeValueClamped(FVector2D(-1.0f, 1.0f), FVector2D(0.0f, 1.0f), CoverThresholdDotProduct);

			return true;
		};
		
		SufficientCover.TestFunction = COVER_QUERY_TEST_LAMBDA_SIG
		{
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(InQuery.World.Get());
			if (!NavSys)
				return false;

			UCoverSystem* CoverSystem = InQuery.World->GetSubsystem<UCoverSystem>();
			if (!CoverSystem)
				return false;
			
			FHitResult HitResult;
			FCollisionQueryParams CollisionQueryParams = InQuery.CollisionQueryParams;
			CollisionQueryParams.bTraceComplex = true;
			
			FVector CrouchOffset = FVector::UpVector * 75.0f;
			//FVector StandOffset = CrouchOffset*2;
			if (const AReadyOrNotRecastNavMesh* ReadyOrNotNavMesh = Cast<AReadyOrNotRecastNavMesh>(NavSys->MainNavData))
			{
				CrouchOffset = ReadyOrNotNavMesh->CalculateCrouchOffset();
				//StandOffset = ReadyOrNotNavMesh->CalculateStandOffset();
			}

			const FVector CoverLocation_CrouchOffset = CoverPoint->CoverLocation + CrouchOffset;
			//const FVector CoverLocation_StandOffset = CoverPoint->CoverLocation + StandOffset;
			FVector DirectionToInstigator = (InQuery.InstigatorTransform.GetLocation() - CoverLocation_CrouchOffset).GetSafeNormal2D();
			FRotator DirectionToInstigator_Rot = DirectionToInstigator.Rotation();

			bool bIsNegative = FVector::DotProduct(CoverPoint->CoverRail.Direction, FVector::UpVector) < 0.0f;
			DirectionToInstigator_Rot.Pitch = bIsNegative ? -CoverPoint->CoverRail.Direction.Rotation().Pitch : CoverPoint->CoverRail.Direction.GetAbs().Rotation().Pitch;

			const FVector TraceStart = CoverLocation_CrouchOffset;
			const FVector TraceEnd = TraceStart + DirectionToInstigator_Rot.Vector() * 100.0f;

			FCollisionObjectQueryParams CollisionObjectQueryParams;
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
			CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_COVER);
			bool bHasPhysicalCoverInFront = InQuery.World->LineTraceSingleByObjectType(HitResult, TraceStart, TraceEnd, CollisionObjectQueryParams, CollisionQueryParams);

			if (UCoverSystem::IsNoCoverHit(HitResult))
				bHasPhysicalCoverInFront = false;

			#if WITH_EDITOR
			InQuery.DebugRenderElements.Lines.Add({TraceStart, TraceEnd, bHasPhysicalCoverInFront ? FColor::Green : FColor::Red});
			if (HitResult.ImpactPoint != FVector::ZeroVector)
				InQuery.DebugRenderElements.Lines.Add({HitResult.ImpactPoint, HitResult.ImpactPoint, FColor::Orange, 10.0f});
			#endif
			
			if (!bHasPhysicalCoverInFront)
			{
				TEST_FAILED("No Physical Cover In Front");
			}

			// Favor cover points that are more in shadow than others
			float DotProduct = FVector::DotProduct(CoverPoint->CoverNormal, DirectionToInstigator);
			OutScore += FMath::GetMappedRangeValueClamped(FVector2D(-1.0f, 1.0f), FVector2D(1.0f, -1.0f), DotProduct);

			// Favor the cover rails that are more perpendicular than others
			float DotProduct_Perp = FVector::DotProduct(CoverPoint->CoverRail.Direction, DirectionToInstigator);
			OutScore += FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(1.0f, -1.0f), FMath::Abs(DotProduct_Perp));

			FHitResult HitResult_ExposureTest_Left, HitResult_ExposureTest_Right;

			FVector TestLocation = HitResult.ImpactPoint;

			bool bInsufficientCover_Left = false;
			bool bInsufficientCover_Right = false;

			const auto IsCoverExposed = [&](FHitResult& OutHit, const FVector& InCoverLocation, const FVector& InCoverDirection, const float EdgeExtent)
			{
				const float TraceDistance = EdgeExtent * 1.15f;
				constexpr float Radius = 15.0f;
	
				const FVector Edge_TraceStart = InCoverLocation + InCoverDirection * TraceDistance;

				if (InQuery.World.Get()->SweepSingleByObjectType(OutHit, Edge_TraceStart, InCoverLocation, FQuat::Identity, {ECC_WorldStatic}, FCollisionShape::MakeSphere(Radius), CollisionQueryParams))
				{
					if (OutHit.Distance > 20.0f)
						return true;
				}

				return false;
			};

			const float EdgeExtent = CoverSystem->CoverGenSettings.LeftRightEdgeExtent;

			if (IsCoverExposed(HitResult_ExposureTest_Left, TestLocation, CoverPoint->CoverRail.Direction, EdgeExtent) /*||
				(!CoverPoint->bIsCrouchOnlyCover && IsCoverExposed(HitResult_ExposureTest_Left, TestLocation + FVector::UpVector * 75.0f, CoverPoint->CoverRail.Direction, EdgeExtent))*/)
			{
				bInsufficientCover_Left = true;
			}

			if (IsCoverExposed(HitResult_ExposureTest_Right, TestLocation, -CoverPoint->CoverRail.Direction, EdgeExtent)/* ||
				(!CoverPoint->bIsCrouchOnlyCover && IsCoverExposed(HitResult_ExposureTest_Right, TestLocation + FVector::UpVector * 75.0f, -CoverPoint->CoverRail.Direction, EdgeExtent))*/)
			{
				bInsufficientCover_Right = true;
			}

			OutScore += FMath::GetMappedRangeValueClamped(FVector2D(0.0f, EdgeExtent), FVector2D(1.0f, -1.0f), HitResult_ExposureTest_Left.Distance);
			OutScore += FMath::GetMappedRangeValueClamped(FVector2D(0.0f, EdgeExtent), FVector2D(1.0f, -1.0f), HitResult_ExposureTest_Right.Distance);

			const auto DrawExposureTest = [&](const FHitResult& InHitResult)
			{
				if (InHitResult.TraceStart != FVector::ZeroVector)
				{
					#if WITH_EDITOR
					InQuery.DebugRenderElements.Lines.Add({InHitResult.TraceStart, InHitResult.TraceEnd, InHitResult.bBlockingHit ? FColor::Black : FColor::Red, 2.0f});
					InQuery.DebugRenderElements.Lines.Add({InHitResult.ImpactPoint, InHitResult.ImpactPoint, FColor::Yellow, 10.0f});
					
					InQuery.DebugRenderElements.Texts.Add({FString::Printf(L"Exposure: %.2f", InHitResult.Distance), InHitResult.TraceStart, FColor::White});
					#endif
				}
			};

			DrawExposureTest(HitResult_ExposureTest_Left);
			DrawExposureTest(HitResult_ExposureTest_Right);
			
			if (bInsufficientCover_Left && bInsufficientCover_Right)
			{
				TEST_FAILED("Insufficient Cover In Front");
			}
			
			return true;
		};

		Distance.TestFunction = COVER_QUERY_TEST_LAMBDA_SIG
		{
			//FHitResult LOSToInstigator;
			//const bool bLOSToInstigator = !InQuery.World->LineTraceSingleByChannel(LOSToInstigator, InQuery.InstigatorTransform.GetLocation(), InQuery.OurTransform.GetLocation(), ECC_Visibility, InQuery.CollisionQueryParams);
			const float DistanceToInstigator = FVector::Distance(InQuery.InstigatorTransform.GetLocation(), CoverPoint->CoverLocation);
			const bool bIsEnemyNearCoverPoint = /*bLOSToInstigator &&*/ DistanceToInstigator < InQuery.SearchDangerRadiusFromInstigator;

			if (bIsEnemyNearCoverPoint || DistanceToInstigator < InQuery.SearchDangerRadiusFromInstigator/1.75f) // Never consider cover around the immediate range of the instigator
			{
				TEST_FAILED("Instigator Near Cover");
			}

			OutScore += FMath::GetMappedRangeValueClamped(FVector2D(0.0f, InQuery.SearchDangerRadiusFromInstigator), FVector2D(0.0f, 1.0f), DistanceToInstigator);

			return true;
		};

		DirectionMatch.TestFunction = COVER_QUERY_TEST_LAMBDA_SIG
		{
			const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(InQuery.World.Get());
			if (!NavSys)
				return false;

			//const UCoverSystem* CoverSystem = InQuery.World->GetSubsystem<UCoverSystem>();
			//if (!CoverSystem)
			//	return false;
				
			const ECrouchCoverType CrouchCoverType = CoverPoint->CrouchCoverType;
			const EStandCoverType StandCoverType = CoverPoint->StandCoverType;

			const bool bIsWall = CrouchCoverType == ECrouchCoverType::Wall && StandCoverType == EStandCoverType::Wall;
			if (!bIsWall)
			{
				FVector CrouchOffset = FVector::UpVector * 75.0f;
				//FVector StandOffset = CrouchOffset*2;
				if (const AReadyOrNotRecastNavMesh* ReadyOrNotNavMesh = Cast<AReadyOrNotRecastNavMesh>(NavSys->MainNavData))
				{
					CrouchOffset = ReadyOrNotNavMesh->CalculateCrouchOffset();
					//StandOffset = ReadyOrNotNavMesh->CalculateStandOffset();
				}

				const FVector CoverLocation_CrouchOffset = CoverPoint->CoverLocation + CrouchOffset;
				//const FVector CoverLocation_StandOffset = CoverPoint->CoverLocation + StandOffset;
				
				const FVector DirectionToInstigator = (InQuery.InstigatorTransform.GetLocation() - CoverLocation_CrouchOffset).GetSafeNormal2D();

				const FVector CrossProduct = FVector::CrossProduct(CoverPoint->CoverNormal, FVector::UpVector);

				const float CoverViewDotProduct = FVector::DotProduct(DirectionToInstigator, -CoverPoint->CoverNormal);

				// If too dis-similar from the normal, probably not a good cover direction
				if (CoverViewDotProduct < 0.4f)
				{
					TEST_FAILED(FString::Printf(TEXT("CoverViewDotProduct < 0.4: %.2f"), CoverViewDotProduct));
				}
				
				#if WITH_EDITOR
				if (InQuery.bDrawDebug)
				{
					InQuery.DebugRenderElements.Lines.Add({CoverLocation_CrouchOffset, CoverLocation_CrouchOffset + CrossProduct * 100.0f, FColor::Orange});

					//DrawDebugLine(InQuery.World.Get(), TraceStart, TraceStart + CrossProduct * 100.0f, FColor::Orange, false, InQuery.DrawTime);
				}
				#endif
				
				const float ForwardDotProduct = FVector::DotProduct(DirectionToInstigator, CoverPoint->CoverNormal);
				const float RightDotProduct_Instigator = FVector::DotProduct(DirectionToInstigator, CrossProduct);

				bool bDotProductMatchesCoverType_Crouch = false;
				bool bDotProductMatchesCoverType_Stand = false;
				
				if (ForwardDotProduct > 0.0f)
				{
					TEST_FAILED("Cover Not In Shadow");
				}
				
				bool bDotProductMatchesCoverDirection_Crouch = true;
				bool bDotProductMatchesCoverDirection_Stand = true;

				constexpr float DirectionDotProductThreshold = -0.1f;

				if (!HasUpCover(CrouchCoverType))
				{
					if (HasLeftCover(StandCoverType) && !HasRightCover(StandCoverType))
					{
						const float CoverFireDotProduct_Stand_Left = FVector::DotProduct(DirectionToInstigator, CoverPoint->StandCoverDirection.Left);
						
						if (CoverFireDotProduct_Stand_Left < DirectionDotProductThreshold)
						{
							bDotProductMatchesCoverDirection_Stand = false;
						}
					}

					if (!HasLeftCover(StandCoverType) && HasRightCover(StandCoverType))
					{
						const float CoverFireDotProduct_Stand_Right = FVector::DotProduct(DirectionToInstigator, CoverPoint->StandCoverDirection.Right);

						if (CoverFireDotProduct_Stand_Right < DirectionDotProductThreshold)
						{
							bDotProductMatchesCoverDirection_Stand = false;
						}
					}

					if (HasLeftCover(CrouchCoverType) && !HasRightCover(CrouchCoverType))
					{
						const float CoverFireDotProduct_Crouch_Left = FVector::DotProduct(DirectionToInstigator, CoverPoint->CrouchCoverDirection.Left);
						
						if (CoverFireDotProduct_Crouch_Left < DirectionDotProductThreshold)
						{
							bDotProductMatchesCoverDirection_Crouch = false;
						}
					}

					if (!HasLeftCover(CrouchCoverType) && HasRightCover(CrouchCoverType))
					{
						const float CoverFireDotProduct_Crouch_Right = FVector::DotProduct(DirectionToInstigator, CoverPoint->CrouchCoverDirection.Right);
						
						if (CoverFireDotProduct_Crouch_Right < DirectionDotProductThreshold)
						{
							bDotProductMatchesCoverDirection_Crouch = false;
						}
					}
				}

				// At least one cover direction must match
				if (!bDotProductMatchesCoverDirection_Crouch && !bDotProductMatchesCoverDirection_Stand)
				{
					TEST_FAILED("Direction Mismatch: Instigator not on " + CrouchCoverTypeToString(CrouchCoverType) + " side");
				}
				
				const bool bHasLeftCover_Crouch = HasLeftCover(CrouchCoverType);
				const bool bHasLeftCover_Stand = HasLeftCover(StandCoverType);
				const bool bHasRightCover_Crouch = HasRightCover(CrouchCoverType);
				const bool bHasRightCover_Stand = HasRightCover(StandCoverType);

				if (RightDotProduct_Instigator < 0.0f || CoverViewDotProduct > 0.95f)
				{
					bDotProductMatchesCoverType_Crouch =	bHasLeftCover_Crouch ||
															CrouchCoverType == ECrouchCoverType::UpOnly;

					bDotProductMatchesCoverType_Stand =		bHasLeftCover_Stand;

					OutScore += FMath::GetMappedRangeValueClamped(FVector2D(-1.0f, 1.0f), FVector2D(1.0f, 0.0f), RightDotProduct_Instigator);
				}

				if (RightDotProduct_Instigator > 0.0f || CoverViewDotProduct > 0.95f)
				{
					if (!bDotProductMatchesCoverType_Crouch)
					{
						bDotProductMatchesCoverType_Crouch =	bHasRightCover_Crouch ||
																CrouchCoverType == ECrouchCoverType::UpOnly;
					}
					
					if (!bDotProductMatchesCoverType_Stand)
					{
						bDotProductMatchesCoverType_Stand =		bHasRightCover_Stand;
					}

					OutScore += FMath::GetMappedRangeValueClamped(FVector2D(-1.0f, 1.0f), FVector2D(0.0f, 1.0f), RightDotProduct_Instigator);
				}

				// At least one cover stance must match
				if (!bDotProductMatchesCoverType_Crouch && !bDotProductMatchesCoverType_Stand)
				{
					TEST_FAILED("Direction Mismatch: " + FString(RightDotProduct_Instigator < 0.0f ? "Right" : "Left"));
				}
			}

			return true;
		};
		
		Room.TestFunction = COVER_QUERY_TEST_LAMBDA_SIG
		{
			if (InQuery.World->WorldType == EWorldType::Editor)
			{
				if (const AWorldDataGenerator* WorldData = AWorldDataGenerator::Get(InQuery.World.Get()))
				{
					if (WorldData->RoomData.Rooms.Num() == 0)
					{
						return true; // if no room system, then skip this test as it's not super critical.
					}
				}
			}
			else
			{
				if (AReadyOrNotGameState* GS = InQuery.World->GetGameState<AReadyOrNotGameState>())
				{
					if (!GS->RoomData || GS->RoomData->Rooms.Num() == 0)
					{
						return true; // if no room system, then skip this test as it's not super critical.
					}
				}
			}
			
			FVector Direction = FVector::ZeroVector;
			if (CoverPoint->StandCoverType == EStandCoverType::LeftOnly)
			{
				Direction = CoverPoint->StandCoverDirection.Left;
			}
			else if (CoverPoint->StandCoverType == EStandCoverType::RightOnly)
			{
				Direction = CoverPoint->StandCoverDirection.Right;
			}
			
			FVector ForwardLocation = CoverPoint->CoverLocation + Direction * 100.0f + -CoverPoint->CoverNormal * 200.0f;
			ForwardLocation.Z += 50.0f;
			
			if (Direction == FVector::ZeroVector)
			{
				ForwardLocation = InQuery.InstigatorTransform.GetLocation() - FVector::UpVector * 50.0f;
			}

			FRoom* CurrentRoom = nullptr;
			FRoom* CoverRoom = nullptr;
			const FRoom* InstigatorRoom = nullptr;
			
			{
				FRoom* RoomPtr;
				if (InQuery.World->WorldType == EWorldType::Editor)
					RoomPtr = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Editor(CoverPoint->CoverLocation + FVector::UpVector * 50.0f);
				else
					RoomPtr = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(CoverPoint->CoverLocation + FVector::UpVector * 50.0f);
				
				if (RoomPtr)
				{
					CoverRoom = RoomPtr;
					CurrentRoom = CoverRoom;
				}
			}

			if (Direction != FVector::ZeroVector)
			{
				FRoom* RoomPtr;
				if (InQuery.World->WorldType == EWorldType::Editor)
					RoomPtr = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Editor(ForwardLocation);
				else
					RoomPtr = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(ForwardLocation);
				
				if (RoomPtr)
				{
					CurrentRoom = RoomPtr;
				}
			}

			{
				FRoom* RoomPtr;
				if (InQuery.World->WorldType == EWorldType::Editor)
					RoomPtr = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Editor(InQuery.InstigatorTransform.GetLocation());
				else
					RoomPtr = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(InQuery.InstigatorTransform.GetLocation());
				
				if (RoomPtr)
				{
					InstigatorRoom = RoomPtr;
				}
			}
			
			#if WITH_EDITOR
			InQuery.DebugRenderElements.Lines.Add({ForwardLocation, ForwardLocation, FColor::Cyan, 20.0f});
			InQuery.DebugRenderElements.Lines.Add({CoverPoint->CoverLocation + FVector::UpVector * 50.0f, CoverPoint->CoverLocation + FVector::UpVector * 50.0f, FColor::Magenta, 20.0f});
			#endif

			if (!CurrentRoom || !InstigatorRoom)
			{
				TEST_FAILED("No room data");
			}
			
			bool bConnected = CoverRoom == InstigatorRoom || CurrentRoom->ConnectingRooms.Contains(InstigatorRoom->Name);

			// is current room connected to instigator room?
			if (!bConnected)
			{
				for (FName NextRoom : CurrentRoom->ConnectingRooms)
				{
					if (CoverRoom)
					{
						if (NextRoom == CoverRoom->Name)
							continue;
					}

					FRoom* Next = nullptr;
					if (InQuery.World->WorldType == EWorldType::Editor)
					{
						Next = UReadyOrNotFunctionLibrary::GetRoomDataFromName_Editor(NextRoom);
					}
					else
					{
						if (AReadyOrNotGameState* GS = InQuery.World->GetGameState<AReadyOrNotGameState>())
						{
							Next = GS->RoomData->Rooms.FindByPredicate([=](const FRoom& Element)
							{
								return Element.Name == NextRoom;
							});
						}
					}
					
					if (Next)
					{
						if (Next->ConnectingRooms.Contains(InstigatorRoom->Name))
						{
							bConnected = true;
							break;
						}
					}
				}
			}

			if (!bConnected)
			{
				TEST_FAILED(CurrentRoom->Name.ToString() + " is not connected to room " + InstigatorRoom->Name.ToString());
			}
			
			#if WITH_EDITOR
			if (CoverRoom)
				InQuery.DebugRenderElements.Texts.Add({FString::Printf(L"Room: %s", *CoverRoom->Name.ToString()), CoverPoint->CoverLocation + FVector::UpVector * 50.0f, FColor::White});
			
			const FRoom* ChosenRoom = Direction == FVector::ZeroVector ? InstigatorRoom : CurrentRoom;
			InQuery.DebugRenderElements.Texts.Add({FString::Printf(L"Room: %s", *ChosenRoom->Name.ToString()), ForwardLocation, FColor::White});
			#endif
			
			return true;
		};
	}
}

FFindCoverTask::FFindCoverTask(TSharedPtr<FCoverData>& OutBestCover, const FFindCoverQuery& InFindCoverQuery, const FFindCoverDelegate& InCoverFoundDelegate, const FFindCoverDelegate& InNoCoverFoundDelegate)
{
	BestCover = OutBestCover;
	
	CoverSearchQuery = InFindCoverQuery;
	NewCoverFoundDelegate = InCoverFoundDelegate;
	NoCoverFoundDelegate = InNoCoverFoundDelegate;

	#if WITH_EDITOR
	ensure(IsCoverQueryValid(InFindCoverQuery));
	ensure(BestCover != nullptr);
	#endif
}

TStatId FFindCoverTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FFindCoverTask, STATGROUP_ThreadPoolAsyncTasks);
}

static void AsyncFindCoverFinished(FFindCoverDelegate Delegate, uint32 NumCoverFound, float TimeMs)
{
	Delegate.ExecuteIfBound(NumCoverFound, TimeMs);
}

void FFindCoverTask::DoWork()
{
	DECLARE_CYCLE_STAT(TEXT("FSimpleDelegateGraphTask.Async find cover"), STAT_FSimpleDelegateGraphTask_AsyncFindCover, STATGROUP_TaskGraphTasks);

	if (!IsCoverQueryValid(CoverSearchQuery))
	{
		#if !UE_BUILD_SHIPPING
		ULog::Error("Failed to start cover search. CoverSearchQuery is not valid");
		#endif

		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(FSimpleDelegateGraphTask::FDelegate::CreateStatic(AsyncFindCoverFinished, NoCoverFoundDelegate, 0u, 0.0f), GET_STATID(STAT_FSimpleDelegateGraphTask_AsyncFindCover), nullptr, ENamedThreads::GameThread);

		return;
	}

	if (!BestCover.IsValid())
	{
		#if !UE_BUILD_SHIPPING
		ULog::Error("Failed to start cover search. Would not be able to return a result since BestCover is null");
		#endif

		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(FSimpleDelegateGraphTask::FDelegate::CreateStatic(AsyncFindCoverFinished, NoCoverFoundDelegate, 0u, 0.0f), GET_STATID(STAT_FSimpleDelegateGraphTask_AsyncFindCover), nullptr, ENamedThreads::GameThread);

		return;
	}
	
	UCoverSystem* CoverSystem = CoverSearchQuery.World.Get()->GetSubsystem<UCoverSystem>();
	if (!CoverSystem)
	{
		#if !UE_BUILD_SHIPPING
		ULog::Error("Failed to start cover search. Cover subsystem does not exist");
		#endif
		
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(FSimpleDelegateGraphTask::FDelegate::CreateStatic(AsyncFindCoverFinished, NoCoverFoundDelegate, 0u, 0.0f), GET_STATID(STAT_FSimpleDelegateGraphTask_AsyncFindCover), nullptr, ENamedThreads::GameThread);

		return;
	}

	CoverQueryTests::UpdateTests();

	#if !UE_BUILD_SHIPPING
	const uint64 Start = FPlatformTime::Cycles64();
	#endif

	// Query the octree to get all cover points in the specified area
	TArray<FCoverData> FoundCoverPoints;
	
	const FVector Location_Offset = CoverSearchQuery.OurTransform.GetLocation() - FVector::UpVector * 100.0f;
	const FVector Origin = Location_Offset + (CoverSearchQuery.OurTransform.GetLocation() - CoverSearchQuery.InstigatorTransform.GetLocation()).GetSafeNormal2D() * CoverSearchQuery.SearchExtent/2;

	CoverSystem->GetAllCoverPointsInArea(FoundCoverPoints, Origin, CoverSearchQuery.SearchExtent);
	
	#if !UE_BUILD_SHIPPING
	const uint64 End = FPlatformTime::Cycles64();
	const uint64 ElapsedTime = End - Start;
	ULog::Info("Octree Query | Operation Took: " + FString::SanitizeFloat(FPlatformTime::ToMilliseconds64(ElapsedTime)) + "ms");
	#endif

	const TArray<FCoverQueryTest>& CoverQueries = {CoverQueryTests::SearchMode,
													CoverQueryTests::HeightDifference,
													CoverQueryTests::LineOfSight,
													CoverQueryTests::CoverBehindInstigator,
													CoverQueryTests::Distance,
													CoverQueryTests::DirectionMatch,
													CoverQueryTests::SufficientCover,
													CoverQueryTests::Room};
	
	FFindCoverResult Result = FindBestCover(FoundCoverPoints, CoverSearchQuery, CoverQueries);
	
	Result.NumCoverFound = FoundCoverPoints.Num();

	// If not the same cover as before
	if (BestCover.IsValid() && IsCoverResultValid(Result))
	{
		*BestCover.Pin() = *Result.BestCover;

		// trigger calling delegate on main thread - otherwise it may depend too much on stuff being thread safe
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(FSimpleDelegateGraphTask::FDelegate::CreateStatic(AsyncFindCoverFinished, NewCoverFoundDelegate, Result.NumCoverFound, Result.TimeMs), GET_STATID(STAT_FSimpleDelegateGraphTask_AsyncFindCover), nullptr, ENamedThreads::GameThread);
	}
	else
	{
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(FSimpleDelegateGraphTask::FDelegate::CreateStatic(AsyncFindCoverFinished, NoCoverFoundDelegate, Result.NumCoverFound, Result.TimeMs), GET_STATID(STAT_FSimpleDelegateGraphTask_AsyncFindCover), nullptr, ENamedThreads::GameThread);
	}
}

FFindCoverResult FFindCoverTask::FindBestCover(TArray<FCoverData>& InCoverPoints, FFindCoverQuery& InQuery, const TArray<FCoverQueryTest>& InTests, bool bAsync)
{
	FFindCoverResult Result;
	Result.NumCoverFound = InCoverPoints.Num();
	
	if (!InQuery.World.IsValid())
		return Result;
	
	if (InCoverPoints.Num() == 0)
		return Result;

	if (InCoverPoints.Num() == 1)
	{
		Result.BestCover = &InCoverPoints[0];
		Result.NumCoverFound = 1;
		Result.TimeMs = 0.0f;
		
		return Result;
	}
	
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(InQuery.World.Get());
	if (!NavSys)
		return Result;

	UCoverSystem* CoverSystem = InQuery.World->GetSubsystem<UCoverSystem>();
	if (!CoverSystem)
		return Result;

	#if !UE_BUILD_SHIPPING
	const uint64 Start = FPlatformTime::Cycles64();
	#endif
	
	InQuery.CollisionQueryParams.bReturnFaceIndex = true;
	InQuery.CollisionQueryParams.bReturnPhysicalMaterial = true;
	
	TArray<FCoverQueryItem> FilteredCoverPoints;
	
	FilteredCoverPoints.Reserve(InCoverPoints.Num());

	// Filter out all occupied cover points
	for (FCoverData& CoverPoint : InCoverPoints)
	{
		if (CoverSystem->IsCoverPointOccupied(CoverPoint.CoverLocation))
			continue;

		FilteredCoverPoints.Add({&CoverPoint});
	}

	for (const FCoverQueryTest& QueryTest : InTests)
	{
		for (FCoverQueryItem& QueryItem : FilteredCoverPoints)
		{
			FCoverData* CoverPoint = QueryItem.CoverPoint;

			if (QueryItem.bDiscarded)
				continue;
				
			float OutScore = 0.0f;
				
			#if UE_BUILD_SHIPPING
			if (!QueryTest.TestFunction(InQuery, CoverPoint, OutScore))
			#else
			FString FailureReason = "";
			if (!QueryTest.TestFunction(InQuery, CoverPoint, OutScore, FailureReason))
			#endif
			{
				if (QueryTest.TestPurpose == ECoverQueryTestPurpose::FilterAndScore || QueryTest.TestPurpose == ECoverQueryTestPurpose::FilterOnly)
				{
					QueryItem.bDiscarded = true;
						
					#if !UE_BUILD_SHIPPING
					QueryItem.DiscardReason = FailureReason;
					#endif
				}
			}

			if (QueryTest.TestPurpose == ECoverQueryTestPurpose::FilterAndScore || QueryTest.TestPurpose == ECoverQueryTestPurpose::ScoreOnly)
				QueryItem.Score += OutScore * QueryTest.ScoringFactor;
		}
	}

	if (bAsync)
	{
		TMap<uint32, const TSharedPtr<FNavigationPath, ESPMode::ThreadSafe>> FindPathResults;

		// Find paths to all filtered cover points (async)
		for (FCoverQueryItem& QueryItem : FilteredCoverPoints)
		{
			if (QueryItem.bDiscarded)
				continue;

			const FCoverData* CoverPoint = QueryItem.CoverPoint;
			
			FPathFindingQuery PathFindingQuery(InQuery.World.Get(), *NavSys->MainNavData, FVector(InQuery.OurTransform.GetLocation().X, InQuery.OurTransform.GetLocation().Y, CoverPoint->CoverLocation.Z), CoverPoint->CoverLocation, NavSys->MainNavData->GetQueryFilter(InQuery.NavQueryFilter));
			PathFindingQuery.SetAllowPartialPaths(false);
			
			FEvent* Semaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);
			if (!Semaphore)
				continue;

			FNavPathQueryDelegate NavQueryResult;
			NavQueryResult.BindLambda([&](uint32 PathId, const ENavigationQueryResult::Type ResultType, const FNavPathSharedPtr Path)
			{
				if (!FindPathResults.Contains(PathId))
					FindPathResults.Add(PathId, Path);

				Semaphore->Trigger();
			});

			uint32 PathFindingRequestId = INVALID_NAVQUERYID;

			// Queue up a task on the game thread, as this CANNOT run on a background thead.
			AsyncTask(ENamedThreads::GameThread, [&PathFindingQuery, &NavQueryResult, &NavSys, &PathFindingRequestId]()
			{
				PathFindingRequestId = NavSys->FindPathAsync(PathFindingQuery.NavAgentProperties, PathFindingQuery, NavQueryResult);
			});

			// Wait for the results from the async
			// this makes sure that PathFindingRequestId is set in the AsyncTask before contunuing
			Semaphore->Wait();
			FGenericPlatformProcess::ReturnSynchEventToPool(Semaphore);

			// Path found
			QueryItem.CoverPath = FindPathResults[PathFindingRequestId];
		}
	}
	else
	{
		// Find paths to all filtered cover points (sync)
		for (FCoverQueryItem& QueryItem : FilteredCoverPoints)
		{
			if (QueryItem.bDiscarded)
				continue;

			const FCoverData* CoverPoint = QueryItem.CoverPoint;
			
			FPathFindingQuery PathFindingQuery(InQuery.World.Get(), *NavSys->MainNavData, FVector(InQuery.OurTransform.GetLocation().X, InQuery.OurTransform.GetLocation().Y, CoverPoint->CoverLocation.Z), CoverPoint->CoverLocation, NavSys->MainNavData->GetQueryFilter(InQuery.NavQueryFilter));
			PathFindingQuery.SetAllowPartialPaths(false);

			FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery);
			QueryItem.CoverPath = PathFindingResult.Path;
		}
	}

	// Filter and score all paths to the filtered cover points
	for (FCoverQueryItem& QueryItem : FilteredCoverPoints)
	{
		if (QueryItem.bDiscarded)
			continue;

		float& Score = QueryItem.Score;
		
		if (!QueryItem.CoverPath.IsValid())
		{
			QueryItem.bDiscarded = true;

			#if !UE_BUILD_SHIPPING
			QueryItem.DiscardReason = "Invalid Path";
			#endif

			continue;
		}

		if (QueryItem.CoverPath->IsPartial())
		{
			QueryItem.bDiscarded = true;

			#if !UE_BUILD_SHIPPING
			QueryItem.DiscardReason = "Partial Path";
			#endif

			continue;
		}

		const TArray<FNavPathPoint>& PathPoints = QueryItem.CoverPath->GetPathPoints();

		if (PathPoints.Num() < 1)
		{
			QueryItem.bDiscarded = true;

			#if !UE_BUILD_SHIPPING
			QueryItem.DiscardReason = "Partial Path";
			#endif

			continue;
		}

		// Is too risky to traverse?
		bool bAnyPathPointCloseToInstigator = false;
		for (int32 i = 2; i < PathPoints.Num(); i++) // Start from 3rd point
		{
			const float DistanceToPathPoint = FVector::Distance(InQuery.InstigatorTransform.GetLocation(), PathPoints[i]);
			if (DistanceToPathPoint < InQuery.SearchDangerRadiusFromInstigator/1.75f)
			{
				bAnyPathPointCloseToInstigator = true;
				break;
			}
		}

		// Don't allow paths that go towards instigator within their danger zone, too risky to traverse
		bool bAnyPathPointGoesTowardsInstigator = false;

		for (int32 i = 1; i < PathPoints.Num(); i++)
		{
			FVector PathPoint = PathPoints[i].Location;

			// Is this path point inside of the instigator's danger radius
			float Distance = FVector::Distance(PathPoint, InQuery.InstigatorTransform.GetLocation());
			if (Distance < InQuery.SearchDangerRadiusFromInstigator)
			{
				const FVector DirectionToPathPoint = (PathPoint - InQuery.OurTransform.GetLocation()).GetSafeNormal2D();
				const FVector DirectionToInstigator = (InQuery.InstigatorTransform.GetLocation() - InQuery.OurTransform.GetLocation()).GetSafeNormal2D();
				
				float PathPointDotProduct = FVector::DotProduct(DirectionToPathPoint, DirectionToInstigator);
				if (PathPointDotProduct > 0.8f)
				{
					bAnyPathPointGoesTowardsInstigator = true;
					break;
				}
			}
		}

		// Too risky to traverse
		if (bAnyPathPointCloseToInstigator)
		{
			QueryItem.bDiscarded = true;

			#if !UE_BUILD_SHIPPING
			QueryItem.DiscardReason = "Path Point Near Instigator";
			#endif
			
			continue;
		}

		if (bAnyPathPointGoesTowardsInstigator)
		{
			QueryItem.bDiscarded = true;

			#if !UE_BUILD_SHIPPING
			QueryItem.DiscardReason = "Path Point Goes Towards Instigator";
			#endif

			continue;
		}

		float PathLength = QueryItem.CoverPath->GetLength();

		if (PathLength > InQuery.SearchExtent*2.0f)
		{
			QueryItem.bDiscarded = true;

			#if !UE_BUILD_SHIPPING
			QueryItem.DiscardReason = FString::Printf(TEXT("Path Too Long (> %.2fm): %.2fm"), (InQuery.SearchExtent*2.0f)/100.0f, PathLength/100.0f);
			#endif

			continue;
		}

		// The closer the cover point, the better
		Score += FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1000.0f), FVector2D(1.0f, 0.0f), PathLength);

		// The less time to get into the cover point (at sprint speed), the better
		Score += FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 2.0f), FVector2D(1.0f, 0.0f), PathLength/500.0f);
	}

	if (FilteredCoverPoints.Num() > 0)
	{
		FilteredCoverPoints.Sort([](const FCoverQueryItem& Lhs, const FCoverQueryItem& Rhs)
		{
			return Lhs.Score > Rhs.Score;
		});

		InQuery.QueryItems = FilteredCoverPoints;
		
		FilteredCoverPoints.RemoveAll([](const FCoverQueryItem& QueryItem)
		{
			return QueryItem.bDiscarded || QueryItem.Score <= 0.0f;
		});
		
		if (FilteredCoverPoints.Num() > 0)
		{
			const FCoverQueryItem& Winner = FilteredCoverPoints[0];

			Result.BestCover = Winner.CoverPoint;
			Result.Path = Winner.CoverPath;
		}
	}

	#if !UE_BUILD_SHIPPING
	const uint64 End = FPlatformTime::Cycles64();
	const uint64 ElapsedTime = End - Start;
	Result.TimeMs = FPlatformTime::ToMilliseconds64(ElapsedTime);
	#endif
	
	return Result;
}

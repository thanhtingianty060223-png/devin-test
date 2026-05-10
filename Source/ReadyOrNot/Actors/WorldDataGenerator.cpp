// Copyright Void Interactive, 2023

#include "WorldDataGenerator.h"

#include "CoverLandmark.h"
#include "CoverLandmarkProxy.h"
#include "StackUpActor.h"
#include "PatrolPoint.h"

#include "Actors/Door.h"
#include "Actors/ThreatAwarenessActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/MirrorPortalComponent.h"

#include "Gameplay/TrapActorAttachedToDoor.h"

#include "Info/WorldBuildingPlacementActor.h"

#include "Navigation/NavLinkProxy.h"
#include "Navigation/ReadyOrNotNavQueries.h"

#include "NavigationSystem.h"
#include "NavLinkCustomComponent.h"
#include "NavMesh/NavMeshBoundsVolume.h"

#include "Kismet/KismetMathLibrary.h"

#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "ReadyOrNotNavigationSystem.h"
#include "ReadyOrNotRecastNavMesh.h"
#include "RoomVisualizer.h"
#include "Actors/CoverPoint.h"
#include "Gameplay/EvidenceActor.h"
#include "Gameplay/IncapacitatedHuman.h"
#include "Info/Activities/WorldBuildingActivity.h"
#include "Misc/UObjectToken.h"
#include "Subsystems/ThreatAwarenessSubsystem.h"

#if WITH_EDITOR
#include "EditorLevelLibrary.h"
#include "Settings/EditorLoadingSavingSettings.h"
#endif

AWorldDataGenerator::AWorldDataGenerator()
{
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bStartWithTickEnabled = false;
    PrimaryActorTick.TickInterval = 9999.0f;
    
    SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));
    SceneComponent->SetMobility(EComponentMobility::Static);
    SetRootComponent(SceneComponent);

    bFindCameraComponentWhenViewTarget = false;
    SetCanBeDamaged(false);
}

AWorldDataGenerator* AWorldDataGenerator::Get(UWorld* World)
{
    if (!World)
        return nullptr;
    
	for (TActorIterator<AWorldDataGenerator>It(World); It;)
	{
	    return *It;
	}

    return nullptr;
}

void AWorldDataGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void Internal_RebuildNavigation(UWorld* World)
{
    if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
    {
        NavSys->Build();
    }
}

#if WITH_EDITOR
void AWorldDataGenerator::CheckForErrors()
{
    Super::CheckForErrors();
    FMessageLog MapCheck("MapCheck");
    for (TActorIterator<AWorldDataGenerator>It(GetWorld()); It; ++It)
    {
        if(*It != this)
        {
            MapCheck.Error()
                       ->AddToken(FUObjectToken::Create(this))
                       ->AddToken(FTextToken::Create(FText::FromString("Multiple World Generators found in level. See https://voidinteractive.atlassian.net/browse/RN-4041")));
            break;
        }
    }
}
#endif

void AWorldDataGenerator::GenerateWorld()
{
    bIsGenerating = true;
    
    for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
    {
        AThreatAwarenessActor* a = *It;
        a->Destroy();
    }

    for (TActorIterator<AStackUpActor> It(GetWorld()); It; ++It)
    {
        It->Destroy();
    }

	TArray<ADoor*> AllDoors;
    AllDoors.Reserve(100);
    
    for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
    {
        ADoor* Door = *It;
        Door->SetActorScale3D(Door->GetActorScale3D().GetAbs());
        
        Door->FrontThreatAwarenessPoints.Empty();
        Door->BackThreatAwarenessPoints.Empty();
        Door->FrontLeftStackUpPoints.Empty();
        Door->FrontRightStackUpPoints.Empty();
        Door->BackLeftStackUpPoints.Empty();
        Door->BackRightStackUpPoints.Empty();

        AllDoors.AddUnique(Door);
    }

    TArray<AAISpawn*> AISpawners;
    AISpawners.Reserve(100);
    for (TActorIterator<AAISpawn> It(GetWorld()); It; ++It)
    {
        AISpawners.AddUnique(*It);
    }

    for (ADoor* Door : AllDoors)
    {
        Door->Setup();
        Door->SetSubDoor(nullptr, false);
        
        if (Door->IsDoorwayOnly())
            Door->ActivateDoorBlocker();
    }

    // Auto link subdoors
    for (int32 i = 0; i < AllDoors.Num(); i++)
    {
        for (int32 y = 0; y < AllDoors.Num(); y++)
        {
            ADoor* DoorA = AllDoors[i];
            ADoor* DoorB = AllDoors[y];
            if (DoorA != DoorB)
            {
                float Dist = (DoorA->GetDoorMidLocation() - DoorB->GetDoorMidLocation()).Size();

                if (Dist < 140.0f)
                {
                    FCollisionObjectQueryParams CollisionObjectQuery;
                    CollisionObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
                    CollisionObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);

                    FCollisionQueryParams CollisionQueryParams;
                    CollisionQueryParams.AddIgnoredActor(DoorA);
                    CollisionQueryParams.AddIgnoredActor(DoorB);

                    FHitResult Hit;
                    GetWorld()->LineTraceSingleByObjectType(Hit, DoorA->GetDoorway()->GetComponentLocation() + DoorA->GetActorForwardVector() * 10.0f, DoorB->GetDoorway()->GetComponentLocation() + DoorA->GetActorForwardVector() * 10.0f, CollisionObjectQuery, CollisionQueryParams);

                    if (!Hit.bBlockingHit)
                    {
                        FRotator DoorARot = DoorA->GetActorRotation();
                        FRotator DoorBRot = DoorB->GetActorRotation();

                        FRotator DeltaRotator = UKismetMathLibrary::NormalizedDeltaRotator(DoorARot, DoorBRot);
                        if (FMath::IsNearlyEqual(FMath::Abs(DeltaRotator.Yaw), 180.0f, 3.0f))
                        {
                            //V_LOGM(LogReadyOrNot, "%s linked to %s", *GetNameSafe(DoorA), *GetNameSafe(DoorB));
                            
                            DoorA->SetSubDoor(DoorB, true);
                            DoorB->SetSubDoor(DoorA, false);
                        }
                    }
                }
            }
        }
    }

    Internal_RebuildNavigation(GetWorld());

    bDoorwaysBlocked = true;
    
    // Generate stair points
    if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        if (AReadyOrNotRecastNavMesh* NavMesh = Cast<AReadyOrNotRecastNavMesh>(NavSys->MainNavData))
        {
            NavMesh->GenerateStairPoints();
        }
    }
    
    ThreatIndex = 0;

    for (ADoor* Door : AllDoors)
    {
        // Temporarily reset these
        //Door->bCanIssueOrdersOnFrontSide = true;
        //Door->bCanIssueOrdersOnBackSide = true;
        
        GenerateDoorPositions(Door);
        
        // test for door frame (double doors are assumed to be framed)
        Door->bHasFrame = true; 
        if (!Door->GetSubDoor())
        {
            FVector BaseLocation = Door->GetDoorMidLocation();
            FVector TraceStart = BaseLocation + FVector::UpVector * 60.0f;

            FHitResult Hit, Hit2;
            FCollisionQueryParams CollisionQueryParams;
            CollisionQueryParams.bTraceComplex = true;
            CollisionQueryParams.AddIgnoredActor(Door);
            float Distance = Door->GetDoorSize().Y + 10.0f;
            bool bLeftHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceStart + Door->GetActorRightVector() * Distance, ECC_Visibility, CollisionQueryParams);
            bool bRightHit = GetWorld()->LineTraceSingleByChannel(Hit2, TraceStart, TraceStart + Door->GetActorRightVector() * -Distance, ECC_Visibility, CollisionQueryParams);

            DrawDebugLine(GetWorld(), TraceStart, TraceStart + Door->GetActorRightVector() * Distance, FColor::Orange, false, 5.0f, 0, 1.5f);
            DrawDebugLine(GetWorld(), TraceStart, TraceStart + Door->GetActorRightVector() * -Distance, FColor::Orange, false, 5.0f, 0, 1.5f);

            Door->bHasFrame = bLeftHit && bRightHit;
        }
        
        // Generate world threat awareness actors
        {
            FScopedSlowTask SlowTask(AllDoors.Num()*1000.0f,NSLOCTEXT("GeneratingThreatAwarenessActors", "Generating Threat Awareness Actors", "Generating Threat Awareness Actors"));
            SlowTask.MakeDialog();
            
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.bNoFail = true;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            
            for (int32 i = -10; i < 10; i++)
            {
                for (int32 y = -10; y < 10; y++)
                {
                    for (int32 z = -2; z < 2; z++)
                    {
                        SlowTask.EnterProgressFrame();

                        //SpawnThreatAwarenessActorAtLocation(Door->GetActorLocation() + FVector(i * 250.0f, y * 250.0f, z * 500.0f), EThreatLevel::TL_Low);

                        // Spawn threat awareness actor at location
                        // plus other setup
                        {
                            FVector ThreatLocation = Door->GetActorLocation() + FVector(i * 250.0f, y * 250.0f, z * 500.0f);
                            if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
                            {
                                FNavLocation NavLocation(ThreatLocation);
                                if (NavSys->ProjectPointToNavigation(NavLocation.Location, NavLocation, FVector(100.0f, 100.0f, 50.0f)))
                                {
                                    ThreatLocation = NavLocation;
                                }
                                else
                                {
                                    continue;
                                }
                            }

                            if (!IsSuitableSpawnLocationForThreatAwareness(ThreatLocation))
                                continue;

                            if (AThreatAwarenessActor* t = GetWorld()->SpawnActor<AThreatAwarenessActor>(AThreatAwarenessActor::StaticClass(), ThreatLocation, FRotator::ZeroRotator, SpawnParameters))
                            {
                                t->CheckIsOutside();
                                t->SetThreatLevel(EThreatLevel::TL_Low);
                                
                                for (ADoor* OtherDoor : AllDoors)
                                {
                                    if (t->GetThreatLevel() < EThreatLevel::TL_High)
                                    {
                                        float Dist = FVector::Distance(OtherDoor->GetActorLocation(), t->GetActorLocation());
                                        
                                        if (Dist < 1000.0f)
                                        {
                                            FCollisionQueryParams CollisionQueryParams;
                                            CollisionQueryParams.AddIgnoredActor(OtherDoor);

                                            bool bHasVisibility = !GetWorld()->LineTraceTestByChannel(ThreatLocation, OtherDoor->GetDoorMidLocation() + OtherDoor->GetActorForwardVector() * -100.0f, ECC_Visibility, CollisionQueryParams) || 
                                                                   !GetWorld()->LineTraceTestByChannel(ThreatLocation, OtherDoor->GetDoorMidLocation() + OtherDoor->GetActorForwardVector() * 100.0f, ECC_Visibility, CollisionQueryParams);
                                            
                                            //DrawDebugLine(GetWorld(), Location, Door->GetDoorMidLocation() + Door->GetActorForwardVector() * -100.0f, Hit.bBlockingHit ? FColor::Red : FColor::Green, false, 2.0f);

                                            //bool bHasVisibility = Internal_PointHasLineOfSightToDoor(ThreatLocation, OtherDoor, true) ||
                                            //                      Internal_PointHasLineOfSightToDoor(ThreatLocation, OtherDoor, false);
                                            
                                            if (bHasVisibility)
                                                t->SetThreatLevel(EThreatLevel::TL_High);
                                            else
                                                t->SetThreatLevel(EThreatLevel::TL_Medium);
                                        }
                                        else
                                        {
                                            t->SetThreatLevel(EThreatLevel::TL_Medium);
                                        }
                                    }
                                }
                                
                                for (AAISpawn* Spawner : AISpawners)
                                {
                                    float Dist = FVector::Distance(Spawner->GetActorLocation(), t->GetActorLocation());
                                    if (Dist < 500.0f)
                                    {
                                        t->SetThreatLevel(EThreatLevel::TL_High);
                                        break;
                                    }
                                }
                                
                                t->Tags.AddUnique("generated");

                                FString ThreatString = "None";
                                switch (t->GetThreatLevel())
                                {
                                    case EThreatLevel::TL_None:     ThreatString = "None"; break;
                                    case EThreatLevel::TL_Low:      ThreatString = "Low"; break;
                                    case EThreatLevel::TL_Medium:   ThreatString = "Medium"; break;
                                    case EThreatLevel::TL_High:     ThreatString = "High"; break;
                                    case EThreatLevel::TL_Extreme:  ThreatString = "Extreme"; break;
                                    case EThreatLevel::TL_Stairs:   ThreatString = "Stairs"; break;
                                }
                                
                                #if WITH_EDITOR
                                t->SetFolderPath("GeneratedThreatAwarenessActors");
                                t->SetActorLabel("TAA_" + ThreatString + "_" + FString::FromInt(ThreatIndex));
                                #else
                                //t->Rename(*FString("TAA_" + ThreatString + "_" + FString::FromInt(ThreatIndex)));
                                #endif
                                
                                ThreatIndex++;
                            }
                        }
                    }
                }
            }
        }
        
        GenerateDoorStackUpsV2(Door);
    }
    
    for (ADoor* Door : AllDoors)
    {
        bool bWasBlockedForGen = false;
        if (Door->IsDoorwayOnly() && Door->bNoNavBlockerForGen)
        {
            Door->ActivateDoorBlockerForWorldGen();
        }
        
        GenerateDoorClearPointsV2(Door);

        if (bWasBlockedForGen)
        {
            Door->DeactivateDoorBlocker();
        }
    }
    
    for (ADoor* Door : AllDoors)
    {
        // Generate door threat awareness actors (the extreme ones)
        {
            for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
            {
                if ((It->IsDoorThreat() && It->GetAttachedDoor() == Door) || (It->IsDoorThreat() && !It->GetAttachedDoor()))
                {
                    It->Destroy();
                }
            }

            FVector DoorLocation = Door->GetDoorMidLocation();
            DoorLocation.Z = Door->GetActorLocation().Z;
            
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.bNoFail = true;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            {
                FVector Loc = DoorLocation + Door->GetActorForwardVector() * 80.0f;
                Loc.Z += 50.0f;
                
                AThreatAwarenessActor* frontT = GetWorld()->SpawnActor<AThreatAwarenessActor>(AThreatAwarenessActor::StaticClass(), Loc, FRotator::ZeroRotator, SpawnParameters);
                frontT->CheckIsOutside();
                frontT->Tags.AddUnique("generated");
                frontT->DoorThreat = Door;
                frontT->bFrontDoorThreat = true;
                frontT->SetThreatLevel(EThreatLevel::TL_Extreme);
                Door->FrontThreat = frontT;
                
                #if WITH_EDITOR
                frontT->SetFolderPath("GeneratedDoorThreats");
                frontT->SetActorLabel("TAA_Extreme_" + FString::FromInt(ThreatIndex));
                #else
                //frontT->Rename(*FString("TAA_Extreme_" + FString::FromInt(ThreatIndex)));
                #endif

                ThreatIndex++;
            }

            {
                FVector Loc = DoorLocation + Door->GetActorForwardVector() * -80.0f;
                Loc.Z += 50.0f;

                AThreatAwarenessActor* backT = GetWorld()->SpawnActor<AThreatAwarenessActor>(AThreatAwarenessActor::StaticClass(), Loc, FRotator::ZeroRotator, SpawnParameters);
                backT->CheckIsOutside();
                backT->Tags.AddUnique("generated");
                backT->DoorThreat = Door;
                backT->bFrontDoorThreat = false;
                backT->SetThreatLevel(EThreatLevel::TL_Extreme);
                Door->BackThreat = backT;
                
                #if WITH_EDITOR
                backT->SetFolderPath("GeneratedDoorThreats");
                backT->SetActorLabel("TAA_Extreme_" + FString::FromInt(ThreatIndex));
                #else
                //backT->Rename(*FString("TAA_Extreme_" + FString::FromInt(ThreatIndex)));
                #endif
                
                ThreatIndex++;
            }
        }
    }
 
    for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
    {
        if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            FNavLocation NavLocation(It->GetActorLocation());
            if (NavSys->ProjectPointToNavigation(NavLocation.Location, NavLocation, FVector(50.0f, 50.0f, 50.0f)))
            {
                It->GetRootComponent()->SetMobility(EComponentMobility::Movable);
                
                FHitResult Hit;
                if (GetWorld()->LineTraceSingleByChannel(Hit, NavLocation.Location + FVector::UpVector * 50.0f, NavLocation.Location - FVector::UpVector * 100.0f, ECC_Visibility))
                {
                    It->SetActorLocation(Hit.Location + FVector::UpVector * 40.0f);
                }
                else
                {
                    It->SetActorLocation(NavLocation.Location + FVector::UpVector * 20.0f);
                }
                
                It->GetRootComponent()->SetMobility(EComponentMobility::Static);
            }
        }
    }

    // Link threat awareness actors
    {
        #if WITH_EDITOR
        FScopedSlowTask SlowTask(GetThreatAwarenessActorCount(),NSLOCTEXT("LinkingThreatAwarenessActors", "Linking Threat Awareness Actors", "Linking Threat Awareness Actors"));
        SlowTask.MakeDialog();
        #endif

        for (TActorIterator<AThreatAwarenessActor> taa(GetWorld()); taa; ++taa)
        {
            #if WITH_EDITOR
            SlowTask.EnterProgressFrame();
            #endif
            
            TArray<AThreatAwarenessActor*> PathableThreatAwarenessActors;
            PathableThreatAwarenessActors.Reserve(100);
            
            TArray<AThreatAwarenessActor*> VisibleThreatAwarenessActors;
            VisibleThreatAwarenessActors.Reserve(100);

            if (taa->PathableThreatAwarenessActors.Num() <= 1)
            {
                // Big optimization in wide open spaces
                GetPathableThreatAwarenessPoints(*taa, PathableThreatAwarenessActors);
                
                FVector Location = taa->GetActorLocation();
                PathableThreatAwarenessActors.Sort([Location](const AThreatAwarenessActor& Lhs, const AThreatAwarenessActor& Rhs)
                {
                    return (Lhs.GetActorLocation() - Location).Size() < (Rhs.GetActorLocation() - Location).Size();
                });
                            
                for (AThreatAwarenessActor* t : PathableThreatAwarenessActors)
                {
                    for (AThreatAwarenessActor* y : PathableThreatAwarenessActors)
                    {
                        t->PathableThreatAwarenessActors.AddUnique(y);
                    }
                }
                
                PathableThreatAwarenessActors.Remove(nullptr);
                taa->PathableThreatAwarenessActors = PathableThreatAwarenessActors;
            }

            #if WITH_EDITOR
            SlowTask.DefaultMessage = FText::FromString(FString::Format(TEXT("Linked {0} (Total Pathable: {1})"), {GetNameSafe(*taa),taa->PathableThreatAwarenessActors.Num() }));
            #endif
        }
    }
    
    SortAndTrimMaxVisibleThreats({});
    
    // generate swat look at points
    {
        #if WITH_EDITOR
        FScopedSlowTask SlowTask(GetThreatAwarenessActorCount(),NSLOCTEXT("GeneratingSwatLookAtPoitns", "Generating Swat Look At Points", "Generating Swat Look At Points"));
        SlowTask.MakeDialog();
        #endif
 
        for (TActorIterator<AThreatAwarenessActor> A(GetWorld()); A; ++A)
        {
            #if WITH_EDITOR
            SlowTask.EnterProgressFrame();
            #endif
            
            for (ADoor* Door : AllDoors)
            {
                //if (Door->GetSubDoor() && Door->IsNonMainSubdoor())
                    //continue;

                FVector DoorLocation = Door->GetDoorMidLocation();
                DoorLocation.Z = Door->GetActorLocation().Z;
                    
                if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
                {
                    FCollisionQueryParams CollisionQueryParams;
                    CollisionQueryParams.AddIgnoredActor(Door);
                    CollisionQueryParams.AddIgnoredActor(*A);
                    
                    FHitResult Hit, ReverseHit;
                    GetWorld()->LineTraceSingleByChannel(Hit, A->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f), Door->GetDoorway()->GetComponentLocation(), ECC_Visibility, CollisionQueryParams);
                    GetWorld()->LineTraceSingleByChannel(ReverseHit, Door->GetDoorway()->GetComponentLocation(), A->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f), ECC_Visibility, CollisionQueryParams);
                    
                    if (Hit.bBlockingHit || ReverseHit.bBlockingHit)
                    {
                        FPathFindingQuery PathFindingQuery;
                        FNavLocation FrontNavLocation(DoorLocation + Door->GetActorForwardVector() * 50.0f);
            
                        if (NavSys->ProjectPointToNavigation(FrontNavLocation.Location, FrontNavLocation, FVector(100.0f, 100.0f, 200.0f)))
                        {
                            PathFindingQuery.StartLocation = A->GetActorLocation();
                            PathFindingQuery.EndLocation = FrontNavLocation.Location;
                            PathFindingQuery.SetAllowPartialPaths(false);
                            
                            const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_Swat::StaticClass();
                            const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
                            PathFindingQuery.QueryFilter = QueryFilter;
                            
                            const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Hierarchical);
                            if (PathFindingResult.Result == ENavigationQueryResult::Success)
                            {
                                if (PathFindingResult.Path->GetPathPoints().Num() > 2)
                                {
                                    FLookAtPoint LookAtPoint;
                                    LookAtPoint.Location = FIntVector(PathFindingResult.Path->GetPathPoints()[2].Location + FVector(0.0f, 0.0f, 120.0f));
                                    LookAtPoint.LinkedDoor = nullptr;
                                    
                                    A->SwatLookAtPoints.AddUnique(LookAtPoint);
                                }
                                else
                                {
                                    FLookAtPoint LookAtPoint;
                                    LookAtPoint.Location = FIntVector(PathFindingResult.Path->GetEndLocation() + FVector(0.0f, 0.0f, 120.0f));
                                    LookAtPoint.LinkedDoor = nullptr;
                                    
                                    A->SwatLookAtPoints.AddUnique(LookAtPoint);
                                }
                                
                                //for (int32 i = 0; i < PathFindingResult.Path->GetPathPoints().Num(); i++)
                                //{
                                //    FNavPathPoint Pt = PathFindingResult.Path->GetPathPoints()[i];
                                //    DrawDebugString(GetWorld(), Pt.Location  + FVector(0.0f, 0.0f, 200.0f),  "PathPt: " +  FString::FromInt(i), nullptr, FColor::White, 5.0f);
                                //}
                            }
                        }
                        
                        FNavLocation BackNavLocation(DoorLocation + Door->GetActorForwardVector() * -50.0f);
                        if (NavSys->ProjectPointToNavigation(BackNavLocation.Location, BackNavLocation, FVector(100.0f, 100.0f, 200.0f)))
                        {
                            PathFindingQuery.StartLocation = A->GetActorLocation();
                            PathFindingQuery.EndLocation = BackNavLocation.Location;
                            PathFindingQuery.SetAllowPartialPaths(false);
                            
                            const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_Swat::StaticClass();
                            const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
                            PathFindingQuery.QueryFilter = QueryFilter;
                            
                            const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Hierarchical);
                            if (PathFindingResult.Result == ENavigationQueryResult::Success)
                            {
                                if (PathFindingResult.Path->GetPathPoints().Num() > 2)
                                {
                                    FLookAtPoint LookAtPoint;
                                    LookAtPoint.Location = FIntVector(PathFindingResult.Path->GetPathPoints()[2].Location + FVector(0.0f, 0.0f, 120.0f));
                                    LookAtPoint.LinkedDoor = nullptr;
                                    
                                    A->SwatLookAtPoints.AddUnique(LookAtPoint);
                                }
                                else
                                {
                                    FLookAtPoint LookAtPoint;
                                    LookAtPoint.Location = FIntVector(PathFindingResult.Path->GetEndLocation() + FVector(0.0f, 0.0f, 120.0f));
                                    LookAtPoint.LinkedDoor = nullptr;
                                    
                                    A->SwatLookAtPoints.AddUnique(LookAtPoint);
                                }
                                
                                //for (int32 i = 0; i < PathFindingResult.Path->GetPathPoints().Num(); i++)
                                //{
                                //    FNavPathPoint Pt = PathFindingResult.Path->GetPathPoints()[i];
                                //    DrawDebugString(GetWorld(), Pt.Location  + FVector(0.0f, 0.0f, 200.0f),  "PathPt: " +  FString::FromInt(i), nullptr, FColor::White, 5.0f);
                                //}
                            }
                        }
                    }
                    else
                    {
                        FLookAtPoint LookAtPoint;
                        
                        if (Door->GetSubDoor())
                            LookAtPoint.Location = FIntVector(Door->GetDoorMidLocation() + Door->GetActorRightVector() * 70.0f);
                        else
                            LookAtPoint.Location = FIntVector(Door->GetDoorMidLocation());

                        LookAtPoint.Location += FIntVector(0.0f, 0.0f, 30.0f);
                        
                        LookAtPoint.LinkedDoor = Door;
                        
                        bool bAnyNearby = false;
                        for (FLookAtPoint& Point : A->SwatLookAtPoints)
                        {
                            if (FVector::Dist(FVector(Point.Location), FVector(LookAtPoint.Location)) < 50.0f)
                            {
                                bAnyNearby = true;
                                break;
                            }
                        }
                        
                        if (!bAnyNearby)
                            A->SwatLookAtPoints.AddUnique(LookAtPoint);
                    }
                }
            }
        }
    }
    
    RemoveAllOverlappingThreats({});
    
    PlacePatrolPointsOnAllThreats({});

    //CalculateAllExits();
    {
        #if WITH_EDITOR
        FScopedSlowTask SlowTask(GetThreatAwarenessActorCount() * 3, FText::FromString("Generating Exit Data"));
        SlowTask.MakeDialog();
        #endif
        
        IgnoredExitThreats.Empty();
        GetOrCreateIgnoredExitThreats(IgnoredExitThreats);
        
        for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
        {
            It->EnableNavLink();
        }

        for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
        {
            #if WITH_EDITOR
            SlowTask.EnterProgressFrame();
            #endif
            
            It->CalculateExits();
        }
        
        for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
        {
            #if WITH_EDITOR
            SlowTask.EnterProgressFrame();
            #endif
            
            It->RemoveAnyVisibleExits();
        }
       
        for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
        {
            #if WITH_EDITOR
            SlowTask.EnterProgressFrame();
            #endif
            
            It->GenerateUniqueExits();
            It->GeneratePossibleRoutes();
        }
        
        for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
        {
            It->DestroyNavLink();
        }
    }
    
    GenerateRooms();

    for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
    {
        It->TryDestroyIfInvalid();
    }

    for (ADoor* Door : AllDoors)
    {
        ensureAlways(Door->FrontThreat != nullptr);
        ensureAlways(Door->BackThreat != nullptr);
    }
    
    for (ADoor* Door : AllDoors)
    {
        if (!Door->GetSubDoor() || (Door->GetSubDoor() && Door->IsMainSubdoor()))
        {
            if (Door->BackRoomPosition != EDoorRoomPosition::Center && Door->BackRoomPosition != EDoorRoomPosition::Hallway)
            {
                if (Door->FrontLeftStackUpPoints.Num() == 0 && Door->FrontRightStackUpPoints.Num() == 0)
                {
                    Door->bCanIssueOrdersOnFrontSide = false;
                }
                
                if (Door->FrontLeftStackUpPoints.Num() == 0 && Door->FrontRightStackUpPoints.Num() < 3)
                {
                    Door->bCanIssueOrdersOnFrontSide = false;
                }
                
                if (Door->FrontLeftStackUpPoints.Num() < 3 && Door->FrontRightStackUpPoints.Num() == 0)
                {
                    Door->bCanIssueOrdersOnFrontSide = false;
                }
            }
                
            if (Door->FrontRoomPosition != EDoorRoomPosition::Center && Door->FrontRoomPosition != EDoorRoomPosition::Hallway)
            {
                if (Door->BackLeftStackUpPoints.Num() == 0 && Door->BackRightStackUpPoints.Num() == 0)
                {
                    Door->bCanIssueOrdersOnBackSide = false;
                }
                
                if (Door->BackLeftStackUpPoints.Num() == 0 && Door->BackRightStackUpPoints.Num() < 3)
                {
                    Door->bCanIssueOrdersOnBackSide = false;
                }
                
                if (Door->BackLeftStackUpPoints.Num() < 3 && Door->BackRightStackUpPoints.Num() == 0)
                {
                    Door->bCanIssueOrdersOnBackSide = false;
                }
            }
        }
    }

    uint16 StackUpFailures = 0;
    uint16 TotalPotentialStackUps = 0;
    for (ADoor* Door : AllDoors)
    {
        uint16 FailuresThisDoor = 0;
        if (!Door->GetSubDoor() || (Door->GetSubDoor() && Door->IsMainSubdoor()))
        {
            if (Door->FrontRoomPosition == EDoorRoomPosition::Center || Door->FrontRoomPosition == EDoorRoomPosition::Hallway)
            {
                TotalPotentialStackUps += 8;
                uint8 Num = 8-(Door->BackLeftStackUpPoints.Num() + Door->BackRightStackUpPoints.Num());
                FailuresThisDoor += Num;
                StackUpFailures += Num;
            }
            
            if (Door->BackRoomPosition == EDoorRoomPosition::Center || Door->BackRoomPosition == EDoorRoomPosition::Hallway)
            {
                TotalPotentialStackUps += 8;
                uint8 Num = 8-(Door->FrontLeftStackUpPoints.Num() + Door->FrontRightStackUpPoints.Num());
                FailuresThisDoor += Num;
                StackUpFailures += Num;
            }

            if (Door->FrontRoomPosition == EDoorRoomPosition::CornerLeft || Door->FrontRoomPosition == EDoorRoomPosition::CornerRight ||
                Door->FrontRoomPosition == EDoorRoomPosition::HallwayLeft || Door->FrontRoomPosition == EDoorRoomPosition::HallwayRight)
            {
                TotalPotentialStackUps += 4;
                if (Door->FrontRoomPosition == EDoorRoomPosition::CornerLeft || Door->FrontRoomPosition == EDoorRoomPosition::HallwayLeft)
                {
                    uint8 Num = 4-Door->BackLeftStackUpPoints.Num();
                    FailuresThisDoor += Num;
                    StackUpFailures += Num;
                }
                else if (Door->FrontRoomPosition == EDoorRoomPosition::CornerRight || Door->FrontRoomPosition == EDoorRoomPosition::HallwayRight)
                {
                    uint8 Num = 4-Door->BackRightStackUpPoints.Num();
                    FailuresThisDoor += Num;
                    StackUpFailures += Num;
                }
            }
            
            if (Door->BackRoomPosition == EDoorRoomPosition::CornerLeft || Door->BackRoomPosition == EDoorRoomPosition::CornerRight ||
                Door->BackRoomPosition == EDoorRoomPosition::HallwayLeft || Door->BackRoomPosition == EDoorRoomPosition::HallwayRight)
            {
                TotalPotentialStackUps += 4;
                if (Door->BackRoomPosition == EDoorRoomPosition::CornerLeft || Door->BackRoomPosition == EDoorRoomPosition::HallwayLeft)
                {
                    uint8 Num = 4-Door->FrontRightStackUpPoints.Num();
                    FailuresThisDoor += Num;
                    StackUpFailures += Num;
                }
                else if (Door->BackRoomPosition == EDoorRoomPosition::CornerRight || Door->BackRoomPosition == EDoorRoomPosition::HallwayRight)
                {
                    uint8 Num = 4-Door->FrontLeftStackUpPoints.Num();
                    FailuresThisDoor += Num;
                    StackUpFailures += Num;
                }
            }
        }
        ULog::Info(Door->GetName() + ": " + FString::FromInt(FailuresThisDoor));
    }

    if (TotalPotentialStackUps > 0)
    {
        ULog::Info(FString::Printf(TEXT("[World Gen] Total potential stack ups: %u"), TotalPotentialStackUps), LO_Both);
        ULog::Info(FString::Printf(TEXT("[World Gen] Total stack ups failures: %u"), StackUpFailures), LO_Both);
        ULog::Info(FString::Printf(TEXT("[World Gen] Stack up gen success rate: %.2f%s"), ((float)(TotalPotentialStackUps-StackUpFailures)/(float)TotalPotentialStackUps)*100.0f, "%"), LO_Both);
    }
    
    UnblockAllDoorways();

    // Generate cover
    if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        if (AReadyOrNotRecastNavMesh* NavMesh = Cast<AReadyOrNotRecastNavMesh>(NavSys->MainNavData))
        {
            NavMesh->GenerateCoverPoints();
        }
    }
    
    ClearNullReferences();
    
    if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
    {
        GS->RoomData = &RoomData;
    }

    if (UThreatAwarenessSubsystem* Subsystem = GetWorld()->GetSubsystem<UThreatAwarenessSubsystem>())
    {
        Subsystem->OnWorldGenerated();
    }

    bHasWorldEverBeenGenerated = true;
    
    bIsGenerating = false;
}

void AWorldDataGenerator::GenerateRooms()
{
    //bDoorwaysBlocked = false;
    BlockAllDoorways();
    
    RoomIndex = 0;
    VisitedDoors.Empty();
    RoomData.Rooms.Empty();

    for (TActorIterator<ARoomVisualizer> It(GetWorld()); It; ++It)
    {
        It->Destroy();
    }

    TArray<FRoom> FrontRooms;
    for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
    {
        ADoor* Door = *It;
        
        RoomIndex++;

        FVector DoorLocation = Door->GetDoorMidLocation();
        DoorLocation.Z = Door->GetActorLocation().Z;
        
        FVector Start = DoorLocation + Door->GetActorForwardVector() * 100.0f;
        
        FRoom Room;
        Room.Name = FName("Room_" + FString::FromInt(RoomIndex));
        Room.Location = FIntVector(Start);
        Room.RootDoor = Door;

        for (TActorIterator<AThreatAwarenessActor> OtherIt(GetWorld()); OtherIt; ++OtherIt)
        {
            AThreatAwarenessActor* TAA = *OtherIt;

            FVector End = TAA->GetActorLocation() + FVector::UpVector * 20.0f;
            FVector StartProjected = Start;
            FVector EndProjected = End;
            Internal_ProjectPointToNav(Start, StartProjected, FVector(40.0f, 40.0f, 200.0f));
            Internal_ProjectPointToNav(End, EndProjected, FVector(40.0f, 40.0f, 200.0f));
            if (Internal_FindPath_RoomGen(StartProjected, EndProjected))
            {
                //DrawDebugPoint(GetWorld(), TAA->GetActorLocation(), 20.0f, FColor::Orange, false, 10.0f);
                Room.Threats.AddUnique(TAA);
            }
        }

        FrontRooms.Add(Room);
    }

    // find duplicates
    {
        TArray<FName> DuplicateRooms;

        for (FRoom& Room : FrontRooms)
        {
            if (DuplicateRooms.Contains(Room.Name))
                continue;
            
            for (FRoom& OtherRoom : FrontRooms)
            {
                if (DuplicateRooms.Contains(OtherRoom.Name))
                    continue;
                
                if (OtherRoom.Name != Room.Name)
                {
                    if (OtherRoom.Threats.Num() == Room.Threats.Num())
                    {
                        bool bAllEqual = true;
                        for (uint16 i = 0; i < OtherRoom.Threats.Num(); i++)
                        {
                            if (!OtherRoom.Threats.Contains(Room.Threats[i]))
                            {
                                bAllEqual = false;
                                break;
                            }
                        }

                        if (bAllEqual)
                        {
                            DuplicateRooms.Add(OtherRoom.Name);
                        }
                    }
                }
            }
        }

        FrontRooms.RemoveAll([&](const FRoom& Room)
        {
            return DuplicateRooms.Contains(Room.Name);
        });
    }
    
    TArray<FRoom> BackRooms;
    for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
    {
        ADoor* Door = *It;
        
        RoomIndex++;
        
        FVector DoorLocation = Door->GetDoorMidLocation();
        DoorLocation.Z = Door->GetActorLocation().Z;
        
        FVector Start = DoorLocation + Door->GetActorForwardVector() * -100.0f;
        
        FRoom Room;
        Room.Name = FName("Room_" + FString::FromInt(RoomIndex));
        Room.Location = FIntVector(Start);
        Room.RootDoor = Door;
        
        for (TActorIterator<AThreatAwarenessActor> OtherIt(GetWorld()); OtherIt; ++OtherIt)
        {
            AThreatAwarenessActor* TAA = *OtherIt;
            
            FVector End = TAA->GetActorLocation() + FVector::UpVector * 20.0f;
            FVector StartProjected = Start;
            FVector EndProjected = End;
            Internal_ProjectPointToNav(Start, StartProjected, FVector(40.0f, 40.0f, 200.0f));
            Internal_ProjectPointToNav(End, EndProjected, FVector(40.0f, 40.0f, 200.0f));
            if (Internal_FindPath_RoomGen(StartProjected, EndProjected))
            {
                Room.Threats.AddUnique(TAA);
            }
        }

        BackRooms.Add(Room);
    }

    // find duplicates
    {
        TArray<FName> DuplicateRooms;

        for (FRoom& Room : BackRooms)
        {
            if (DuplicateRooms.Contains(Room.Name))
                continue;
            
            for (FRoom& OtherRoom : BackRooms)
            {
                if (DuplicateRooms.Contains(OtherRoom.Name))
                    continue;
            
                if (OtherRoom.Name != Room.Name)
                {
                    if (OtherRoom.Threats.Num() == Room.Threats.Num())
                    {
                        bool bAllEqual = true;
                        for (uint16 i = 0; i < OtherRoom.Threats.Num(); i++)
                        {
                            if (!OtherRoom.Threats.Contains(Room.Threats[i]))
                            {
                                bAllEqual = false;
                                break;
                            }
                        }

                        if (bAllEqual)
                        {
                            DuplicateRooms.Add(OtherRoom.Name);
                        }
                    }
                }
            }
        }

        BackRooms.RemoveAll([&](const FRoom& Room)
        {
            return DuplicateRooms.Contains(Room.Name);
        });
    }

    TArray<FRoom> FinalRooms;

    // find duplicates
    {
        TArray<FName> DuplicateRooms;

        for (FRoom& Room : FrontRooms)
        {
            if (DuplicateRooms.Contains(Room.Name))
                continue;
            
            for (FRoom& OtherRoom : BackRooms)
            {
                if (DuplicateRooms.Contains(OtherRoom.Name))
                    continue;
            
                if (OtherRoom.Name != Room.Name)
                {
                    if (OtherRoom.Threats.Num() == Room.Threats.Num())
                    {
                        bool bAllEqual = true;
                        for (uint16 i = 0; i < OtherRoom.Threats.Num(); i++)
                        {
                            if (!OtherRoom.Threats.Contains(Room.Threats[i]))
                            {
                                bAllEqual = false;
                                break;
                            }
                        }

                        if (bAllEqual)
                        {
                            DuplicateRooms.Add(OtherRoom.Name);
                        }
                    }
                }
            }
        }

        BackRooms.RemoveAll([&](const FRoom& Room)
        {
            return DuplicateRooms.Contains(Room.Name);
        });
    }
    
    FinalRooms = FrontRooms;
    FinalRooms += BackRooms;
    
    int32 i = 0;
    for (FRoom& Room : FinalRooms)
    {
        Room.Name = FName("Room_" + FString::FromInt(i));
        
        for (AThreatAwarenessActor* TAA : Room.Threats)
        {
            TAA->OwningRoom = Room.Name;
        }
        
        i++;
    }

    // another pass to find multiple root doors for a room
    {
        for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
        {
            AThreatAwarenessActor* TAA = *It;

            if (TAA->GetAttachedDoor())
            {
                if (FRoom* FoundRoom = FinalRooms.FindByPredicate([&](const FRoom& Room)
                {
                    return Room.Name == TAA->OwningRoom;
                }))
                {
                    FoundRoom->AdditionalRootDoors.AddUnique(TAA->GetAttachedDoor());
                }
            }
        }
    }

    for (FRoom& Room : FinalRooms)
    {
        for (ADoor* Door : Room.AdditionalRootDoors)
        {
            if (Door)
            {
                if (Door->FrontThreat->OwningRoom != Room.Name)
                {
                    Room.ConnectingRooms.AddUnique(Door->FrontThreat->OwningRoom);
                }
                else if (Door->BackThreat->OwningRoom != Room.Name)
                {
                    Room.ConnectingRooms.AddUnique(Door->BackThreat->OwningRoom);
                }
            }
        }
    }

    RoomData.Rooms = FinalRooms;
    
    for (FRoom& Room : RoomData.Rooms)
    {
        DrawDebugBox(GetWorld(), FVector(Room.Location), FVector(15.0f), FColor::Yellow, false, 10.0f);

        FVector AverageThreatLocation = FVector(Room.Location);
        FVector Sum = FVector::ZeroVector;
        uint8 NumExtreme = 0;
        uint8 NumNonExtreme = 0;
        if (Room.Threats.Num() > 0)
        {
            for (AThreatAwarenessActor* TAA : Room.Threats)
            {
                Sum += TAA->GetActorLocation();
            }

            AverageThreatLocation = Sum/Room.Threats.Num();

            for (AThreatAwarenessActor* TAA : Room.Threats)
            {
                if (TAA->GetThreatLevel() == EThreatLevel::TL_Extreme)
                    NumExtreme++;
                else
                    NumNonExtreme++;
            }
        }
        
        if (ARoomVisualizer* RoomVisualizer = GetWorld()->SpawnActor<ARoomVisualizer>(ARoomVisualizer::StaticClass(), AverageThreatLocation + FVector::UpVector * 100.0f, FRotator::ZeroRotator))
        {
            if (NumExtreme > NumNonExtreme)
            {
                RoomVisualizer->SetRoomSize(ERoomSize::Corridor);
            }
            else
            {
                if (NumNonExtreme <= 10)
                {
                    RoomVisualizer->SetRoomSize(ERoomSize::Small);
                }
                else if (NumNonExtreme <= 20)
                {
                    RoomVisualizer->SetRoomSize(ERoomSize::Medium);
                }
                else
                {
                    RoomVisualizer->SetRoomSize(ERoomSize::Large);
                }
            }
            
            RoomVisualizer->OwningRoom = Room.Name;

            #if WITH_EDITOR
            FString NewName = Room.Name.ToString() + " (" + RON_ENUM_TO_STRING(ERoomSize, RoomVisualizer->GetRoomSize()) + ")";
            RoomVisualizer->SetActorLabel(NewName);
            RoomVisualizer->SetFolderPath("GeneratedRooms");
            #endif
        }
    }
}

void AWorldDataGenerator::GenerateWebbedBreachPoints()
{
    FScopedSlowTask SlowTask(GetThreatAwarenessActorCount(),NSLOCTEXT("LinkingThreatAwarenessActors", "Linking Threat Awareness Actors", "Linking Threat Awareness Actors"));
    SlowTask.MakeDialog();

    BlockAllDoorways();
    
    for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
    {
        SlowTask.EnterProgressFrame();
        AThreatAwarenessActor* taa = *It;
        TArray<AThreatAwarenessActor*> PathableThreatAwarenessActors = {};
        TArray<AThreatAwarenessActor*> VisibleThreatAwarenessActors = {};

        if (taa->PathableThreatAwarenessActors.Num() <= 1)
        {
            // Big optimization in wide open spaces
            GetPathableThreatAwarenessPoints(taa, PathableThreatAwarenessActors);
            FVector Location = taa->GetActorLocation();
            PathableThreatAwarenessActors.Sort([Location](const AThreatAwarenessActor& Lhs, const AThreatAwarenessActor& Rhs)
            {
                return (Lhs.GetActorLocation() - Location).Size() < (Rhs.GetActorLocation() - Location).Size();
            });
                        
            for (AThreatAwarenessActor* t : PathableThreatAwarenessActors)
            {
                for (AThreatAwarenessActor* y : PathableThreatAwarenessActors)
                {
                    t->PathableThreatAwarenessActors.AddUnique(y);
                }

            }
            PathableThreatAwarenessActors.Remove(nullptr);
            taa->PathableThreatAwarenessActors = PathableThreatAwarenessActors;
        }
        
        SlowTask.DefaultMessage = FText::FromString(FString::Format(TEXT("Linked {0} (Total Pathable: {1})"), {GetNameSafe(taa),taa->PathableThreatAwarenessActors.Num() }));
    }
}

void AWorldDataGenerator::GetAllReachableDoors(FVector Location, TArray<ADoor*>& OutDoors)
{
    for (TActorIterator<ADoor>It(GetWorld());It; ++It)
    {
        ADoor* Door = *It;

        FVector DoorLocation = Door->GetDoorMidLocation();
        DoorLocation.Z = Door->GetActorLocation().Z;
        
        if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            FPathFindingQuery PathFindingQuery;
            FNavLocation FrontNavLocation(DoorLocation);
    
            if (NavSys->ProjectPointToNavigation(FrontNavLocation.Location, FrontNavLocation, FVector(100.0f, 100.0f, 200.0f)))
            {
                PathFindingQuery.StartLocation = FrontNavLocation.Location;
                PathFindingQuery.EndLocation = Location;
                PathFindingQuery.SetAllowPartialPaths(false);
                const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_Swat::StaticClass();
                const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
                PathFindingQuery.QueryFilter = QueryFilter;
                const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Hierarchical);
                if (PathFindingResult.Result == ENavigationQueryResult::Success)
                {
                    OutDoors.AddUnique(Door);
                }
            }
            
            FNavLocation BackNavLocation(DoorLocation);
            if (NavSys->ProjectPointToNavigation(BackNavLocation.Location, BackNavLocation, FVector(100.0f, 100.0f, 200.0f)))
            {
                PathFindingQuery.StartLocation = BackNavLocation.Location;
                PathFindingQuery.EndLocation = Location;
                PathFindingQuery.SetAllowPartialPaths(false);
                const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_Swat::StaticClass();
                const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
                PathFindingQuery.QueryFilter = QueryFilter;
                const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Hierarchical);
                if (PathFindingResult.Result == ENavigationQueryResult::Success)
                {
                    OutDoors.AddUnique(Door);
                }
            }          
        }
    }
}

void AWorldDataGenerator::GenerateSpawnHidingSpots()
{    
    // for (TActorIterator<AAISpawn>It(GetWorld()); It; ++It)
    // {
    //     AAISpawn* Spawner = *It;
    //     Spawner->GenerateHidingSpots();
    //     if (Spawner->SpawnData.HidingSpots.Num() == 0)
    //     {
    //         V_LOGM(LogReadyOrNot, "%s has no hiding spots generated!", *Spawner->GetName());
    //     }
    // }    
    // SaveAllScenarios();
}

void AWorldDataGenerator::GetPathableThreatAwarenessPoints(AThreatAwarenessActor* Threat, TArray<AThreatAwarenessActor*>& PathableThreatAwarenessActors)
{
    if (!Threat)
        return;

    for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
    {
        AThreatAwarenessActor* taa = *It;
        if (taa == Threat)
            continue;

        //if (taa->GetThreatLevel() == EThreatLevel::TL_Extreme)
            //continue;

        if (PathableThreatAwarenessActors.Contains(taa))
            continue;

        if (taa->PathableThreatAwarenessActors.Contains(Threat))
        {
            PathableThreatAwarenessActors.AddUnique(taa);
            continue;
        }

        //if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            //FPathFindingQuery PathFindingQuery;
            //FNavLocation NavLocation(Threat->GetActorLocation());
            FVector NavLocation = Threat->GetActorLocation();
            if (Internal_ProjectPointToNav(Threat->GetActorLocation(), NavLocation))
            //if (NavSys->ProjectPointToNavigation(Threat->GetActorLocation(), NavLocation, FVector(100.0f, 100.0f, 200.0f)))
            {
                //FNavLocation TaaNavLocation(taa->GetActorLocation());
                //NavSys->ProjectPointToNavigation(taa->GetActorLocation(), TaaNavLocation, FVector(100.0f, 100.0f, 200.0f));
                
                FVector TaaNavLocation = taa->GetActorLocation();
                Internal_ProjectPointToNav(taa->GetActorLocation(), TaaNavLocation);

                /*
                PathFindingQuery.StartLocation = NavLocation.Location;
                PathFindingQuery.EndLocation = TaaNavLocation;
                PathFindingQuery.SetAllowPartialPaths(false);
                
                const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_Swat::StaticClass();
                const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
                PathFindingQuery.QueryFilter = QueryFilter;
                
                const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Hierarchical);
                if (PathFindingResult.Result == ENavigationQueryResult::Success)
                {
                    PathableThreatAwarenessActors.AddUnique(taa);
                }
                */

                if (Internal_FindPath(NavLocation, TaaNavLocation, 99999))
                {
                    PathableThreatAwarenessActors.AddUnique(taa);
                }
            }
            else
            {
                if (!Threat->IsDoorThreat() && Threat->GetThreatLevel() != EThreatLevel::TL_Stairs)
                {
                    Threat->Destroy();
                    V_LOGM(LogReadyOrNot, "Unable to project %s to navmesh!", *Threat->GetName());
                    break;
                }
            }
        }
    }    
}

void AWorldDataGenerator::GetVisibleThreatAwarenessPoints(AThreatAwarenessActor* Threat, TArray<AThreatAwarenessActor*>& VisibleThreatAwarenessActors)
{
    if (!Threat)
        return;
    
    for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
    {
        AThreatAwarenessActor* taa = *It;
        if (taa == Threat)
            continue;

        if (VisibleThreatAwarenessActors.Contains(taa))
            continue;

        FHitResult Hit, HitReverse;
        
        FCollisionQueryParams CollisionQueryParams;
        CollisionQueryParams.bTraceComplex = true;
        
        GetWorld()->LineTraceSingleByChannel(Hit, Threat->GetActorLocation(), taa->GetActorLocation(), ECollisionChannel::ECC_Visibility, CollisionQueryParams);
        GetWorld()->LineTraceSingleByChannel(HitReverse, taa->GetActorLocation(), Threat->GetActorLocation(), ECollisionChannel::ECC_Visibility, CollisionQueryParams);
        
        if (!Hit.bBlockingHit && !HitReverse.bBlockingHit)
        {
            VisibleThreatAwarenessActors.AddUnique(taa);
        }
    }  
}

void AWorldDataGenerator::GenerateSwatLookAtPointsForEachThreat()
{
    FScopedSlowTask SlowTask(GetThreatAwarenessActorCount(),NSLOCTEXT("GeneratingSwatLookAtPoitns", "Generating Swat Look At Points", "Generating Swat Look At Points"));
    SlowTask.MakeDialog();
    
    for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
    {
        SlowTask.EnterProgressFrame();
        GenerateSwatLookAtPoint(*It);
    }   
}

void AWorldDataGenerator::GenerateSwatLookAtPoint(AThreatAwarenessActor* A)
{
    if (!A)
        return;
    
    A->SwatLookAtPoints.Empty();
    
    for (TActorIterator<ADoor>DoorIt(GetWorld());DoorIt; ++DoorIt)
    {
        ADoor* Door = *DoorIt;
        //if (Door->GetSubDoor() && Door->IsNonMainSubdoor())
        //    continue;
        
        FVector DoorLocation = Door->GetDoorMidLocation();
        DoorLocation.Z = Door->GetActorLocation().Z;
            
        if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            FCollisionQueryParams CollisionQueryParams;
            CollisionQueryParams.AddIgnoredActor(Door);
            CollisionQueryParams.AddIgnoredActor(A);
            FHitResult Hit, ReverseHit;
            GetWorld()->LineTraceSingleByChannel(Hit, A->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f), Door->GetDoorway()->GetComponentLocation(), ECC_Visibility, CollisionQueryParams);
            GetWorld()->LineTraceSingleByChannel(ReverseHit, Door->GetDoorway()->GetComponentLocation(), A->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f), ECC_Visibility, CollisionQueryParams);
            if (Hit.bBlockingHit || ReverseHit.bBlockingHit)
            {
                FPathFindingQuery PathFindingQuery;
                FNavLocation FrontNavLocation(DoorLocation + Door->GetActorForwardVector() * 50.0f);
    
                if (NavSys->ProjectPointToNavigation(FrontNavLocation.Location, FrontNavLocation, FVector(100.0f, 100.0f, 200.0f)))
                {
                    PathFindingQuery.StartLocation = A->GetActorLocation();
                    PathFindingQuery.EndLocation = FrontNavLocation.Location;
                    PathFindingQuery.SetAllowPartialPaths(false);
                    const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_Swat::StaticClass();
                    const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
                    PathFindingQuery.QueryFilter = QueryFilter;
                    const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Hierarchical);
                    if (PathFindingResult.Result == ENavigationQueryResult::Success)
                    {
                        if (PathFindingResult.Path->GetPathPoints().Num() > 2)
                        {
                            FLookAtPoint LookAtPoint;
                            LookAtPoint.Location = FIntVector(PathFindingResult.Path->GetPathPoints()[2].Location + FVector(0.0f, 0.0f, 120.0f));
                            LookAtPoint.LinkedDoor = nullptr;
                            A->SwatLookAtPoints.AddUnique(LookAtPoint);
                        }
                        else
                        {
                            FLookAtPoint LookAtPoint;
                            LookAtPoint.Location = FIntVector(PathFindingResult.Path->GetEndLocation() + FVector(0.0f, 0.0f, 120.0f));
                            LookAtPoint.LinkedDoor = nullptr;
                            A->SwatLookAtPoints.AddUnique(LookAtPoint);
                        }
                        
                        for (int32 i = 0; i < PathFindingResult.Path->GetPathPoints().Num(); i++)
                        {
                            FNavPathPoint Pt = PathFindingResult.Path->GetPathPoints()[i];
                            DrawDebugString(GetWorld(), Pt.Location  + FVector(0.0f, 0.0f, 200.0f),  "PathPt: " +  FString::FromInt(i), nullptr, FColor::White, 5.0f);
                        }
                    }
                }
                
                FNavLocation BackNavLocation(DoorLocation + Door->GetActorForwardVector() * -50.0f);
                if (NavSys->ProjectPointToNavigation(BackNavLocation.Location, BackNavLocation, FVector(100.0f, 100.0f, 200.0f)))
                {
                    PathFindingQuery.StartLocation = A->GetActorLocation();
                    PathFindingQuery.EndLocation = BackNavLocation.Location;
                    PathFindingQuery.SetAllowPartialPaths(false);
                    const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_Swat::StaticClass();
                    const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
                    PathFindingQuery.QueryFilter = QueryFilter;
                    const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Hierarchical);
                    if (PathFindingResult.Result == ENavigationQueryResult::Success)
                    {
                        if (PathFindingResult.Path->GetPathPoints().Num() > 2)
                        {
                            FLookAtPoint LookAtPoint;
                            LookAtPoint.Location = FIntVector(PathFindingResult.Path->GetPathPoints()[2].Location + FVector(0.0f, 0.0f, 120.0f));
                            LookAtPoint.LinkedDoor = nullptr;
                            A->SwatLookAtPoints.AddUnique(LookAtPoint);
                        }
                        else
                        {
                            FLookAtPoint LookAtPoint;
                            LookAtPoint.Location = FIntVector(PathFindingResult.Path->GetEndLocation() + FVector(0.0f, 0.0f, 120.0f));
                            LookAtPoint.LinkedDoor = nullptr;
                            A->SwatLookAtPoints.AddUnique(LookAtPoint);
                        }
                        
                        for (int32 i = 0; i < PathFindingResult.Path->GetPathPoints().Num(); i++)
                        {
                            FNavPathPoint Pt = PathFindingResult.Path->GetPathPoints()[i];
                            DrawDebugString(GetWorld(), Pt.Location + FVector(0.0f, 0.0f, 200.0f),  "PathPt: " + FString::FromInt(i), nullptr, FColor::White, 5.0f);
                        }
                    }
                }
            }
            else
            {
                FLookAtPoint LookAtPoint;
                
                if (Door->GetSubDoor())
                    LookAtPoint.Location = FIntVector(Door->GetDoorMidLocation() + Door->GetActorRightVector() * 70.0f);
                else
                    LookAtPoint.Location = FIntVector(Door->GetDoorMidLocation());
                
                LookAtPoint.Location += FIntVector(0.0f, 0.0f, 30.0f);
                
                LookAtPoint.LinkedDoor = Door;

                bool bAnyNearby = false;
                for (FLookAtPoint& Point : A->SwatLookAtPoints)
                {
                    if (FVector::Dist(FVector(Point.Location), FVector(LookAtPoint.Location)) < 50.0f)
                    {
                        bAnyNearby = true;
                        break;
                    }
                }
                
                if (!bAnyNearby)
                    A->SwatLookAtPoints.AddUnique(LookAtPoint);
            }
        }
    }
}

AThreatAwarenessActor* AWorldDataGenerator::GetSwatLookAtThreats(AThreatAwarenessActor* Threat, TArray<AThreatAwarenessActor*> NotLookAtThese)
{
    float FurthestDist = 0.0f;
    AThreatAwarenessActor* FurtherestThreat = nullptr;
    for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
    {
        AThreatAwarenessActor* A = *It;
        if (A == Threat)
            continue;

        bool bBadThreat = false;

        for (AThreatAwarenessActor* B : NotLookAtThese)
        {
            if (B)
            {
                FVector v1 = UKismetMathLibrary::FindLookAtRotation(Threat->GetActorLocation(), A->GetActorLocation()).Vector();
                FVector v2 = UKismetMathLibrary::FindLookAtRotation(Threat->GetActorLocation(), B->GetActorLocation()).Vector();

                float DotProduct2D = FVector::DotProduct(v1, v2);
                if (DotProduct2D > 0.95f)
                {
                    bBadThreat = true;
                    break;
                }
            }

        }

        if (bBadThreat)
            continue;     

        float Dist = (Threat->GetActorLocation() - A->GetActorLocation()).Size();
        FHitResult Hit;
        FCollisionQueryParams CollisionQueryParams;
        for (TActorIterator<ADoor>DoorIt(GetWorld()); DoorIt; ++DoorIt)
        {
            CollisionQueryParams.AddIgnoredActor(*DoorIt);
        }
        GetWorld()->LineTraceSingleByObjectType(Hit, Threat->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f), A->GetActorLocation()  + FVector(0.0f, 0.0f, 120.0f), FCollisionObjectQueryParams(ECC_WorldStatic), CollisionQueryParams);
        if (!Hit.bBlockingHit && Dist > FurthestDist && Dist > 250.0f)
        {
            if (FurtherestThreat)
            {
                if (FurtherestThreat->GetThreatLevel() > A->GetThreatLevel())
                    continue;
            }
            FurtherestThreat = A;
            FurthestDist = Dist;
        }
        
    }
    return FurtherestThreat;
}

void AWorldDataGenerator::ClearNullReferences()
{
	GEngine->ForceGarbageCollection(true);
    
    for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
    {
        AThreatAwarenessActor* a = *It;
        
        a->PathableThreatAwarenessActors.Remove(nullptr);
    }
    
    for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
    {
        ADoor* d = *It;
        
        d->FrontThreatAwarenessPoints.Remove(nullptr);
        d->BackThreatAwarenessPoints.Remove(nullptr);
        
        d->FrontRightStackUpPoints.Remove(nullptr);
        d->FrontLeftStackUpPoints.Remove(nullptr);
        d->BackLeftStackUpPoints.Remove(nullptr);
        d->BackRightStackUpPoints.Remove(nullptr);

        for (FClearPoint& p : d->FrontLeftClearPoints)
        {
            p.CoverLandmarks.Remove(nullptr);
        }
        
        for (FClearPoint& p : d->FrontRightClearPoints)
        {
            p.CoverLandmarks.Remove(nullptr);
        }
        
        for (FClearPoint& p : d->BackLeftClearPoints)
        {
            p.CoverLandmarks.Remove(nullptr);
        }
        
        for (FClearPoint& p : d->BackRightClearPoints)
        {
            p.CoverLandmarks.Remove(nullptr);
        }
    }
}

void AWorldDataGenerator::GenerateCoverPoints()
{
    if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        if (AReadyOrNotRecastNavMesh* NavMesh = Cast<AReadyOrNotRecastNavMesh>(NavSys->MainNavData))
        {
            NavMesh->GenerateCoverPoints();
        }
    }
}

void AWorldDataGenerator::DestroyCoverPoints()
{
    if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        if (AReadyOrNotRecastNavMesh* NavMesh = Cast<AReadyOrNotRecastNavMesh>(NavSys->MainNavData))
        {
            NavMesh->DeleteAllCoverPoints();
        }
    }
}

void AWorldDataGenerator::CalculateAllExits()
{
    FScopedSlowTask SlowTask(GetThreatAwarenessActorCount() * 3.0f, FText::FromString("Generating Exit Data"));
    SlowTask.MakeDialog();
    
    IgnoredExitThreats.Empty();
    GetOrCreateIgnoredExitThreats(IgnoredExitThreats);
    
    for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
    {
        It->EnableNavLink();
    }
    
    for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
    {
        SlowTask.EnterProgressFrame();
        It->CalculateExits();
    }
    
    for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
    {
        SlowTask.EnterProgressFrame();
        It->RemoveAnyVisibleExits();
    }
   
    for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
    {
        AThreatAwarenessActor* Threat = *It;
        SlowTask.EnterProgressFrame();
        Threat->GenerateUniqueExits();
        Threat->GeneratePossibleRoutes();
    }
    
    for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
    {
        It->DestroyNavLink();
    }
}

void AWorldDataGenerator::GetOrCreateIgnoredExitThreats(TArray<AThreatAwarenessActor*>& OutThreats)
{
    // don't allow exits where the player start is!
    if (IgnoredExitThreats.Num() == 0)
    {
        for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
        {
            AThreatAwarenessActor* Threat = *It;

            FHitResult OutsideTest;
            GetWorld()->LineTraceSingleByObjectType(OutsideTest, Threat->GetActorLocation(), Threat->GetActorLocation() + FVector(0.0f, 0.0f, 100000.0f), FCollisionObjectQueryParams(ECC_WorldStatic));
            if (!OutsideTest.bBlockingHit)
            {
                IgnoredExitThreats.AddUnique(Threat);
            }
		
            for (TActorIterator<APlayerStart>PlayerStartIt(GetWorld()); PlayerStartIt; ++PlayerStartIt)
            {
                FHitResult LOSTest;
                FCollisionObjectQueryParams CollisionObjectQueryParams;
                CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
                GetWorld()->LineTraceSingleByObjectType(LOSTest, PlayerStartIt->GetActorLocation(), Threat->GetActorLocation(), CollisionObjectQueryParams);
                if (!LOSTest.bBlockingHit)
                {
                    for (AThreatAwarenessActor* t : Threat->PathableThreatAwarenessActors)
                    {
                        IgnoredExitThreats.AddUnique(t);				
                    }				
                }
            }
        }

        for (TActorIterator<APlayerStart>StartIt(GetWorld()); StartIt; ++StartIt)
        {
            for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
            {
                AThreatAwarenessActor* ThreatAwarenessActor = *It;
                if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
                {
                    FPathFindingQuery PathFindingQuery;
                    FNavLocation NavLocation(StartIt->GetActorLocation()), SpawnNavLocation(ThreatAwarenessActor->GetActorLocation());
                    NavSys->ProjectPointToNavigation(NavLocation.Location, NavLocation, FVector(100.0f, 100.0f, 200.0f));
                    NavSys->ProjectPointToNavigation(SpawnNavLocation.Location, SpawnNavLocation, FVector(100.0f, 100.0f, 200.0f));
                    PathFindingQuery.StartLocation = SpawnNavLocation.Location;
                    PathFindingQuery.EndLocation = NavLocation.Location;
                    PathFindingQuery.SetAllowPartialPaths(false);
                    TSubclassOf<UNavigationQueryFilter> FilterClass = UNavigationQueryFilter::StaticClass();
                    FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
                    PathFindingQuery.QueryFilter = QueryFilter;
                    FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
                    if (PathFindingResult.Result == ENavigationQueryResult::Success)
                    {
                        IgnoredExitThreats.AddUnique(ThreatAwarenessActor);
                    } 
                }
            }
        }
    }
    OutThreats = IgnoredExitThreats;   
}

float AWorldDataGenerator::GetPathLength(FVector StartLocation, FVector EndLocation)
{
    if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        if (NavSys->MainNavData)
        {
            FPathFindingQuery PathFindingQuery;
            FNavLocation NavStartLocation(StartLocation);
            NavSys->ProjectPointToNavigation(NavStartLocation.Location, NavStartLocation, FVector(100.0f, 100.0f, 200.0f));
            FNavLocation NavEndLocation(EndLocation);
            NavSys->ProjectPointToNavigation(NavEndLocation.Location, NavEndLocation, FVector(100.0f, 100.0f, 200.0f));
            PathFindingQuery.StartLocation = NavStartLocation.Location;            
            PathFindingQuery.EndLocation = NavEndLocation.Location;
            PathFindingQuery.SetAllowPartialPaths(false);
            TSubclassOf<UNavigationQueryFilter> FilterClass = UNavigationQueryFilter::StaticClass();
            FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
            PathFindingQuery.QueryFilter = QueryFilter;
            FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
            if (PathFindingResult.Result == ENavigationQueryResult::Success)
            {
                return PathFindingResult.Path->GetLength();
            }
        }
    }
    
    return -1.0f;
}

TArray<ANavMeshBoundsVolume*> AWorldDataGenerator::GetNavMeshBounds()
{
    TArray<AActor*> OutActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANavMeshBoundsVolume::StaticClass(), OutActors);
    TArray<ANavMeshBoundsVolume*> NavMeshes;
    for (int32 i = 0; i < OutActors.Num(); i++)
    {
        ANavMeshBoundsVolume* nav =  Cast<ANavMeshBoundsVolume>(OutActors[i]);
        NavMeshes.Add(nav);
    }
    return NavMeshes;
}

int32 AWorldDataGenerator::GetThreatAwarenessActorCount()
{
    TArray<AActor*> OutActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AThreatAwarenessActor::StaticClass(), OutActors);
    return OutActors.Num();
}

AThreatAwarenessActor* AWorldDataGenerator::GetNearestThreat(FVector Location, bool bRequirePath, bool bExcludeDoorThreats)
{
    float closestPathDist = 9999.0f;
    AThreatAwarenessActor* closestTaa = nullptr;
    for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
    {
        AThreatAwarenessActor* taa = *It;

        if (bExcludeDoorThreats && taa->GetAttachedDoor())
            continue;
        
        float dist = (taa->GetActorLocation() - Location).Size();
        if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            if (NavSys->MainNavData)
            {
                if (bRequirePath)
                {
                    FPathFindingQuery PathFindingQuery;
                    PathFindingQuery.StartLocation = Location;            
                    PathFindingQuery.EndLocation = taa->GetActorLocation();
                    PathFindingQuery.SetAllowPartialPaths(false);
                    TSubclassOf<UNavigationQueryFilter> FilterClass = UNavigationQueryFilter::StaticClass();
                    FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
                    PathFindingQuery.QueryFilter = QueryFilter;
                    FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
                    if (PathFindingResult.Result != ENavigationQueryResult::Success)
                    {
                        continue;
                    }
                    if (PathFindingResult.Path->GetLength() < closestPathDist)
                    {
                        closestTaa = taa;
                        closestPathDist = PathFindingResult.Path->GetLength();
                    }
                }
                else 
                {
                    if (dist < closestPathDist)
                    {
                        closestPathDist = dist;
                        closestTaa = taa;
                    }
                }
            }
        }
    }
    return closestTaa;
}

void AWorldDataGenerator::GenerateWorldThreatAwarenessActors()
{
    int32 Doors = 0;
    for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
    {
        Doors++;
    }
    
    FScopedSlowTask SlowTask(Doors*1000.0f,NSLOCTEXT("GeneratingThreatAwarenessActors", "Generating Threat Awareness Actors", "Generating Threat Awareness Actors"));
    SlowTask.MakeDialog();
    
    for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
    {
        ADoor* Door = *It;
        
        for (int32 i = -10; i < 10; i++)
        {
            for (int32 y = -10; y < 10; y++)
            {
                for (int32 z = -2; z < 2; z++)
                {
                    SlowTask.EnterProgressFrame();

                    // Spawn threat awareness actor at location
                    // plus other setup
                    {
                        FVector ThreatLocation = Door->GetActorLocation() + FVector(i * 250.0f, y * 250.0f, z * 500.0f);
                        if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
                        {
                            FNavLocation NavLocation(ThreatLocation);
                            if (NavSys->ProjectPointToNavigation(NavLocation.Location, NavLocation, FVector(100.0f, 100.0f, 50.0f)))
                            {
                                ThreatLocation = NavLocation;
                            }
                            else
                            {
                                continue;
                            }
                        }

                        if (!IsSuitableSpawnLocationForThreatAwareness(ThreatLocation))
                            continue;;

                        if (AThreatAwarenessActor* t = GetWorld()->SpawnActor<AThreatAwarenessActor>(AThreatAwarenessActor::StaticClass(), ThreatLocation, FRotator::ZeroRotator))
                        {
                            t->CheckIsOutside();
                            t->SetThreatLevel(EThreatLevel::TL_Low);
                            
                            for (TActorIterator<ADoor> It2(GetWorld()); It2; ++It2)
                            {
                                ADoor* OtherDoor = *It2;
                                
                                if (t->GetThreatLevel() < EThreatLevel::TL_High)
                                {
                                    float Dist = FVector::Distance(OtherDoor->GetActorLocation(), t->GetActorLocation());
                                    
                                    if (Dist < 1000.0f)
                                    {
                                        FCollisionQueryParams CollisionQueryParams;
                                        CollisionQueryParams.AddIgnoredActor(OtherDoor);

                                        bool bHasVisibility = !GetWorld()->LineTraceTestByChannel(ThreatLocation, OtherDoor->GetDoorMidLocation() + OtherDoor->GetActorForwardVector() * -100.0f, ECC_Visibility, CollisionQueryParams) || 
                                                               !GetWorld()->LineTraceTestByChannel(ThreatLocation, OtherDoor->GetDoorMidLocation() + OtherDoor->GetActorForwardVector() * 100.0f, ECC_Visibility, CollisionQueryParams);
                                        
                                        //DrawDebugLine(GetWorld(), Location, Door->GetDoorMidLocation() + Door->GetActorForwardVector() * -100.0f, Hit.bBlockingHit ? FColor::Red : FColor::Green, false, 2.0f);

                                        //bool bHasVisibility = Internal_PointHasLineOfSightToDoor(ThreatLocation, OtherDoor, true) ||
                                        //                      Internal_PointHasLineOfSightToDoor(ThreatLocation, OtherDoor, false);
                                        
                                        if (bHasVisibility)
                                            t->SetThreatLevel(EThreatLevel::TL_High);
                                        else
                                            t->SetThreatLevel(EThreatLevel::TL_Medium);
                                    }
                                    else
                                    {
                                        t->SetThreatLevel(EThreatLevel::TL_Medium);
                                    }
                                }
                            }
                            
                            for (TActorIterator<AAISpawn> It2(GetWorld()); It2; ++It2)
                            {
                                AAISpawn* Spawner = *It2;
                                
                                float Dist = FVector::Distance(Spawner->GetActorLocation(), t->GetActorLocation());
                                if (Dist < 500.0f)
                                {
                                    t->SetThreatLevel(EThreatLevel::TL_High);
                                    break;
                                }
                            }
                            
                            t->Tags.AddUnique("generated");

                            FString ThreatString = "None";
                            switch (t->GetThreatLevel())
                            {
                                case EThreatLevel::TL_None:     ThreatString = "None"; break;
                                case EThreatLevel::TL_Low:      ThreatString = "Low"; break;
                                case EThreatLevel::TL_Medium:   ThreatString = "Medium"; break;
                                case EThreatLevel::TL_High:     ThreatString = "High"; break;
                                case EThreatLevel::TL_Extreme:  ThreatString = "Extreme"; break;
                                case EThreatLevel::TL_Stairs:   ThreatString = "Stairs"; break;
                            }
                            
                            #if WITH_EDITOR
                            t->SetFolderPath("GeneratedThreatAwarenessActors");
                            t->SetActorLabel("TAA_" + ThreatString + "_" + FString::FromInt(ThreatIndex));
                            #endif
                            
                            ThreatIndex++;
                        }
                    }
                }
            }
        }
    }
    
    for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
    {
        if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            FNavLocation NavLocation(It->GetActorLocation());
            if (NavSys->ProjectPointToNavigation(NavLocation.Location, NavLocation, FVector(50.0f, 50.0f, 50.0f)))
            {
                FHitResult Hit;
                if (GetWorld()->LineTraceSingleByChannel(Hit, NavLocation.Location + FVector::UpVector * 50.0f, NavLocation.Location - FVector::UpVector * 100.0f, ECC_Visibility))
                {
                    It->SetActorLocation(Hit.Location + FVector::UpVector * 40.0f);
                }
                else
                {
                    It->SetActorLocation(NavLocation.Location + FVector::UpVector * 20.0f);
                }
            }
        }
    }
}

void AWorldDataGenerator::DestroyAllThreats()
{
    for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
    {
        AThreatAwarenessActor* Threat = *It;
        Threat->Destroy();
    }
    
    if (UThreatAwarenessSubsystem* Subsystem = GetWorld()->GetSubsystem<UThreatAwarenessSubsystem>())
    {
        Subsystem->RemoveAllThreatPoints();
    }
}

void AWorldDataGenerator::CleanUpOverlappingThreats()
{
    TArray<AThreatAwarenessActor*> ThreatsA;
    TArray<AThreatAwarenessActor*> ThreatsB;

    TArray<AThreatAwarenessActor*> OutThreats;
    

    for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
    {
        if (!It->IsDoorThreat())
        {
            OutThreats.AddUnique(*It);
        }
    }
    
    ThreatsA = OutThreats;
    ThreatsB = OutThreats;

    for (int32 i = 0; i < ThreatsA.Num(); i++)
    {
        ThreatsB.Remove(ThreatsA[i]);
        for (int32 y = 0; y < ThreatsB.Num(); y++)
        {
            if (ThreatsA[i]->IsDoorThreat() || ThreatsB[y]->IsDoorThreat())
                continue;
            float Dist = (ThreatsA[i]->GetActorLocation() - ThreatsB[y]->GetActorLocation()).Size();
            if (Dist < 225.0f)
            {
                ThreatsB[y]->Destroy();
                V_LOGM(LogReadyOrNot, "Destroying %s overlapping threat", *ThreatsB[y]->GetName());
                ThreatsB.RemoveAt(y);
                if (y>0)
                    y--;               
            }
        }
    }
}

void AWorldDataGenerator::GenerateDoorThreatAwarenessActors(ADoor* Door)
{
    for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
    {
        AThreatAwarenessActor* a = *It;
        if ((a->IsDoorThreat() && a->GetAttachedDoor() == Door) || (a->IsDoorThreat() && !a->GetAttachedDoor()))
        {
            V_LOGM(LogReadyOrNot, "Destroying %s door threat not attached to door", *a->GetName());
            a->Destroy();
        }
    }

    Door->Setup();

    FVector DoorLocation = Door->GetDoorMidLocation();
    DoorLocation.Z = Door->GetActorLocation().Z;
    
    FVector Loc1 = DoorLocation + Door->GetActorForwardVector() * 80.0f;
    Loc1.Z += 50.0f;
    
    AThreatAwarenessActor* frontT = GetWorld()->SpawnActor<AThreatAwarenessActor>(AThreatAwarenessActor::StaticClass(), Loc1, FRotator::ZeroRotator);
    if (frontT)
    {
        frontT->CheckIsOutside();
        frontT->Tags.AddUnique("generated");
        frontT->DoorThreat = Door;
        frontT->bFrontDoorThreat = true;
        frontT->SetThreatLevel(EThreatLevel::TL_Extreme);
        Door->FrontThreat = frontT;
        
        #if WITH_EDITOR
        frontT->SetFolderPath("GeneratedDoorThreats");
        #endif
    }
    
    FVector Loc2 = DoorLocation + Door->GetActorForwardVector() * -80.0f;
    Loc2.Z += 50.0f;
    
    AThreatAwarenessActor* backT = GetWorld()->SpawnActor<AThreatAwarenessActor>(AThreatAwarenessActor::StaticClass(), Loc2, FRotator::ZeroRotator);
    if (backT)
    {
        backT->CheckIsOutside();
        backT->Tags.AddUnique("generated");
        backT->DoorThreat = Door;
        backT->bFrontDoorThreat = false;
        backT->SetThreatLevel(EThreatLevel::TL_Extreme);
        Door->BackThreat = backT;
        
        #if WITH_EDITOR
        backT->SetFolderPath("GeneratedDoorThreats");
        #endif
    }
}

bool AWorldDataGenerator::IsSuitableSpawnLocationForThreatAwareness(FVector Location)
{
    for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
    {
        AThreatAwarenessActor* a = *It;
        if (a->IsDoorThreat())
            continue;
        if ((a->GetActorLocation() - Location).Size() <  150.0f)
            return false;
    }
   
    bool bIsPointInAnyNavMesh = false;
    for (ANavMeshBoundsVolume* nav : GetNavMeshBounds())
    {
        if (UKismetMathLibrary::IsPointInBox(Location, nav->GetBounds().Origin, nav->GetBounds().BoxExtent))
        {
            bIsPointInAnyNavMesh = true;
        }
    }
    if (!bIsPointInAnyNavMesh)
    {
        return false; 
    }
    
    return true;
}

void AWorldDataGenerator::GenerateAllDoorThreatAwarenessActors()
{
    for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
    {
        ADoor* Door = *It;

        FVector DoorLocation = Door->GetDoorMidLocation();
        DoorLocation.Z = Door->GetActorLocation().Z;
        
        // Generate door threat awareness actors (the extreme ones)
        {
            for (TActorIterator<AThreatAwarenessActor> It2(GetWorld()); It2; ++It2)
            {
                AThreatAwarenessActor* a = *It2;
                if ((a->IsDoorThreat() && a->GetAttachedDoor() == Door) || (a->IsDoorThreat() && !a->GetAttachedDoor()))
                {
                    //V_LOGM(LogReadyOrNot, "Destroying %s door threat not attached to door", *a->GetName());
                    a->Destroy();
                }
            }

            {
                FVector Loc = DoorLocation + Door->GetActorForwardVector() * 80.0f;
                Loc.Z += 50.0f;
                
                if (AThreatAwarenessActor* frontT = GetWorld()->SpawnActor<AThreatAwarenessActor>(AThreatAwarenessActor::StaticClass(), Loc, FRotator::ZeroRotator))
                {
                    frontT->CheckIsOutside();
                    frontT->Tags.AddUnique("generated");
                    frontT->DoorThreat = Door;
                    frontT->bFrontDoorThreat = true;
                    frontT->SetThreatLevel(EThreatLevel::TL_Extreme);
                    Door->FrontThreat = frontT;
                    
                    #if WITH_EDITOR
                    frontT->SetFolderPath("GeneratedDoorThreats");
                    frontT->SetActorLabel("TAA_Extreme_" + FString::FromInt(ThreatIndex));
                    #endif

                    ThreatIndex++;
                }
            }

            {
                FVector Loc = DoorLocation + Door->GetActorForwardVector() * -80.0f;
                Loc.Z += 50.0f;
                
                if (AThreatAwarenessActor* backT = GetWorld()->SpawnActor<AThreatAwarenessActor>(AThreatAwarenessActor::StaticClass(), Loc, FRotator::ZeroRotator))
                {
                    backT->CheckIsOutside();
                    backT->Tags.AddUnique("generated");
                    backT->DoorThreat = Door;
                    backT->bFrontDoorThreat = false;
                    backT->SetThreatLevel(EThreatLevel::TL_Extreme);
                    Door->BackThreat = backT;
                    
                    #if WITH_EDITOR
                    backT->SetFolderPath("GeneratedDoorThreats");
                    backT->SetActorLabel("TAA_Extreme_" + FString::FromInt(ThreatIndex));
                    #endif
                    
                    ThreatIndex++;
                }
            }
        }
    }
}

void AWorldDataGenerator::GenerateAllDoorClearPoints()
{
    BlockAllDoorways();
    for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
    {
        GenerateDoorClearPointsV2(*It);
    }
    UnblockAllDoorways();
}

void AWorldDataGenerator::GenerateAllDoorPositions()
{
    for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
    {
        GenerateDoorPositions(*It);
    }
}

void AWorldDataGenerator::GenerateDoorPositions(ADoor* Door)
{
    if (Door->bManualRoomPositionSetup)
        return;
        
    // Front
    {
        FVector DoorForward = Door->GetActorForwardVector();
        
        FVector BaseLocation = Door->GetDoorMidLocation();
        FVector TraceStart = BaseLocation + DoorForward * 100.0f;

        FHitResult Hit, Hit2;
        float Distance = Door->GetDoorSize().Y + 80.0f;
        float UpOffset = 130.0f;
        bool bLeftHit1 = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceStart + Door->GetActorRightVector() * Distance, ECC_Visibility);
        bool bLeftHit2 = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart + FVector::UpVector * UpOffset, TraceStart + FVector::UpVector * UpOffset + Door->GetActorRightVector() * Distance, ECC_Visibility);
        bool bRightHit1 = GetWorld()->LineTraceSingleByChannel(Hit2, TraceStart, TraceStart + Door->GetActorRightVector() * -Distance, ECC_Visibility);
        bool bRightHit2 = GetWorld()->LineTraceSingleByChannel(Hit2, TraceStart + FVector::UpVector * UpOffset, TraceStart + FVector::UpVector * UpOffset + Door->GetActorRightVector() * -Distance, ECC_Visibility);
        
        DrawDebugLine(GetWorld(), TraceStart, TraceStart + Door->GetActorRightVector() * Distance, FColor::Cyan, false, 10.0f);
        DrawDebugLine(GetWorld(), TraceStart, TraceStart + Door->GetActorRightVector() * -Distance, FColor::Cyan, false, 10.0f);
        
        DrawDebugLine(GetWorld(), TraceStart + FVector::UpVector * UpOffset, TraceStart + FVector::UpVector * UpOffset + Door->GetActorRightVector() * -Distance, FColor::Cyan, false, 10.0f);
        DrawDebugLine(GetWorld(), TraceStart + FVector::UpVector * UpOffset, TraceStart + FVector::UpVector * UpOffset + Door->GetActorRightVector() * Distance, FColor::Cyan, false, 10.0f);

        bool bAllRightHit = bRightHit1 && bRightHit2;
        bool bAllLeftHit = bLeftHit1 && bLeftHit2;
        
        if (bAllRightHit && bAllLeftHit)
        {
            Door->SetBackRoomPosition(EDoorRoomPosition::Hallway);
        }
        else if (!bAllLeftHit && bAllRightHit)
        {
            Door->SetBackRoomPosition(EDoorRoomPosition::CornerLeft);
        }
        else if (bAllLeftHit && !bAllRightHit)
        {
            Door->SetBackRoomPosition(EDoorRoomPosition::CornerRight);
        }
        else
        {
            Door->SetBackRoomPosition(EDoorRoomPosition::Center);
        }

        if (Door->BackRoomPosition == EDoorRoomPosition::Center ||
            Door->BackRoomPosition == EDoorRoomPosition::Hallway)
        {
            FVector Whatever;
            FVector DoorLocation = Door->GetDoorMidLocation();
            DoorLocation.Z = Door->GetActorLocation().Z;
            bool bRightProjectionSuccess = false;
            bool bLeftProjectionSuccess = false;

            {
                FVector RightLocation = DoorLocation + DoorForward * 100.0f + Door->GetActorRightVector() * 100.0f;
                uint8 i = 0;
                while (i < 3)
                {
                    if (Internal_ProjectPointToNav(RightLocation, Whatever))
                    {
                        bRightProjectionSuccess = true;
                        break;
                    }
                    
                    RightLocation = RightLocation + Door->GetActorForwardVector() * 100.0f;
                    i++;
                }
            }
            
            {
                FVector LeftLocation = DoorLocation + DoorForward * 100.0f + -Door->GetActorRightVector() * 100.0f;
                uint8 i = 0;
                while (i < 3)
                {
                    if (Internal_ProjectPointToNav(LeftLocation, Whatever))
                    {
                        bLeftProjectionSuccess = true;
                        break;
                    }
                    
                    LeftLocation = LeftLocation + Door->GetActorForwardVector() * 100.0f;
                    i++;
                }
            }

            if (Door->BackRoomPosition == EDoorRoomPosition::Center)
            {
                if (!bRightProjectionSuccess && !bLeftProjectionSuccess)
                {
                    Door->SetBackRoomPosition(EDoorRoomPosition::Hallway);
                }
                else if (bRightProjectionSuccess && !bLeftProjectionSuccess)
                {
                    Door->SetBackRoomPosition(EDoorRoomPosition::CornerLeft);
                }
                else if (!bRightProjectionSuccess && bLeftProjectionSuccess)
                {
                    Door->SetBackRoomPosition(EDoorRoomPosition::CornerRight);
                }
            }
            else if (Door->BackRoomPosition == EDoorRoomPosition::Hallway)
            {
                if (!bRightProjectionSuccess && !bLeftProjectionSuccess)
                {
                    Door->SetBackRoomPosition(EDoorRoomPosition::HallwayRight);
                }
            }
        }
    }

    // Back
    {
        FVector DoorForward = -Door->GetActorForwardVector();
        
        FVector BaseLocation = Door->GetDoorMidLocation();
        FVector TraceStart = BaseLocation + DoorForward * 100.0f;

        FHitResult Hit, Hit2;
        float Distance = Door->GetDoorSize().Y + 80.0f;
        float UpOffset = 130.0f;
        bool bRightHit1 = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceStart + Door->GetActorRightVector() * Distance, ECC_Visibility);
        bool bRightHit2 = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart + FVector::UpVector * UpOffset, TraceStart + FVector::UpVector * UpOffset + Door->GetActorRightVector() * Distance, ECC_Visibility);
        
        bool bLeftHit1 = GetWorld()->LineTraceSingleByChannel(Hit2, TraceStart, TraceStart + Door->GetActorRightVector() * -Distance, ECC_Visibility);
        bool bLeftHit2 = GetWorld()->LineTraceSingleByChannel(Hit2, TraceStart + FVector::UpVector * UpOffset, TraceStart + FVector::UpVector * UpOffset + Door->GetActorRightVector() * -Distance, ECC_Visibility);
        
        DrawDebugLine(GetWorld(), TraceStart, TraceStart + Door->GetActorRightVector() * Distance, FColor::Cyan, false, 10.0f);
        DrawDebugLine(GetWorld(), TraceStart, TraceStart + Door->GetActorRightVector() * -Distance, FColor::Cyan, false, 10.0f);
        
        DrawDebugLine(GetWorld(), TraceStart + FVector::UpVector * UpOffset, TraceStart + FVector::UpVector * UpOffset + Door->GetActorRightVector() * Distance, FColor::Cyan, false, 10.0f);
        DrawDebugLine(GetWorld(), TraceStart + FVector::UpVector * UpOffset, TraceStart + FVector::UpVector * UpOffset + Door->GetActorRightVector() * -Distance, FColor::Cyan, false, 10.0f);
        
        bool bAllRightHit = bRightHit1 && bRightHit2;
        bool bAllLeftHit = bLeftHit1 && bLeftHit2;

        if (bAllRightHit && bAllLeftHit)
        {
            Door->SetFrontRoomPosition(EDoorRoomPosition::Hallway);
        }
        else if (!bAllLeftHit && bAllRightHit)
        {
            Door->SetFrontRoomPosition(EDoorRoomPosition::CornerLeft);
        }
        else if (bAllLeftHit && !bAllRightHit)
        {
            Door->SetFrontRoomPosition(EDoorRoomPosition::CornerRight);
        }
        else
        {
            Door->SetFrontRoomPosition(EDoorRoomPosition::Center);
        }

        if (Door->FrontRoomPosition == EDoorRoomPosition::Center ||
            Door->FrontRoomPosition == EDoorRoomPosition::Hallway)
        {
            FVector Whatever;
            FVector DoorLocation = Door->GetDoorMidLocation();
            DoorLocation.Z = Door->GetActorLocation().Z;
            //bool bRightProjectionSuccess = Internal_ProjectPointToNav(DoorLocation + -Door->GetActorForwardVector() * 100.0f + Door->GetActorRightVector() * 100.0f, Whatever);
            //bool bLeftProjectionSuccess = Internal_ProjectPointToNav(DoorLocation + -Door->GetActorForwardVector() * 100.0f + -Door->GetActorRightVector() * 100.0f, Whatever);
            
            bool bRightProjectionSuccess = false;//Internal_ProjectPointToNav(, Whatever);
            bool bLeftProjectionSuccess = false;//Internal_ProjectPointToNav(, Whatever);

            {
                FVector RightLocation = DoorLocation + DoorForward * 100.0f + Door->GetActorRightVector() * 100.0f;
                uint8 i = 0;
                while (i < 3)
                {
                    if (Internal_ProjectPointToNav(RightLocation, Whatever))
                    {
                        bRightProjectionSuccess = true;
                        break;
                    }
                    
                    RightLocation = RightLocation + -Door->GetActorForwardVector() * 100.0f;
                    i++;
                }
            }
            
            {
                FVector LeftLocation = DoorLocation + DoorForward * 100.0f + -Door->GetActorRightVector() * 100.0f;
                uint8 i = 0;
                while (i < 3)
                {
                    if (Internal_ProjectPointToNav(LeftLocation, Whatever))
                    {
                        bLeftProjectionSuccess = true;
                        break;
                    }
                    
                    LeftLocation = LeftLocation + -Door->GetActorForwardVector() * 100.0f;
                    i++;
                }
            }

            if (Door->FrontRoomPosition == EDoorRoomPosition::Center)
            {
                if (!bRightProjectionSuccess && !bLeftProjectionSuccess)
                {
                    Door->SetFrontRoomPosition(EDoorRoomPosition::Hallway);
                }
                else if (bRightProjectionSuccess && !bLeftProjectionSuccess)
                {
                    Door->SetFrontRoomPosition(EDoorRoomPosition::CornerRight);
                }
                else if (!bRightProjectionSuccess && bLeftProjectionSuccess)
                {
                    Door->SetFrontRoomPosition(EDoorRoomPosition::CornerLeft);
                }
            }
            else if (Door->FrontRoomPosition == EDoorRoomPosition::Hallway)
            {
                if (!bRightProjectionSuccess && !bLeftProjectionSuccess)
                {
                    Door->SetFrontRoomPosition(EDoorRoomPosition::HallwayRight);
                }
            }
        }
    }

    if (Door->BackRoomPosition == EDoorRoomPosition::Hallway)
    {
        if (Door->FrontRoomPosition == EDoorRoomPosition::CornerLeft)
        {
            Door->SetBackRoomPosition(EDoorRoomPosition::HallwayRight);
        }
        else if (Door->FrontRoomPosition == EDoorRoomPosition::CornerRight)
        {
            Door->SetBackRoomPosition(EDoorRoomPosition::HallwayLeft);
        }
    }

    if (Door->FrontRoomPosition == EDoorRoomPosition::Hallway)
    {
        if (Door->BackRoomPosition == EDoorRoomPosition::CornerLeft)
        {
            Door->SetFrontRoomPosition(EDoorRoomPosition::HallwayRight);
        }
        else if (Door->BackRoomPosition == EDoorRoomPosition::CornerRight)
        {
            Door->SetFrontRoomPosition(EDoorRoomPosition::HallwayLeft);
        }
    }

    if (Door->GetSubDoor())
    {
        if (Door->IsMainSubdoor())
        {
            GenerateDoorPositions(Door->GetSubDoor());

            if (Door->GetSubDoor()->FrontRoomPosition == EDoorRoomPosition::CornerLeft ||
                Door->GetSubDoor()->FrontRoomPosition == EDoorRoomPosition::CornerRight)
            {
                // transform into hallway
                if (Door->BackRoomPosition == EDoorRoomPosition::CornerLeft ||
                    Door->BackRoomPosition == EDoorRoomPosition::CornerRight)
                {
                    Door->SetBackRoomPosition(EDoorRoomPosition::Hallway);
                }
                else
                {
                    Door->SetBackRoomPosition(Door->GetSubDoor()->FrontRoomPosition);
                }
            }
            
            if (Door->GetSubDoor()->BackRoomPosition == EDoorRoomPosition::CornerLeft ||
                Door->GetSubDoor()->BackRoomPosition == EDoorRoomPosition::CornerRight ||
                Door->GetSubDoor()->BackRoomPosition == EDoorRoomPosition::Hallway)
            {
                if (Door->FrontRoomPosition == EDoorRoomPosition::CornerLeft ||
                    Door->FrontRoomPosition == EDoorRoomPosition::CornerRight)
                {
                    Door->SetFrontRoomPosition(EDoorRoomPosition::Hallway);
                }
                else
                {
                    Door->SetFrontRoomPosition(Door->GetSubDoor()->BackRoomPosition);
                }
            }
            
            Door->GetSubDoor()->SetFrontRoomPosition(Door->BackRoomPosition);
            Door->GetSubDoor()->SetBackRoomPosition(Door->FrontRoomPosition);
        }
    }
}

void AWorldDataGenerator::GenerateDoorClearPointsV2(ADoor* Door)
{
    if (Door->bManualClearPointSetup)
        return;
    
    Door->FrontLeftClearPoints.Empty();
    Door->FrontRightClearPoints.Empty();
    Door->BackLeftClearPoints.Empty();
    Door->BackRightClearPoints.Empty();
    
    if (Door->GetSubDoor())
    {
        if (!Door->IsMainSubdoor())
            return;
    }

    //FScopedSlowTask SlowTask(2, FText::FromString("Generating Clear Points for " + Door->GetName()));
    //SlowTask.MakeDialog();

    if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        if (const AReadyOrNotRecastNavMesh* NavMesh = Cast<AReadyOrNotRecastNavMesh>(NavSys->MainNavData))
        {
            FRecastDebugGeometry NavGeometry;
            NavGeometry.bGatherNavMeshEdges = true;

            // Get the navigation vertices via a batch query
            NavMesh->BeginBatchQuery();
            NavMesh->GetDebugGeometry(NavGeometry, -1);
            NavMesh->FinishBatchQuery();

            TArray<FVector>& Vertices = NavGeometry.NavMeshEdges;
            
            const int32 NumVertices = Vertices.Num();

            TArray<FClearPoint> ClearPoints_Front;
            TArray<FClearPoint> ClearPoints_Back;
            
            FTransform Transform;
            Transform.SetLocation(Door->GetDoorway()->GetComponentLocation());
            Transform.SetRotation(Door->GetActorRotation().Quaternion());
            
            FVector Extent = FVector(75.0f, 50.0f, 120.0f); 
            DrawDebugBox(GetWorld(), Transform.GetLocation(), Extent, Transform.GetRotation(), FColor::Cyan, false, 10.0f);
            
            if (NumVertices > 1)
            {
                for (int32 i = 0; i < NumVertices; i+=2)
                {
                    FVector Vertex1 = FVector(Vertices[i].X, Vertices[i].Y, Vertices[i].Z + 100.0f);
                    FVector Vertex2 = FVector(Vertices[i+1].X, Vertices[i+1].Y, Vertices[i+1].Z + 100.0f);
                    
                    if (FVector::Distance(Vertex1, Door->GetDoorMidLocation()) > 3000.0f)
                    {
                        //DrawDebugPoint(GetWorld(), Vertex1, 10.0f, FColor::Red, false, 10.0f);
                        continue;
                    }
                    
                    const float ZHeightDifference = FMath::Abs(Vertex1.Z - Door->GetActorLocation().Z);
                    if (ZHeightDifference > 400.0f)
                    {
                        //DrawDebugPoint(GetWorld(), Vertex1, 10.0f, FColor::Orange, false, 10.0f);
                        continue;
                    }

                    const float EdgeLength = (Vertex2 - Vertex1).Size2D();
                    const int32 AmountOfPoints = EdgeLength/100.0f;

                    float IncrementAmount = AmountOfPoints == 1 ? 0.5f : 1.0f/AmountOfPoints;
                    if (AmountOfPoints <= 0)
                    {
                        IncrementAmount = 1.0f;
                    }

                    // front
                    if (Door->IsPointInFrontOfDoorway(Vertex1))
                    {
                        FVector DoorLocation = Door->GetDoorMidLocation() + Door->GetActorForwardVector() * 75.0f;
                        DoorLocation.Z = Door->GetActorLocation().Z;
                        DrawDebugBox(GetWorld(), DoorLocation, FVector(10.0f), FColor::Magenta, false, 1.0f);
                        
                        FVector Whatever;
                        if (!Internal_ProjectPointToNav(DoorLocation, DoorLocation) ||
                            !Internal_ProjectPointToNav(Vertices[i], Whatever))
                        {
                            continue;
                        }
                        
                        if (!Internal_FindPath_RoomGen(Vertices[i], DoorLocation))
                        {
                            continue;
                        }
                        
                        for (float j = 0.0f; j < 1.0f; j += IncrementAmount)
                        {
                            FVector VertexA = FMath::Lerp(Vertex1, Vertex2, FMath::Clamp(j, 0.0f, 1.0f));
                            FVector VertexB = FMath::Lerp(Vertex1, Vertex2, FMath::Clamp(j+IncrementAmount, 0.0f, 1.0f));

                            if (!Internal_ProjectPointToNav(VertexA, Whatever) ||
                                !Internal_ProjectPointToNav(VertexB, Whatever))
                            {
                                DrawDebugPoint(GetWorld(), VertexA, 10.0f, FColor::Red, false, 10.0f);
                                continue;
                            }
                            
                            // make sure point is not in the doorway
                            if ((Door->BackRoomPosition == EDoorRoomPosition::CornerLeft ||
                                Door->BackRoomPosition == EDoorRoomPosition::CornerRight) ||
                                (!UKismetMathLibrary::IsPointInBoxWithTransform(VertexA, Transform, Extent) &&
                                !UKismetMathLibrary::IsPointInBoxWithTransform(VertexB, Transform, Extent)))
                            {
                                FClearPoint PointA, PointB;
                                PointA.Location = FVector(FIntVector(VertexA));
                                PointA.Location_Relative = Door->GetActorTransform().InverseTransformPosition(PointA.Location);
                                PointB.Location = FVector(FIntVector(VertexB));
                                PointB.Location_Relative = Door->GetActorTransform().InverseTransformPosition(PointB.Location);
                                PointA.bHasLineOfSightToDoor = Internal_PointHasLineOfSightToDoor(VertexA, Door, true);
                                PointB.bHasLineOfSightToDoor = Internal_PointHasLineOfSightToDoor(VertexA, Door, true);
                    
                                ClearPoints_Front.AddUnique(PointA);
                                //DrawDebugPoint(GetWorld(), VertexA, 10.0f, FColor::Cyan, false, 10.0f);
                                ClearPoints_Front.AddUnique(PointB);
                                //DrawDebugPoint(GetWorld(), VertexB, 10.0f, FColor::Cyan, false, 10.0f);
                            }
                                
                        }
                    }
                    // back
                    else
                    {
                        FVector DoorLocation = Door->GetDoorMidLocation() + Door->GetActorForwardVector() * -75.0f;
                        DoorLocation.Z = Door->GetActorLocation().Z;
                        //DrawDebugBox(GetWorld(), DoorLocation, FVector(10.0f), FColor::Magenta, false, 1.0f);
                        
                        FVector Whatever;
                        if (!Internal_ProjectPointToNav(DoorLocation, DoorLocation) ||
                            !Internal_ProjectPointToNav(Vertices[i], Whatever))
                        {
                            continue;
                        }
                        
                        if (!Internal_FindPath_RoomGen(Vertices[i], DoorLocation))
                            continue;
                        
                        for (float j = 0.0f; j < 1.0f; j += IncrementAmount)
                        {
                            const FVector VertexA = FMath::Lerp(Vertex1, Vertex2, FMath::Clamp(j, 0.0f, 1.0f));
                            const FVector VertexB = FMath::Lerp(Vertex1, Vertex2, FMath::Clamp(j+IncrementAmount, 0.0f, 1.0f));
                            
                            if (!Internal_ProjectPointToNav(VertexA, Whatever) ||
                                !Internal_ProjectPointToNav(VertexB, Whatever))
                                continue;
                    
                            // make sure point is not in the doorway
                            if ((Door->FrontRoomPosition == EDoorRoomPosition::CornerLeft ||
                                Door->FrontRoomPosition == EDoorRoomPosition::CornerRight) ||
                                (!UKismetMathLibrary::IsPointInBoxWithTransform(VertexA, Transform, Extent) &&
                                !UKismetMathLibrary::IsPointInBoxWithTransform(VertexB, Transform, Extent)))
                            {
                                FClearPoint PointA, PointB;
                                PointA.Location = FVector(FIntVector(VertexA));
                                PointA.Location_Relative = Door->GetActorTransform().InverseTransformPosition(PointA.Location);
                                PointB.Location = FVector(FIntVector(VertexB));
                                PointB.Location_Relative = Door->GetActorTransform().InverseTransformPosition(PointB.Location);
                                PointA.bHasLineOfSightToDoor = Internal_PointHasLineOfSightToDoor(VertexA, Door, false);
                                PointB.bHasLineOfSightToDoor = Internal_PointHasLineOfSightToDoor(VertexA, Door, false);
                                
                                ClearPoints_Back.AddUnique(PointA);
                                //DrawDebugPoint(GetWorld(), VertexA + FVector::UpVector * 10.0f, 10.0f, FColor::Cyan, false, 10.0f);
                                ClearPoints_Back.AddUnique(PointB);
                                //DrawDebugPoint(GetWorld(), VertexB + FVector::UpVector * 10.0f, 10.0f, FColor::Cyan, false, 10.0f);
                            }
                        }
                    }
                }
            }
            
            if (ClearPoints_Front.Num() > 2)
            {
                ClearPoints_Front.Sort([Door](const FClearPoint& Lhs, const FClearPoint& Rhs)
                {
                    return (Door->GetDoorMidLocation() - Lhs.Location).Size() < (Door->GetDoorMidLocation() - Rhs.Location).Size(); 
                });

                FVector DoorLocation = Door->GetDoorMidLocation();
                
                TArray<FClearPoint> RightmostClearPoints = FindClearPointsInDirection(ClearPoints_Front, DoorLocation, Door->GetActorForwardVector(), Door->GetActorRightVector(), Door->MaxFrontHorizontalClearingDistance, Door->MaxFrontRightClearThreshold, Door->BackRoomPosition);
                TArray<FClearPoint> LeftmostClearPoints = FindClearPointsInDirection(ClearPoints_Front, DoorLocation, Door->GetActorForwardVector(), -Door->GetActorRightVector(), Door->MaxFrontHorizontalClearingDistance, Door->MaxFrontLeftClearThreshold, Door->BackRoomPosition);

                FClearPoint FirstPoint;
                FirstPoint.Location = FVector(FIntVector(FVector(DoorLocation.X, DoorLocation.Y, Door->GetActorLocation().Z+100.0f) + Door->GetActorForwardVector() * 60.0f));
                FirstPoint.Location_Relative = Door->GetActorTransform().InverseTransformPosition(FirstPoint.Location);
                FirstPoint.Stage = 0;
                
                RightmostClearPoints.Insert(FirstPoint, 0);
                LeftmostClearPoints.Insert(FirstPoint, 0);

                RightmostClearPoints = IncreaseClearPointResolution(RightmostClearPoints);
                LeftmostClearPoints = IncreaseClearPointResolution(LeftmostClearPoints);

                LinkNearbyCoverLandmarksToClearPoints(Door, &RightmostClearPoints, &LeftmostClearPoints);
                
                Door->FrontRightClearPoints = RightmostClearPoints;
                Door->FrontLeftClearPoints = LeftmostClearPoints;

                if (Door->MaxFrontRightClearPoints > 0)
                {
                    Door->FrontRightClearPoints.Empty();
                    
                    int32 i = 0;
                    for (FClearPoint& Point : RightmostClearPoints)
                    {
                        if (i == Door->MaxFrontRightClearPoints+1)
                            break;
                        
                        Door->FrontRightClearPoints.Add(Point);
                        
                        i++;
                    }
                }

                if (Door->MaxFrontLeftClearPoints > 0)
                {
                    Door->FrontLeftClearPoints.Empty();
                    
                    int32 i = 0;
                    for (FClearPoint& Point : LeftmostClearPoints)
                    {
                        if (i == Door->MaxFrontLeftClearPoints+1)
                            break;
                        
                        Door->FrontLeftClearPoints.Add(Point);
                        
                        i++;
                    }
                }

                const auto ConvertWorldToRelative = [Door](TArray<FClearPoint>& InClearPoints)
                {
                    for (FClearPoint& Point : InClearPoints)
                    {
                        Point.Location_Relative = Door->GetActorTransform().InverseTransformPosition(Point.Location);
                    }
                };

                ConvertWorldToRelative(Door->FrontRightClearPoints);
                ConvertWorldToRelative(Door->FrontLeftClearPoints);
                
                // bake in line of sight results
                {
                    for (FClearPoint& Point : Door->FrontRightClearPoints)
                    {
                        Point.bHasLineOfSightToDoor = Internal_PointHasLineOfSightToDoor(FVector(Point.Location), Door, true);
                    }
                    
                    for (FClearPoint& Point : Door->FrontLeftClearPoints)
                    {
                        Point.bHasLineOfSightToDoor = Internal_PointHasLineOfSightToDoor(FVector(Point.Location), Door, true);
                    }
                }

                Door->DrawClearPointsV2(RightmostClearPoints, 1.0f);
                Door->DrawClearPointsV2(LeftmostClearPoints, 1.0f);
            }
            
            if (ClearPoints_Back.Num() > 2)
            {
                ClearPoints_Back.Sort([Door](const FClearPoint& Lhs, const FClearPoint& Rhs)
                {
                    return (Door->GetDoorMidLocation() - FVector(Lhs.Location)).Size() < (Door->GetDoorMidLocation() - FVector(Rhs.Location)).Size(); 
                });
                
                FVector DoorLocation = Door->GetDoorMidLocation();

                TArray<FClearPoint> RightmostClearPoints = FindClearPointsInDirection(ClearPoints_Back, Door->GetDoorMidLocation(), -Door->GetActorForwardVector(), Door->GetActorRightVector(), Door->MaxBackHorizontalClearingDistance, Door->MaxBackRightClearThreshold, Door->FrontRoomPosition);
                TArray<FClearPoint> LeftmostClearPoints = FindClearPointsInDirection(ClearPoints_Back, Door->GetDoorMidLocation(), -Door->GetActorForwardVector(), -Door->GetActorRightVector(), Door->MaxBackHorizontalClearingDistance, Door->MaxBackLeftClearThreshold, Door->FrontRoomPosition);

                FClearPoint FirstPoint;
                FirstPoint.Location = FVector(FIntVector(FVector(DoorLocation.X, DoorLocation.Y, Door->GetActorLocation().Z+100.0f) + -Door->GetActorForwardVector() * 60.0f));
                FirstPoint.Location_Relative = Door->GetActorTransform().InverseTransformPosition(FirstPoint.Location);
                FirstPoint.Stage = 0;
                
                RightmostClearPoints.Insert(FirstPoint, 0);
                LeftmostClearPoints.Insert(FirstPoint, 0);

                RightmostClearPoints = IncreaseClearPointResolution(RightmostClearPoints);
                LeftmostClearPoints = IncreaseClearPointResolution(LeftmostClearPoints);

                LinkNearbyCoverLandmarksToClearPoints(Door, &RightmostClearPoints, &LeftmostClearPoints);

                Door->BackRightClearPoints = RightmostClearPoints;
                Door->BackLeftClearPoints = LeftmostClearPoints;

                if (Door->MaxBackRightClearPoints > 0)
                {
                    Door->BackRightClearPoints.Empty();
                    
                    int32 i = 0;
                    for (FClearPoint& Point : RightmostClearPoints)
                    {
                        if (i == Door->MaxBackRightClearPoints+1)
                            break;
                        
                        Door->BackRightClearPoints.Add(Point);
                        
                        i++;
                    }
                }

                if (Door->MaxBackLeftClearPoints > 0)
                {
                    Door->BackLeftClearPoints.Empty();
                    
                    int32 i = 0;
                    for (FClearPoint& Point : LeftmostClearPoints)
                    {
                        if (i == Door->MaxBackLeftClearPoints+1)
                            break;
                        
                        Door->BackLeftClearPoints.Add(Point);
                        
                        i++;
                    }
                }
                
                const auto ConvertWorldToRelative = [Door](TArray<FClearPoint>& InClearPoints)
                {
                    for (FClearPoint& Point : InClearPoints)
                    {
                        Point.Location_Relative = Door->GetActorTransform().InverseTransformPosition(Point.Location);
                    }
                };

                ConvertWorldToRelative(Door->BackRightClearPoints);
                ConvertWorldToRelative(Door->BackLeftClearPoints);

                // bake in line of sight results
                {
                    for (FClearPoint& Point : Door->BackRightClearPoints)
                    {
                        Point.bHasLineOfSightToDoor = Internal_PointHasLineOfSightToDoor(FVector(Point.Location), Door, true);
                    }
                    
                    for (FClearPoint& Point : Door->BackLeftClearPoints)
                    {
                        Point.bHasLineOfSightToDoor = Internal_PointHasLineOfSightToDoor(FVector(Point.Location), Door, true);
                    }
                }

                //Door->DrawClearPointsV2(RightmostClearPoints, 1.0f);
                //Door->DrawClearPointsV2(LeftmostClearPoints, 1.0f);
            }
        }
    }

    if (Door->GetSubDoor())
    {
        if (Door->IsMainSubdoor())
        {
            Door->GetSubDoor()->BackRightClearPoints = Door->FrontLeftClearPoints;
            Door->GetSubDoor()->BackLeftClearPoints = Door->FrontRightClearPoints;
            Door->GetSubDoor()->FrontRightClearPoints = Door->BackLeftClearPoints;
            Door->GetSubDoor()->FrontLeftClearPoints = Door->BackRightClearPoints;
            //return;
        }
    }
    
    //SlowTask.EnterProgressFrame();
}

TArray<FClearPoint> AWorldDataGenerator::FindClearPointsInDirection(const TArray<FClearPoint>& ClearPoints, FVector DoorLocation, FVector ForwardDirection, FVector RightDirection, float MaxHorizontalDistance, FVector2D MaxRightDotRange, EDoorRoomPosition RoomPosition) const
{
    TArray<FClearPoint> FoundClearPoints;
    FClearPoint ClosestStartingPoint;

    float BestDot = 0.0f;
    uint8 Iter = 0;
    for (const FClearPoint& Point : ClearPoints) // ClearPoints was already sorted
    {
        if (Iter == 4)
            break;

        if (Point.Location.Z < DoorLocation.Z-100.0f)
            continue;
        
        const float Dot = FVector::DotProduct(RightDirection, (FVector(Point.Location) - DoorLocation).GetSafeNormal2D());
        
        if (Dot > 0.0f && Dot > BestDot)
        {
            BestDot = Dot;
            ClosestStartingPoint = Point;

            Iter++;
        }
    }
    
    if (ClosestStartingPoint.Location != FVector::ZeroVector)
    {
        ClosestStartingPoint.Stage = 1;
        ClosestStartingPoint.Direction = EClearDirection::Right;
        FoundClearPoints.AddUnique(ClosestStartingPoint);
        
        FClearPoint Checkpoint = ClosestStartingPoint;
        for (int32 i = 0; i < ClearPoints.Num(); i++)
        {
            float ClosestDistance = FLT_MAX;
            FClearPoint ClosestPoint;
            for (const FClearPoint& OtherPoint : ClearPoints)
            {
                const float ZHeightDiff = FMath::Abs(Checkpoint.Location.Z - OtherPoint.Location.Z);
                if (ZHeightDiff > 300.0f)
                    continue;
                
                if (FVector::Distance(FVector(Checkpoint.Location), DoorLocation) > MaxHorizontalDistance)
                    break;
                
                if (!FoundClearPoints.Contains(OtherPoint) && Checkpoint.Location != OtherPoint.Location)
                {
                    // is point right of door?
                    // Note: only care if close to door
                    if (FVector::DotProduct(RightDirection, (FVector(OtherPoint.Location) - DoorLocation).GetSafeNormal2D()) > 0.0f)
                    {
                        // does point make forward progress? and continues to the right?
                        if (/*FVector::DotProduct(ForwardDirection, (FVector(OtherPoint.Location) - FVector(Checkpoint.Location)).GetSafeNormal2D()) > -0.25f &&*/
                           FVector::DotProduct(RightDirection, (FVector(OtherPoint.Location) - FVector(Checkpoint.Location)).GetSafeNormal2D()) > 0.5f)
                        {
                            FVector A = FVector::ZeroVector, B = FVector::ZeroVector;
                            Internal_ProjectPointToNav(DoorLocation + ForwardDirection * 100.0f, A);
                            Internal_ProjectPointToNav(FVector(OtherPoint.Location) + ForwardDirection * 50.0f, B);
                            
                            //DrawDebugPoint(GetWorld(), DoorLocation + ForwardDirection * 100.0f, 10.0f, FColor::Yellow, false, 10.0f);

                            if (!Internal_FindPath_RoomGen(A, B))
                                continue;
                            
                            float Distance = FVector::Distance(FVector(Checkpoint.Location), FVector(OtherPoint.Location));
                            
                            if (Distance < ClosestDistance)
                            {
                                ClosestDistance = Distance;
                                ClosestPoint = OtherPoint;
                            }
                        }
                    }
                }
            }

            // no more points this way
            if (ClosestPoint.Location == FVector::ZeroVector)
            {
                break;
            }
            
            Checkpoint = ClosestPoint;
            Checkpoint.Stage = FoundClearPoints.Num()+1;
            Checkpoint.Direction = EClearDirection::Right;
            FoundClearPoints.AddUnique(Checkpoint);
            //DrawDebugPoint(GetWorld(), Checkpoint - FVector::UpVector * 10.0f, 10.0f, FColor::Yellow, false, 10.0f);
        }

        DrawDebugPoint(GetWorld(), FVector(Checkpoint.Location) - FVector::UpVector * 10.0f, 10.0f, FColor::Yellow, false, 10.0f);
        DrawDebugLine(GetWorld(), FVector(Checkpoint.Location) - FVector::UpVector * 100000.0f, FVector(Checkpoint.Location) + FVector::UpVector * 100000.0f, FColor::Orange, false, 10.0f);
        
        float RightDotThreshold = MaxRightDotRange.X;
        
        float ForwardDotAgainstWorldForward = FMath::Abs(FVector::DotProduct(FVector::ForwardVector, ForwardDirection));
        float RightDotAgainstWorldRight = FMath::Abs(FVector::DotProduct(FVector::RightVector, RightDirection));
        bool bIsAxisAligned =   FMath::IsNearlyEqual(RightDotAgainstWorldRight, 1.0f, 0.2f) ||
                                FMath::IsNearlyEqual(RightDotAgainstWorldRight, 0.0f, 0.2f) ||
                                FMath::IsNearlyEqual(ForwardDotAgainstWorldForward, 1.0f, 0.2f) ||
                                FMath::IsNearlyEqual(ForwardDotAgainstWorldForward, 0.0f, 0.2f);
        if (!bIsAxisAligned)
        {
            if (RoomPosition != EDoorRoomPosition::Center)
            {
                if (MaxRightDotRange.Y > 1.0f-RightDotAgainstWorldRight)
                    MaxRightDotRange.Y = (1.0f-RightDotAgainstWorldRight-MaxRightDotRange.Y)+0.1f;
            }
            
            RightDotThreshold = FMath::Clamp(RightDotAgainstWorldRight, MaxRightDotRange.X, MaxRightDotRange.Y);
        }

        //LOG_NUMBER(RightDotThreshold);
        //LOG_NUMBER(RightDotAgainstWorldRight);
        
        for (int32 i = 0; i < ClearPoints.Num(); i++)
        {
            float ClosestDistance = FLT_MAX;
            FClearPoint ClosestPoint;
            for (const FClearPoint& OtherPoint : ClearPoints)
            {
                if (!FoundClearPoints.Contains(OtherPoint) && Checkpoint.Location != OtherPoint.Location)
                {
                    const float ZHeightDiff = FMath::Abs(Checkpoint.Location.Z - OtherPoint.Location.Z);
                    if (ZHeightDiff > 300.0f)
                        continue;

                    // is point right of door?
                    if (FVector::DotProduct(RightDirection, (FVector(OtherPoint.Location) - DoorLocation).GetSafeNormal2D()) > RightDotThreshold)
                    {
                        // does point make forward progress?
                        if (FVector::DotProduct(ForwardDirection, (FVector(OtherPoint.Location) - FVector(Checkpoint.Location)).GetSafeNormal2D()) > 0.75f)
                        {
                            FVector A = FVector::ZeroVector, B = FVector::ZeroVector;
                            Internal_ProjectPointToNav(FVector(ClosestStartingPoint.Location), A);
                            Internal_ProjectPointToNav(FVector(OtherPoint.Location), B);

                            if (!Internal_FindPath_RoomGen(A, B))
                                continue;
                            
                            const float Distance = FVector::Distance(FVector(Checkpoint.Location), FVector(OtherPoint.Location));

                            if (Distance < ClosestDistance)
                            {
                                ClosestDistance = Distance;
                                ClosestPoint = OtherPoint;
                            }
                        }
                    }
                }
            }

            // no more points this way
            if (ClosestPoint.Location == FVector::ZeroVector)
            {
                break;
            }
            
            Checkpoint = ClosestPoint;
            Checkpoint.Stage = FoundClearPoints.Num()+1;
            Checkpoint.Direction = EClearDirection::Forward;
            FoundClearPoints.AddUnique(Checkpoint);
            //DrawDebugPoint(GetWorld(), Checkpoint - FVector::UpVector * 10.0f, 10.0f, FColor::Yellow, false, 10.0f);
        }
    }

    return FoundClearPoints;
}

TArray<FClearPoint> AWorldDataGenerator::IncreaseClearPointResolution(const TArray<FClearPoint>& ClearPoints) const
{
    TArray<FClearPoint> FinalClearPoints;
    FinalClearPoints.Reserve(ClearPoints.Num()*2);
    FinalClearPoints.AddUnique(ClearPoints[0]);
    
    FVector FirstClearPoint = FVector(ClearPoints[0].Location);
    Internal_ProjectPointToNav(FVector(ClearPoints[0].Location), FirstClearPoint);
    
    for (int32 i = 0; i < ClearPoints.Num(); i++)
    {
        FVector PointA = FVector(ClearPoints[i].Location);
        FClearPoint P = ClearPoints[i];
        P.Stage = FinalClearPoints.Num();
        FinalClearPoints.AddUnique(P);
        
        if (!ClearPoints.IsValidIndex(i+1))
            break;
        
        FVector PointB = FVector(ClearPoints[i+1].Location);

        const float EdgeLength = (PointB - PointA).Size2D();
        const int32 AmountOfPoints = EdgeLength/100.0f;

        if (AmountOfPoints <= 1)
            continue;

        float IncrementAmount = AmountOfPoints == 1 ? 0.5f : 1.0f/AmountOfPoints;
        if (AmountOfPoints <= 0)
        {
            IncrementAmount = 1.0f;
        }

        for (float j = IncrementAmount; j < 1.0f; j += IncrementAmount)
        {
            FVector Vertex = FMath::Lerp(PointA, PointB, FMath::Clamp(j, 0.0f, 1.0f));

            FVector A = FVector::ZeroVector, V = FVector::ZeroVector;
            if (Internal_ProjectPointToNav(Vertex, V) &&
                Internal_ProjectPointToNav(PointA, A) &&
                Internal_FindPath(A, V, 2000.0f))
            {
                {
                    FClearPoint Point = ClearPoints[i];
                    Point.Location = FVector(FIntVector(V.X, V.Y, Vertex.Z));
                    Point.Stage = FinalClearPoints.Num();
                    Point.Direction = ClearPoints[i+1].Direction;
                    
                    FinalClearPoints.AddUnique(Point);
                }
            }

            //DrawDebugPoint(GetWorld(), Vertex, 10.0f, FColor::White, false, 10.0f);
        }

        /*
        if (ClearPoints[i].Direction == EClearDirection::Forward)
        {
            TArray<FNavPathPoint> PathPoints;
            
            FVector A = FVector::ZeroVector, B = FVector::ZeroVector;
            if (Internal_ProjectPointToNav(PointA, A) &&
                Internal_ProjectPointToNav(PointB, B) &&
                Internal_FindPath(A, B, 2000.0f, &PathPoints))
            {
                for (const FNavPathPoint& PathPoint : PathPoints)
                {
                    FClearPoint Point;
                    Point.Location = FIntVector(PathPoint.Location.X, PathPoint.Location.Y, PointA.Z);
                    Point.Stage = FinalClearPoints.Num();
                    Point.Direction = ClearPoints[i+1].Direction;
                    
                    FinalClearPoints.AddUnique(Point);
                }
            }
        }
        */
    }

    return FinalClearPoints;
}

void AWorldDataGenerator::LinkNearbyCoverLandmarksToClearPoints(ADoor* Door, TArray<FClearPoint>* RightClearPoints, TArray<FClearPoint>* LeftClearPoints)
{
    for (TActorIterator<ACoverLandmark> It(GetWorld()); It; ++It)
    {
        ACoverLandmark* Landmark = *It;

        if (Landmark->Type == ECoverLandmarkType::Corner)
            continue;
        
        if (Door->IsPointRightOfDoorway(Landmark->GetActorLocation()))
        {
            FClearPoint* ClosestClearPoint = nullptr;
            float ClosestDistance = FLT_MAX;
            for (FClearPoint& ClearPoint : *RightClearPoints)
            {
                const float DistanceToDoor = FVector::Distance(Door->GetDoorMidLocation(), FVector(ClearPoint.Location));
                if (DistanceToDoor < 300.0f)
                    continue;
                
                const float Distance = FVector::Distance(Landmark->GetActorLocation(), FVector(ClearPoint.Location));
                if (Distance < ClosestDistance)
                {
                    if (Internal_PointHasLineOfSightTo(Landmark->GetActorLocation(), FVector(ClearPoint.Location)))
                    {
                        for (const ACoverLandmarkProxy* Proxy : Landmark->EntryPoints)
                        {
                            FVector A = FVector::ZeroVector, B = FVector::ZeroVector;
                            Internal_ProjectPointToNav(Proxy->GetActorLocation(), A);
                            Internal_ProjectPointToNav(FVector(ClearPoint.Location), B);
                            
                            if (Internal_FindPath(A, B, 2000.0f))
                            {
                                ClosestClearPoint = &ClearPoint;
                                ClosestDistance = Distance;
                                break;
                            }
                        }
                    }
                }
            }

            if (ClosestClearPoint)
            {
                ClosestClearPoint->CoverLandmarks.AddUnique(Landmark);
            }
        }
        else
        {
            FClearPoint* ClosestClearPoint = nullptr;
            float ClosestDistance = FLT_MAX;
            for (FClearPoint& ClearPoint : *LeftClearPoints)
            {
                const float DistanceToDoor = FVector::Distance(Door->GetDoorMidLocation(), FVector(ClearPoint.Location));
                if (DistanceToDoor < 300.0f)
                    continue;
                
                const float Distance = FVector::Distance(Landmark->GetActorLocation(), FVector(ClearPoint.Location));
                if (Distance < ClosestDistance)
                {
                    if (Internal_PointHasLineOfSightTo(Landmark->GetActorLocation(), FVector(ClearPoint.Location)))
                    {
                        for (const ACoverLandmarkProxy* Proxy : Landmark->EntryPoints)
                        {
                            FVector A = FVector::ZeroVector, B = FVector::ZeroVector;
                            Internal_ProjectPointToNav(Proxy->GetActorLocation(), A);
                            Internal_ProjectPointToNav(FVector(ClearPoint.Location), B);
                            
                            if (Internal_FindPath(A, B, 2000.0f))
                            {
                                ClosestClearPoint = &ClearPoint;
                                ClosestDistance = Distance;
                                break;
                            }
                        }
                    }
                }
            }

            if (ClosestClearPoint)
            {
                ClosestClearPoint->CoverLandmarks.AddUnique(Landmark);
            }
        }
    }
}

void AWorldDataGenerator::BlockAllDoorways()
{
    if (!bDoorwaysBlocked)
    {
        for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
        {
            ADoor* OtherDoor = *It;
            //OtherDoor->SpawnDoorBlocker();
            if (OtherDoor->IsDoorwayOnly())
                OtherDoor->ActivateDoorBlocker();
        }
    
        GEngine->Exec(GetWorld(), TEXT("rebuildnavigation"));
        bDoorwaysBlocked = true;
    }
}

void AWorldDataGenerator::UnblockAllDoorways() 
{
    // only rebuild gen if it actually changed
    if (bDoorwaysBlocked)
    {
        GEngine->Exec(GetWorld(), TEXT("rebuildnavigation"));
        bDoorwaysBlocked = false;
    }

    // always attempt this
    for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
    {
        ADoor* OtherDoor = *It;
        //OtherDoor->RemoveDoorBlocker();
        OtherDoor->DeactivateDoorBlocker();
    }
}

void AWorldDataGenerator::GatherAllThreatsBetweenDistance(TArray<AThreatAwarenessActor*> InThreats, float MinDist,
                                                          float MaxDist, FVector TestLocation, TArray<AThreatAwarenessActor*>& OutThreats)
{
    for (AThreatAwarenessActor* Threat : InThreats)
    {
        if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            FNavLocation NavLocation(TestLocation);
            NavSys->ProjectPointToNavigation(NavLocation.Location, NavLocation, FVector(100.0f, 100.0f, 200.0f));
            FPathFindingQuery PathFindingQuery;
            PathFindingQuery.StartLocation = Threat->GetActorLocation();
            PathFindingQuery.EndLocation = NavLocation.Location;
            PathFindingQuery.SetAllowPartialPaths(false);
            TSubclassOf<UNavigationQueryFilter> FilterClass = UNavigationQueryFilter::StaticClass();
            FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
            PathFindingQuery.QueryFilter = QueryFilter;
            FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
            if (PathFindingResult.Result == ENavigationQueryResult::Success)
            {
                float Dist = PathFindingResult.Path->GetLength();
                if (Dist > MinDist && Dist < MaxDist && FMath::Abs((TestLocation.Z - Threat->GetActorLocation().Z)) < 100.0f)
                {
                    OutThreats.Add(Threat);
                }
            }
        }
    }
}

AThreatAwarenessActor* AWorldDataGenerator::GatherLeftMostThreat(TArray<AThreatAwarenessActor*> InThreats,
    FVector StartLocation, FVector ForwardVector, bool bFront, ADoor* Door, float MaxRightMostDist, AThreatAwarenessActor* PrevClearPoint)
{
    float BestDotProduct = 0.25f;
    AThreatAwarenessActor* BestThreat = nullptr;
    
    for (AThreatAwarenessActor* Threat : InThreats)
    {
        if (bFront && !Door->IsPointInFrontOfDoorway(Threat->GetActorLocation()))
            continue;

        if (!bFront && Door->IsPointInFrontOfDoorway(Threat->GetActorLocation()))
            continue;

        if (PrevClearPoint)
        {
            if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
            {
                FPathFindingQuery PathFindingQuery;
                PathFindingQuery.StartLocation = Threat->GetActorLocation();
                PathFindingQuery.EndLocation = PrevClearPoint->GetActorLocation();
                PathFindingQuery.SetAllowPartialPaths(false);
                const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_Swat::StaticClass();
                const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
                PathFindingQuery.QueryFilter = QueryFilter;
                const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Hierarchical);
                if (PathFindingResult.Result == ENavigationQueryResult::Success)
                {
                    if (PathFindingResult.Path->GetLength() > 3000.0f)
                        continue;
                }
            
          
            }
        }
        
        FVector v1 = ForwardVector;
        FVector v2 = UKismetMathLibrary::FindLookAtRotation(StartLocation, Threat->GetActorLocation()).Vector();
	    
        float DotProduct = FVector2D::DotProduct(FVector2D(v1.X, v1.Y), FVector2D(v2.X, v2.Y));
        if (DotProduct < BestDotProduct)
        {
            BestDotProduct = DotProduct;
            BestThreat = Threat;
        }
    }

   
    return BestThreat;
}

AThreatAwarenessActor* AWorldDataGenerator::GatherRightMostThreat(TArray<AThreatAwarenessActor*> InThreats,
    FVector StartLocation, FVector ForwardVector, bool bFront, ADoor* Door, float MaxLeftMostDist, AThreatAwarenessActor* PrevClearPoint)
{
    float BestDotProduct = -0.25f;
    AThreatAwarenessActor* BestThreat = nullptr;
  
    for (AThreatAwarenessActor* Threat : InThreats)
    {
        if (bFront && !Door->IsPointInFrontOfDoorway(Threat->GetActorLocation()))
            continue;

        if (!bFront && Door->IsPointInFrontOfDoorway(Threat->GetActorLocation()))
            continue;


        if (PrevClearPoint)
        {
            if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
            {
                FPathFindingQuery PathFindingQuery;
                PathFindingQuery.StartLocation = Threat->GetActorLocation();
                PathFindingQuery.EndLocation = PrevClearPoint->GetActorLocation();
                PathFindingQuery.SetAllowPartialPaths(false);
                const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_Swat::StaticClass();
                const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
                PathFindingQuery.QueryFilter = QueryFilter;
                const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Hierarchical);
                if (PathFindingResult.Result == ENavigationQueryResult::Success)
                {
                    if (PathFindingResult.Path->GetLength() > 3000.0f)
                        continue;
                }            
          
            }
        }
       

        FVector v1 = ForwardVector;
        FVector v2 = UKismetMathLibrary::FindLookAtRotation(StartLocation, Threat->GetActorLocation()).Vector();
	    
        float DotProduct = FVector2D::DotProduct(FVector2D(v1.X, v1.Y), FVector2D(v2.X, v2.Y));
        if (DotProduct > BestDotProduct)
        {
            BestDotProduct = DotProduct;
            BestThreat = Threat;
        }
    }


    return BestThreat;
}

AThreatAwarenessActor* AWorldDataGenerator::GatherAnyThreat(TArray<AThreatAwarenessActor*> InThreats, bool bFront,
    ADoor* Door)
{
    for (AThreatAwarenessActor* Threat : InThreats)
    {
        if (bFront && !Door->IsPointInFrontOfDoorway(Threat->GetActorLocation()))
            continue;

        if (!bFront && Door->IsPointInFrontOfDoorway(Threat->GetActorLocation()))
            continue;

        return Threat;
    }
    return nullptr;
}

void AWorldDataGenerator::TrimFarAwayStages(TArray<AThreatAwarenessActor*> InClearPoints,
    TArray<AThreatAwarenessActor*>& OutClearPoints)
{
    OutClearPoints = InClearPoints;
    for (int32 i = 1; i < InClearPoints.Num(); i++)
    {
        AThreatAwarenessActor* ClearPoint1 = InClearPoints[i-1];
        AThreatAwarenessActor* ClearPoint2 = InClearPoints[i];
        if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            FPathFindingQuery PathFindingQuery;
            PathFindingQuery.StartLocation = ClearPoint1->GetActorLocation();
            PathFindingQuery.EndLocation = ClearPoint2->GetActorLocation();
            PathFindingQuery.SetAllowPartialPaths(false);
            TSubclassOf<UNavigationQueryFilter> FilterClass = UNavigationQueryFilter::StaticClass();
            FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
            PathFindingQuery.QueryFilter = QueryFilter;
            FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
            if (PathFindingResult.Result == ENavigationQueryResult::Success)
            {
                float Dist = PathFindingResult.Path->GetLength();
                if (Dist > 1500.0f)
                {
                  OutClearPoints.Remove(ClearPoint2); 
                }
            }
        }
    }
    
}

AThreatAwarenessActor* AWorldDataGenerator::GetLastValidThreatInArray(TArray<AThreatAwarenessActor*> InThreats)
{
    AThreatAwarenessActor* OutThreat = nullptr;
    for (int32 i = 0; i < InThreats.Num(); i++)
    {
        if (InThreats[i])
        {
            OutThreat = InThreats[i];
        }
    }
    return OutThreat;
}

void AWorldDataGenerator::GenerateStackUpPoints()
{
    for (TActorIterator<AStackUpActor>It(GetWorld()); It; ++It)
    {
        AStackUpActor* StackUpActor = *It;
        StackUpActor->Destroy();
    }

    TArray<AActor*> OutDoors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADoor::StaticClass(), OutDoors);
    FScopedSlowTask SlowTask(OutDoors.Num(), NSLOCTEXT("GeneratingDoorStackPoints", "Generating Stack Points", "Generating Stack Points"));
    SlowTask.MakeDialog();
    for (int32 i = 0; i < OutDoors.Num(); i++)
    {
        ADoor* Door = Cast<ADoor>(OutDoors[i]);
        SlowTask.EnterProgressFrame();
        Door->FrontLeftStackUpPoints.Empty();
        Door->FrontRightStackUpPoints.Empty();
        Door->BackLeftStackUpPoints.Empty();
        Door->BackRightStackUpPoints.Empty();

        GenerateDoorStackUpsV2(Door);
    }
}

void AWorldDataGenerator::DestroyAllStackups()
{
    for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
    {
        ADoor* Door = *It;
        Door->FrontLeftStackUpPoints.Empty();
        Door->FrontRightStackUpPoints.Empty();
        Door->BackLeftStackUpPoints.Empty();
        Door->BackRightStackUpPoints.Empty();
    }

    for (TActorIterator<AStackUpActor>It(GetWorld()); It; ++It)
    {
        It->Destroy();
    }
}

bool AWorldDataGenerator::Internal_ProjectPointToNav(FVector Point, FVector& OutLocation, FVector Extent) const
{
    if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        FNavLocation NavLocation;
        if (NavSys->ProjectPointToNavigation(Point, NavLocation, Extent))
        {
            OutLocation = NavLocation.Location + FVector::UpVector * 10.0f; // up 10 units because there were instances where the point would be below the floor and thus have line of sight to things
            return true;
        }

        return false;
    }

    return false;
}

bool AWorldDataGenerator::Internal_PointHasLineOfSightToDoor(FVector Point, ADoor* Door, bool bFront) const
{
    FCollisionQueryParams CollisionQueryParams;
    CollisionQueryParams.AddIgnoredActor(Door);

    FVector ForwardOffset = Door->GetActorForwardVector() * 50.0f;
    
    if (!bFront)
        ForwardOffset = -Door->GetActorForwardVector() * 50.0f;

    FVector Start = Door->GetDoorMidLocation();
    Point.Z = Start.Z;
    
    return !GetWorld()->LineTraceTestByChannel(Start + ForwardOffset, Point, ECC_WorldStatic, CollisionQueryParams);
}

bool AWorldDataGenerator::Internal_PointHasLineOfSightTo(FVector Point, FVector OtherPoint) const
{
    return !GetWorld()->LineTraceTestByChannel(Point, OtherPoint, ECC_WorldStatic);
}

bool AWorldDataGenerator::Internal_FindPath(FVector From, FVector To, float MaxPathLength, TArray<FNavPathPoint>* OutPathPoints) const
{
    if (From == FVector::ZeroVector || To == FVector::ZeroVector)
        return false;
    
    // if the distance is so short, we prob don't need to path anyway
    if (FVector::Distance(From, To) < 100.0f)
    {
        if (!GetWorld()->LineTraceTestByChannel(From, To, ECC_WorldStatic))
            return true;
    }
    
    if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        FPathFindingQuery PathFindingQuery;
        PathFindingQuery.StartLocation = From;
        PathFindingQuery.EndLocation = To;
        PathFindingQuery.SetAllowPartialPaths(false);
        const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavigationQueryFilter::StaticClass();
        if (NavSys->MainNavData)
        {
            const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
            PathFindingQuery.QueryFilter = QueryFilter;
        }
        
        const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
        if (PathFindingResult.IsSuccessful())
        {
            if (OutPathPoints)
            {
                *OutPathPoints = PathFindingResult.Path->GetPathPoints();
            }
            
            if (PathFindingResult.Path->GetLength() > MaxPathLength)
                return false;

            return true;
        }
    }

    return false;
}

bool AWorldDataGenerator::Internal_FindPath_RoomGen(FVector From, FVector To) const
{
    if (From == FVector::ZeroVector || To == FVector::ZeroVector)
        return false;
    
    if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        FPathFindingQuery PathFindingQuery;
        PathFindingQuery.StartLocation = From;
        PathFindingQuery.EndLocation = To;
        PathFindingQuery.SetAllowPartialPaths(false);
        const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavigationQueryFilter::StaticClass();
        if (NavSys->MainNavData)
        {
            const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
            PathFindingQuery.QueryFilter = QueryFilter;
        }

        const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
        if (PathFindingResult.IsSuccessful() && !PathFindingResult.IsPartial())
        {
            return true;
        }
    }

    return false;
}

void AWorldDataGenerator::GenerateDoorStackUpsV2(ADoor* Door)
{
    // Front
    if (Door->bCanIssueOrdersOnFrontSide)
    {
        // Right stack ups
        if (Door->BackRoomPosition == EDoorRoomPosition::CornerLeft)
        {
            //float Slack = Door->GetDoorSize().Y - 50.0f;
            
            if (Door->GetSubDoor())
            {
                if (Door->IsMainSubdoor())
                {
                    GenerateDoorStackUps_Corner(Door->GetSubDoor(), true, false, 75.0f);
                    Door->FrontRightStackUpPoints = Door->GetSubDoor()->FrontRightStackUpPoints;
                }
            }
            else
            {
                GenerateDoorStackUps_Corner(Door, true, false, 75.0f);
            }
        }
        else if (Door->BackRoomPosition == EDoorRoomPosition::CornerRight)
        {
            /*
            float Slack = 0.0f;
            if (Door->GetDoorSize().Y > 75.0f)
                Slack = Door->GetDoorSize().Y-75.0f;
            */
            
            if (Door->GetSubDoor())
            {
                if (Door->IsMainSubdoor())
                {
                    GenerateDoorStackUps_Corner(Door, true, true, 75.0f);
                    Door->GetSubDoor()->FrontLeftStackUpPoints = Door->FrontLeftStackUpPoints;
                }
            }
            else
            {
                GenerateDoorStackUps_Corner(Door, true, true, 75.0f);
            }
        }
        else if (Door->BackRoomPosition == EDoorRoomPosition::Center)
        {
            if (Door->GetSubDoor())
            {
                if (Door->IsMainSubdoor())
                {
                    GenerateDoorStackUps_Center(Door->GetSubDoor(), true, false, 100.0f);
                    GenerateDoorStackUps_Center(Door, true, true, 100.0f);
                    Door->FrontRightStackUpPoints = Door->GetSubDoor()->FrontRightStackUpPoints;
                    Door->GetSubDoor()->FrontLeftStackUpPoints = Door->FrontLeftStackUpPoints;
                }
            }
            else
            {
                GenerateDoorStackUps_Center(Door, true, false, 100.0f);
                GenerateDoorStackUps_Center(Door, true, true, 100.0f);
            }
        }
        else if (Door->BackRoomPosition == EDoorRoomPosition::Hallway)
        {
            if (Door->GetSubDoor())
            {
                if (Door->IsMainSubdoor())
                {
                    GenerateDoorStackUps_Hallway(Door, true, true, 100.0f);
                    GenerateDoorStackUps_Hallway(Door->GetSubDoor(), true, false, 100.0f);
                    Door->FrontRightStackUpPoints = Door->GetSubDoor()->FrontRightStackUpPoints;
                    Door->GetSubDoor()->FrontLeftStackUpPoints = Door->FrontLeftStackUpPoints;
                }
            }
            else
            {
                GenerateDoorStackUps_Hallway(Door, true, true, 100.0f);
                GenerateDoorStackUps_Hallway(Door, true, false, 100.0f);
            }
        }
        else if (Door->BackRoomPosition == EDoorRoomPosition::HallwayRight)
        {
            if (Door->GetSubDoor())
            {
                if (Door->IsMainSubdoor())
                {
                    GenerateDoorStackUps_Hallway(Door, true, true, 100.0f);
                    Door->GetSubDoor()->FrontLeftStackUpPoints = Door->FrontLeftStackUpPoints;
                }
            }
            else
            {
                GenerateDoorStackUps_Hallway(Door, true, true, 100.0f);
            }
        }
        else if (Door->BackRoomPosition == EDoorRoomPosition::HallwayLeft)
        {
            if (Door->GetSubDoor())
            {
                if (Door->IsMainSubdoor())
                {
                    GenerateDoorStackUps_Hallway(Door->GetSubDoor(), true, false, 100.0f);
                    Door->FrontRightStackUpPoints = Door->GetSubDoor()->FrontRightStackUpPoints;
                }
            }
            else
            {
                GenerateDoorStackUps_Hallway(Door, true, false, 100.0f);
            }
        }
    }

    // Back
    if (Door->bCanIssueOrdersOnBackSide)
    {
        if (Door->FrontRoomPosition == EDoorRoomPosition::CornerLeft)
        {
            if (Door->GetSubDoor())
            {
                if (Door->IsMainSubdoor())
                {
                    GenerateDoorStackUps_Corner(Door, false, true, 75.0f);
                    Door->GetSubDoor()->BackLeftStackUpPoints = Door->BackLeftStackUpPoints;
                }
            }
            else
            {
                GenerateDoorStackUps_Corner(Door, false, true, 75.0f);
            }
        }
        else if (Door->FrontRoomPosition == EDoorRoomPosition::CornerRight)
        {
            if (Door->GetSubDoor())
            {
                if (Door->IsMainSubdoor())
                {
                    GenerateDoorStackUps_Corner(Door->GetSubDoor(), false, false, 75.0f);
                    Door->BackRightStackUpPoints = Door->GetSubDoor()->BackRightStackUpPoints;
                }
            }
            else
            {
                GenerateDoorStackUps_Corner(Door, false, false, 75.0f);
            }
        }
        else if (Door->FrontRoomPosition == EDoorRoomPosition::Center)
        {
            if (Door->GetSubDoor())
            {
                if (Door->IsMainSubdoor())
                {
                    GenerateDoorStackUps_Center(Door->GetSubDoor(), false, false, 100.0f);
                    GenerateDoorStackUps_Center(Door, false, true, 100.0f);
                    Door->BackRightStackUpPoints = Door->GetSubDoor()->BackRightStackUpPoints;
                    Door->GetSubDoor()->BackLeftStackUpPoints = Door->BackLeftStackUpPoints;
                }
            }
            else
            {
                GenerateDoorStackUps_Center(Door, false, false, 100.0f);
                GenerateDoorStackUps_Center(Door, false, true, 100.0f);
            }
        }
        else if (Door->FrontRoomPosition == EDoorRoomPosition::Hallway)
        {
            if (Door->GetSubDoor())
            {
                if (Door->IsMainSubdoor())
                {
                    GenerateDoorStackUps_Hallway(Door->GetSubDoor(), false, false, 100.0f);
                    GenerateDoorStackUps_Hallway(Door, false, true, 100.0f);
                    Door->BackRightStackUpPoints = Door->GetSubDoor()->BackRightStackUpPoints;
                    Door->GetSubDoor()->BackLeftStackUpPoints = Door->BackLeftStackUpPoints;
                }
            }
            else
            {
                GenerateDoorStackUps_Hallway(Door, false, false, 100.0f);
                GenerateDoorStackUps_Hallway(Door, false, true, 100.0f);
            }
        }
        else if (Door->FrontRoomPosition == EDoorRoomPosition::HallwayRight)
        {
            if (Door->GetSubDoor())
            {
                if (Door->IsMainSubdoor())
                {
                    GenerateDoorStackUps_Hallway(Door, false, false, 100.0f);
                    Door->GetSubDoor()->BackRightStackUpPoints = Door->BackRightStackUpPoints;
                }
            }
            else
            {
                GenerateDoorStackUps_Hallway(Door, false, false, 100.0f);
            }
        }
        else if (Door->FrontRoomPosition == EDoorRoomPosition::HallwayLeft)
        {
            if (Door->GetSubDoor())
            {
                if (Door->IsMainSubdoor())
                {
                    GenerateDoorStackUps_Hallway(Door->GetSubDoor(), false, true, 100.0f);
                    Door->BackLeftStackUpPoints = Door->GetSubDoor()->BackLeftStackUpPoints;
                }
            }
            else
            {
                GenerateDoorStackUps_Hallway(Door, false, true, 100.0f);
            }
        }
    }
}

void AWorldDataGenerator::GenerateDoorStackUps_Center(ADoor* Door, bool bFront, bool bLeft, float ForwardOffset)
{
    FVector DoorForward = -Door->GetActorForwardVector();
    if (bFront)
        DoorForward = Door->GetActorForwardVector();

    FVector DoorRight = Door->GetActorRightVector();
    if (bLeft)
        DoorRight = -Door->GetActorRightVector();
    
    if (Door->GetSubDoor() && Door->IsNonMainSubdoor())
    {
        DoorForward = -DoorForward;
        DoorRight = -DoorRight;
    }

    constexpr float ElementSpacing = 85.0f;
    constexpr float PointSize = 20.0f;
    constexpr float Lifetime = 10.0f;
    constexpr float DotThreshold = 0.05f;
    constexpr float ElementBounds = 38.0f;

	uint8 Depth = 0;

    FVector DoorLocation = Door->GetDoorMidLocation();
    DoorLocation.Z = Door->GetActorLocation().Z;

    ForwardOffset = 75.0f;

    FVector AlphaLocation = DoorLocation + DoorForward * ForwardOffset;
    FVector AlphaLocation_Adjusted = AlphaLocation;
    FVector FirstLocation = AlphaLocation;
    
    if (!Internal_ProjectPointToNav(AlphaLocation, AlphaLocation_Adjusted))
    {
        return;
    }

    float DoorWidth = 100.0f;
    if (Door->GetDoorSize().Y > 75.0f)
        DoorWidth += Door->GetDoorSize().Y-75.0f;

    DoorWidth += (75*Door->GetActorScale3D().Y)-75.0f;
    
    {
        uint8 i = 0;
        while (Internal_ProjectPointToNav(AlphaLocation, AlphaLocation_Adjusted) &&
               FVector::Distance(FirstLocation, AlphaLocation_Adjusted) < DoorWidth && i < 20) // cap iterations
        {
            DrawDebugPoint(GetWorld(), AlphaLocation, PointSize, FColor::Blue, false, Lifetime);
            AlphaLocation = AlphaLocation + DoorRight * 20.0f; // keep searching right
            i++;
        }

        // move forward once again
        if (i == 20 || FVector::Distance(FirstLocation, AlphaLocation_Adjusted) < 50.0f)
        {
            AlphaLocation = FirstLocation + DoorForward * (ElementSpacing/2); // keep searching right
            FirstLocation = AlphaLocation;

            i = 0;
            while (Internal_ProjectPointToNav(AlphaLocation, AlphaLocation_Adjusted) &&
                    FVector::Distance(FirstLocation, AlphaLocation) < DoorWidth && i < 20) // cap iterations
            {
                DrawDebugPoint(GetWorld(), AlphaLocation, PointSize, FColor::Blue, false, Lifetime);
                AlphaLocation = AlphaLocation + DoorRight * 20.0f; // keep searching right
                i++;
            }
        }
    }

    if (FVector::Distance(FirstLocation, AlphaLocation_Adjusted) < 25.0f)
    {
        return;
    }
    
    SpawnStackUpActorAtLocation(AlphaLocation_Adjusted, Door, ESquadPosition::SP_Alpha, bFront, bLeft, Depth);
    
    FBoxSphereBounds AlphaBounds = FBoxSphereBounds(AlphaLocation_Adjusted, FVector(ElementBounds), ElementBounds);

    bool bBetaSuccess = true;
    FVector BetaLocation = AlphaLocation_Adjusted + DoorRight * ElementSpacing;
    FVector BetaLocation_Adjusted = BetaLocation;

    if (!Internal_ProjectPointToNav(BetaLocation + -DoorForward * 10.0f, BetaLocation_Adjusted) ||
        !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted) ||
        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(BetaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds))
    {
        //DrawDebugPoint(GetWorld(), BetaLocation, PointSize, FColor::Green, false, Lifetime);
        BetaLocation = BetaLocation + DoorForward * ElementSpacing;

		Depth++;
        
        uint8 i = 0;
        
        while ((!Internal_ProjectPointToNav(BetaLocation, BetaLocation_Adjusted) ||
                !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted) ||
                FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(BetaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds)) && i < 1) // cap iterations
        {
            DrawDebugPoint(GetWorld(), BetaLocation, PointSize, FColor::Green, false, Lifetime);
            BetaLocation = BetaLocation + DoorForward * 25.0f;
            i++;
        }

        if (i == 1)
        {
            BetaLocation = AlphaLocation_Adjusted + DoorForward * ElementSpacing + DoorRight * (ElementSpacing/2);
            
            if (!Internal_ProjectPointToNav(BetaLocation, BetaLocation_Adjusted) ||
                !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted) ||
                FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(BetaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds))
            {
                BetaLocation = AlphaLocation_Adjusted + DoorForward * ElementSpacing;
                
                i = 0;
                while ((!Internal_ProjectPointToNav(BetaLocation, BetaLocation_Adjusted) ||
                        !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted) ||
                        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(BetaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds)) && i < 5) // cap iterations
                {
                    //DrawDebugPoint(GetWorld(), BetaLocation, PointSize, FColor::Green, false, Lifetime);
                    BetaLocation = BetaLocation + DoorForward * ElementSpacing;
                    i++;
                }
            }

            bool bIsFar = FVector::Distance(AlphaLocation_Adjusted, BetaLocation_Adjusted) > 300.0f;

            if (i == 5 || bIsFar)
            {
                BetaLocation = AlphaLocation_Adjusted + -DoorRight * (ElementSpacing/2) + DoorForward * ElementSpacing;
                
                i = 0;
                while ((!Internal_ProjectPointToNav(BetaLocation, BetaLocation_Adjusted) ||
                        !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted) ||
                        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(BetaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds)) && i < 2) // cap iterations
                {
                    //DrawDebugPoint(GetWorld(), BetaLocation, PointSize, FColor::Green, false, Lifetime);
                    BetaLocation = BetaLocation + DoorForward * ElementSpacing;
                    i++;
                }


                if (i == 2)
                {
                    BetaLocation = AlphaLocation_Adjusted + DoorForward * ElementSpacing;
                    
                    if (!Internal_ProjectPointToNav(BetaLocation, BetaLocation_Adjusted) ||
                        !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted) ||
                        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(BetaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds))
                    {
                        //DrawDebugPoint(GetWorld(), BetaLocation, PointSize, FColor::Green, false, Lifetime);
                        BetaLocation = AlphaLocation_Adjusted + DoorForward * ElementSpacing + DoorRight * (ElementSpacing/2);
                        if (!Internal_ProjectPointToNav(BetaLocation, BetaLocation_Adjusted))
                        {
                            bBetaSuccess = false;
                        }
                    }
                }
            }
        }
    }
    
    if (FVector::Distance(BetaLocation_Adjusted, AlphaLocation_Adjusted) > 300.0f)
        bBetaSuccess = false;
    
    // crossed over to the other side or in middle of door
    if (FVector::DotProduct(DoorRight, (BetaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold)
        bBetaSuccess = false;

    if (bBetaSuccess)
    {
        SpawnStackUpActorAtLocation(BetaLocation_Adjusted, Door, ESquadPosition::SP_Beta, bFront, bLeft, Depth);
    }
    else
    {
        return;
    }
    
    FBoxSphereBounds BetaBounds = FBoxSphereBounds(BetaLocation_Adjusted, FVector(ElementBounds), ElementBounds);

    bool bCharlieSuccess = true;
    FVector CharlieLocation = BetaLocation_Adjusted + DoorRight * (ElementSpacing/2) + DoorForward * ElementSpacing;
    FVector CharlieLocation_Adjusted = CharlieLocation;
    
	if (!Internal_ProjectPointToNav(CharlieLocation, CharlieLocation_Adjusted) ||
		!Internal_FindPath(BetaLocation_Adjusted, CharlieLocation_Adjusted) ||
		FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
		FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds))
	{
		DrawDebugPoint(GetWorld(), CharlieLocation, PointSize, FColor::Red, false, Lifetime);
		CharlieLocation = BetaLocation_Adjusted + DoorForward * ElementSpacing;
        
		if ((!Internal_ProjectPointToNav(CharlieLocation, CharlieLocation_Adjusted) ||
			!Internal_FindPath(BetaLocation_Adjusted, CharlieLocation_Adjusted) ||
			FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
			FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds)))
		{
			DrawDebugPoint(GetWorld(), CharlieLocation, PointSize, FColor::Red, false, Lifetime);
            
			CharlieLocation = BetaLocation_Adjusted + DoorRight * ElementSpacing;
            
			uint8 i = 0;
			while ((!Internal_ProjectPointToNav(CharlieLocation, CharlieLocation_Adjusted) ||
				!Internal_FindPath(BetaLocation_Adjusted, CharlieLocation_Adjusted) ||
				FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
				FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds)) && i < 5) // cap iterations
			{
				DrawDebugPoint(GetWorld(), CharlieLocation, PointSize, FColor::Red, false, Lifetime);
				CharlieLocation = CharlieLocation + DoorRight * ElementSpacing;
				i++;
			}

			bool bIsFar = FVector::Distance(BetaLocation_Adjusted, CharlieLocation_Adjusted) > 300.0f;
            
			// walk backwards from beta location
			if (i == 5 || bIsFar)
			{
				CharlieLocation = BetaLocation_Adjusted + DoorForward * ElementSpacing;
				Depth++;
                
				i = 0;
				while ((!Internal_ProjectPointToNav(CharlieLocation, CharlieLocation_Adjusted) ||
					!Internal_FindPath(BetaLocation_Adjusted, CharlieLocation_Adjusted) ||
					FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
					FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds)) && i < 5) // cap iterations
				{
					//DrawDebugPoint(GetWorld(), CharlieLocation, PointSize, FColor::Red, false, Lifetime);
					CharlieLocation = CharlieLocation + DoorForward * ElementSpacing;
					i++;
				}

				bIsFar = FVector::Distance(BetaLocation_Adjusted, CharlieLocation_Adjusted) > 300.0f && Internal_PointHasLineOfSightToDoor(CharlieLocation, Door, bFront);

				// walk backwards from slight left of beta location
				if (i == 5 || bIsFar)
				{
					CharlieLocation = BetaLocation_Adjusted + DoorForward * ElementSpacing + -DoorRight * (ElementSpacing / 2);
                    
					i = 0;
					while ((!Internal_ProjectPointToNav(CharlieLocation, CharlieLocation_Adjusted) ||
						!Internal_FindPath(BetaLocation_Adjusted, CharlieLocation_Adjusted) ||
						FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
						FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds)) && i < 5) // cap iterations
					{
						//DrawDebugPoint(GetWorld(), CharlieLocation, PointSize, FColor::Red, false, Lifetime);
						CharlieLocation = CharlieLocation + DoorForward * ElementSpacing;
						i++;
					}
                    
					if (i == 5)
					{
						bCharlieSuccess = false;
					}
				}
			}
		}
		else
		{
			Depth++;
		}
    }
	else
	{
		Depth++;
	}

    {
        bool bIsFar = FVector::Distance(BetaLocation_Adjusted, CharlieLocation_Adjusted) > 300.0f && Internal_PointHasLineOfSightToDoor(CharlieLocation, Door, bFront);
        if (bIsFar)
            bCharlieSuccess = false;
    }
    
    // crossed over to the other side or in middle of door
    if (FVector::DotProduct(DoorRight, (CharlieLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold)
        bCharlieSuccess = false;

    if (bCharlieSuccess)
    {
        SpawnStackUpActorAtLocation(CharlieLocation_Adjusted, Door, ESquadPosition::SP_Charlie, bFront, bLeft, Depth);
    }
    else
    {
        return;
    }
    
    FBoxSphereBounds CharlieBounds = FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds);

    bool bDeltaSuccess = true;
    FVector DeltaLocation = CharlieLocation_Adjusted + DoorRight * ElementSpacing;
    FVector DeltaLocation_Adjusted = DeltaLocation;
    
    if (!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
        !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted) ||
        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||  
        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||  
        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
        (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
        Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront)))
    {
        DrawDebugPoint(GetWorld(), DeltaLocation, PointSize, FColor::Yellow, false, Lifetime);
        DeltaLocation = CharlieLocation_Adjusted + DoorRight * (ElementSpacing/2) + DoorForward * ElementSpacing;
        
        if ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
            !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted) ||
            FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||  
            FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||  
            FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
            (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
            Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront))))
        {
            DrawDebugPoint(GetWorld(), DeltaLocation, PointSize, FColor::Yellow, false, Lifetime);
            DeltaLocation = CharlieLocation_Adjusted + DoorRight * ElementSpacing;
            
            // try to expand right, twice
            int32 i = 0;
            while ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
                    !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted) ||
                    FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||  
                    FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||  
                    FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
                    (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
                    Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront))) && i < 2) // cap iterations
            {
                //DrawDebugPoint(GetWorld(), DeltaLocation, PointSize, FColor::Yellow, false, Lifetime);
                DeltaLocation = DeltaLocation + DoorRight * ElementSpacing;
                i++;
            }

            if (i == 2)
            {
                DeltaLocation = CharlieLocation_Adjusted + DoorForward * ElementSpacing;
				Depth++;
                
                {
                    i = 0;
                    while ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
                            !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted)
                            /*!(Internal_PointHasLineOfSightTo(DeltaLocation_Adjusted, CharlieLocation_Adjusted) ||
                            Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront) ||
                            Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted))*/ ||
                            FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||  
                            FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||  
                            FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
                            (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
                            Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront))) && i < 4) // cap iterations
                    {
                        DrawDebugPoint(GetWorld(), DeltaLocation, PointSize, FColor::Yellow, false, Lifetime);
                        DeltaLocation = DeltaLocation + DoorForward * ElementSpacing;
                        i++;
                    }

                    bool bIsFar = FVector::Distance(CharlieLocation_Adjusted, DeltaLocation_Adjusted) > 300.0f && Internal_PointHasLineOfSightToDoor(DeltaLocation, Door, bFront);
                    
                    if (i == 4 || bIsFar)
                    {
                        DeltaLocation = CharlieLocation_Adjusted + (DoorForward + -DoorRight) * (ElementSpacing/2);
                        
                        i = 0;
                        while ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
                                !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted)
                                /*!(Internal_PointHasLineOfSightTo(DeltaLocation_Adjusted, CharlieLocation_Adjusted) ||
                                Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront) ||
                                Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted))*/ ||
                                FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||  
                                FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||  
                                FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
                                (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
                                Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront))) && i < 4) // cap iterations
                        {
                            DrawDebugPoint(GetWorld(), DeltaLocation, PointSize, FColor::Yellow, false, Lifetime);
                            DeltaLocation = DeltaLocation + DoorForward * ElementSpacing;
                            i++;
                        }

                        if (i == 4)
                        {
                            DeltaLocation = CharlieLocation_Adjusted + DoorRight * ElementSpacing + DoorForward * (ElementSpacing/2);
                            
                            i = 0;
                            while ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
                                    !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted)
                                    /*!(Internal_PointHasLineOfSightTo(DeltaLocation_Adjusted, CharlieLocation_Adjusted) ||
                                    Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted))*/ ||
                                    FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||  
                                    FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||  
                                    FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
                                    (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
                                    Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront))) && i < 5) // cap iterations
                            {
                                DrawDebugPoint(GetWorld(), DeltaLocation, PointSize, FColor::Yellow, false, Lifetime);
                                DeltaLocation = DeltaLocation + DoorRight * ElementSpacing + DoorForward * (ElementSpacing/2);
                                i++;
                            }

                            if (i == 5)
                            {
                                DeltaLocation = CharlieLocation_Adjusted + DoorRight * ElementSpacing + -DoorForward * (ElementSpacing/2);
                                
                                i = 0;
                                while ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
                                        !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted)
                                        /*!(Internal_PointHasLineOfSightTo(DeltaLocation_Adjusted, CharlieLocation_Adjusted) ||
                                        Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted))*/ ||
                                        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||  
                                        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||  
                                        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
                                        (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
                                        Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront))) && i < 5) // cap iterations
                                {
                                    DrawDebugPoint(GetWorld(), DeltaLocation, PointSize, FColor::Yellow, false, Lifetime);
                                    DeltaLocation = DeltaLocation + DoorRight * ElementSpacing + -DoorForward * (ElementSpacing/2);
                                    i++;
                                }

                                if (i == 5)
                                {
                                    DeltaLocation = CharlieLocation_Adjusted + DoorForward * 50.0f + -DoorRight * 25.0f;
                                    
                                    i = 0;
                                    while ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
                                            !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted)
                                            /*!(Internal_PointHasLineOfSightTo(DeltaLocation_Adjusted, CharlieLocation_Adjusted) ||
                                            Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted))*/ ||
                                            FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||  
                                            FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||  
                                            FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
                                            (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
                                            Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront))) && i < 3) // cap iterations
                                    {
                                        DrawDebugPoint(GetWorld(), DeltaLocation, PointSize, FColor::Yellow, false, Lifetime);
                                        DeltaLocation = DeltaLocation + DoorForward * 50.0f + -DoorRight * 25.0f;
                                        i++;
                                    }

                                    if (i == 3)
                                    {
                                        bDeltaSuccess = false;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
		else
		{
			Depth++;
		}
    }

    bool bHasLOS = Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront);
    bool bIsFar = FVector::Distance(CharlieLocation_Adjusted, DeltaLocation_Adjusted) > 300.0f && bHasLOS;
    if (bIsFar)
        bDeltaSuccess = false;

    // crossed over to the other side or in middle of door
    if (bHasLOS)
    {
        if (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold)
            bDeltaSuccess = false;
    }

    if (bDeltaSuccess)
        SpawnStackUpActorAtLocation(DeltaLocation_Adjusted, Door, ESquadPosition::SP_Delta, bFront, bLeft, Depth);
}

void AWorldDataGenerator::GenerateDoorStackUps_Hallway(ADoor* Door, bool bFront, bool bLeft, float ForwardOffset)
{
    FVector DoorForward = -Door->GetActorForwardVector();
    if (bFront)
        DoorForward = Door->GetActorForwardVector();

    FVector DoorRight = Door->GetActorRightVector();
    if (bLeft)
        DoorRight = -Door->GetActorRightVector();

    if (Door->GetSubDoor() && Door->IsNonMainSubdoor())
    {
        DoorForward = -DoorForward;
        DoorRight = -DoorRight;
    }
    
    ForwardOffset = 75.0f;
    
    constexpr float ElementSpacing = 85.0f;
    constexpr float PointSize = 20.0f;
    constexpr float Lifetime = 10.0f;
    constexpr float ElementBounds = 38.0f;

	uint8 Depth = 0;
    
    FVector DoorLocation = Door->GetDoorMidLocation();
    DoorLocation.Z = Door->GetActorLocation().Z;

    FVector AlphaLocation = DoorLocation + DoorForward * ForwardOffset;
    FVector AlphaLocation_Adjusted = AlphaLocation;
    FVector FirstLocation = AlphaLocation;
    
    if (!Internal_ProjectPointToNav(AlphaLocation, AlphaLocation_Adjusted))
    {
        DrawDebugPoint(GetWorld(), AlphaLocation_Adjusted, PointSize, FColor::Blue, false, Lifetime);
        return;
    }
    
    float DoorWidth = 100.0f;
    if (Door->GetDoorSize().Y > 75.0f)
        DoorWidth += Door->GetDoorSize().Y-75.0f;

    DoorWidth += (75*Door->GetActorScale3D().Y)-75.0f;
    
    {
        int32 i = 0;
        while (Internal_ProjectPointToNav(AlphaLocation, AlphaLocation_Adjusted) &&
                FVector::Distance(FirstLocation, AlphaLocation) < DoorWidth && i < 20) // cap iterations
        {
            AlphaLocation = AlphaLocation + DoorRight * 20.0f; // keep searching right
            i++;
        }
    }

    FBoxSphereBounds AlphaBounds = FBoxSphereBounds(AlphaLocation_Adjusted, FVector(ElementBounds), ElementBounds);
    
    // check if other side has an alpha point and if we're overlapping it
    if (bLeft)
    {
        if (bFront)
        {
            if (Door->FrontRightStackUpPoints.Num() > 0)
            {
                if (FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(Door->FrontRightStackUpPoints[0]->GetActorLocation(), FVector(ElementBounds), ElementBounds), AlphaBounds))
                {
                    return;
                }
            }
        }
        else
        {
            
            if (Door->BackRightStackUpPoints.Num() > 0)
            {
                if (FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(Door->BackRightStackUpPoints[0]->GetActorLocation(), FVector(ElementBounds), ElementBounds), AlphaBounds))
                {
                    return;
                }
            }
        }
    }
    else
    {
        if (bFront)
        {
            if (Door->FrontLeftStackUpPoints.Num() > 0)
            {
                if (FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(Door->FrontLeftStackUpPoints[0]->GetActorLocation(), FVector(ElementBounds), ElementBounds), AlphaBounds))
                {
                    return;
                }
            }
        }
        else
        {
            
            if (Door->BackLeftStackUpPoints.Num() > 0)
            {
                if (FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(Door->BackLeftStackUpPoints[0]->GetActorLocation(), FVector(ElementBounds), ElementBounds), AlphaBounds))
                {
                    return;
                }
            }
        }
    }

    SpawnStackUpActorAtLocation(AlphaLocation_Adjusted, Door, ESquadPosition::SP_Alpha, bFront, bLeft, Depth);

    bool bBetaSuccess = true;
    FVector BetaLocation = AlphaLocation_Adjusted + DoorForward * ElementSpacing;
    FVector BetaLocation_Adjusted = BetaLocation;

    {
        uint8 i = 0;

		Depth++;
        
        BetaLocation = BetaLocation + DoorRight * (ElementSpacing/2);
        
        while ((!Internal_ProjectPointToNav(BetaLocation, BetaLocation_Adjusted) ||
                !Internal_PointHasLineOfSightTo(BetaLocation_Adjusted, AlphaLocation_Adjusted) ||
                !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted)) && i < 5)
        {
            BetaLocation = BetaLocation + DoorRight * (ElementSpacing/2);
            i++;
        }

        if (i == 5)
        {
            BetaLocation = AlphaLocation_Adjusted + DoorForward * ElementSpacing;
        
            i = 0;
            while ((!Internal_ProjectPointToNav(BetaLocation, BetaLocation_Adjusted) ||
                    !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted)) && i < 5)
            {
                BetaLocation = BetaLocation + DoorForward * ElementSpacing;
                i++;
            }

            if (i == 5)
            {
                BetaLocation = AlphaLocation_Adjusted + DoorForward * ElementSpacing;
                BetaLocation = BetaLocation + -DoorRight * (ElementSpacing/2);

                i = 0;
                while ((!Internal_ProjectPointToNav(BetaLocation, BetaLocation_Adjusted) ||
                        !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted)) && i < 5)
                {
                    BetaLocation = BetaLocation + DoorForward * ElementSpacing;
                    i++;
                }

                if (i == 5)
                {
                    bBetaSuccess = false;
                }
            }
        }
    }

    if (bBetaSuccess)
    {
        SpawnStackUpActorAtLocation(BetaLocation_Adjusted, Door, ESquadPosition::SP_Beta, bFront, bLeft, Depth);
    }
    else
    {
        return;
    }

    bool bCharlieSuccess = true;
    FVector CharlieLocation = BetaLocation_Adjusted + DoorForward * ElementSpacing;
    FVector CharlieLocation_Adjusted = CharlieLocation;
    
    {
        uint8 i = 0;

		Depth++;
        
        CharlieLocation = CharlieLocation + DoorRight * (ElementSpacing/2);
        
        while ((!Internal_ProjectPointToNav(CharlieLocation, CharlieLocation_Adjusted) ||
                !Internal_PointHasLineOfSightTo(CharlieLocation_Adjusted, BetaLocation_Adjusted) ||
                !Internal_FindPath(BetaLocation_Adjusted, CharlieLocation_Adjusted)) && i < 5)
        {
            DrawDebugPoint(GetWorld(), CharlieLocation, PointSize, FColor::Red, false, Lifetime);
            CharlieLocation = CharlieLocation + DoorRight * (ElementSpacing/2);
            i++;
        }

        if (i == 5)
        {
            CharlieLocation = BetaLocation_Adjusted + DoorForward * ElementSpacing;
            
            i = 0;
            while ((!Internal_ProjectPointToNav(CharlieLocation, CharlieLocation_Adjusted) ||
                    !Internal_FindPath(BetaLocation_Adjusted, CharlieLocation_Adjusted)) && i < 5)
            {
                DrawDebugPoint(GetWorld(), CharlieLocation, PointSize, FColor::Red, false, Lifetime);
                CharlieLocation = CharlieLocation + DoorForward * ElementSpacing;
                i++;
            }

            if (i == 5)
            {
                bCharlieSuccess = false;
            }
        }
    }

    if (bCharlieSuccess)
    {
        SpawnStackUpActorAtLocation(CharlieLocation_Adjusted, Door, ESquadPosition::SP_Charlie, bFront, bLeft, Depth);
    }
    else
    {
        return;
    }

    bool bDeltaSuccess = true;
    FVector DeltaLocation = CharlieLocation_Adjusted + DoorForward * ElementSpacing;
    FVector DeltaLocation_Adjusted = DeltaLocation;
    
    {
        uint8 i = 0;

		Depth++;
        
        DeltaLocation = DeltaLocation + DoorRight * (ElementSpacing/2);
        
        while ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
                !Internal_PointHasLineOfSightTo(DeltaLocation_Adjusted, CharlieLocation_Adjusted) ||
                !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted)) && i < 5)
        {
            DeltaLocation = DeltaLocation + DoorRight * (ElementSpacing/2);
            i++;
        }

        if (i == 5)
        {
            DeltaLocation = CharlieLocation_Adjusted + DoorForward * ElementSpacing;
            
            i = 0;
            while ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
                    !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted)) && i < 5)
            {
                DeltaLocation = DeltaLocation + DoorForward * ElementSpacing;
                i++;
            }

            if (i == 5)
            {
                bDeltaSuccess = false;
            }
        }
    }

    if (bDeltaSuccess)
    {
        SpawnStackUpActorAtLocation(DeltaLocation_Adjusted, Door, ESquadPosition::SP_Delta, bFront, bLeft, Depth);
    }
}

void AWorldDataGenerator::GenerateDoorStackUps_Corner(ADoor* Door, bool bFront, bool bLeft, float ForwardOffset)
{
    FVector DoorForward = -Door->GetActorForwardVector();
    if (bFront)
        DoorForward = Door->GetActorForwardVector();

    FVector DoorRight = Door->GetActorRightVector();
    if (bLeft)
        DoorRight = -Door->GetActorRightVector();

    if (Door->GetSubDoor() && Door->IsNonMainSubdoor())
    {
        DoorForward = -DoorForward;
        DoorRight = -DoorRight;
    }
    
    ForwardOffset = 75.0f;
    
    constexpr float ElementSpacing = 85.0f;
    constexpr float ElementBounds = 38.0f;
    constexpr float PointSize = 20.0f;
    constexpr float Lifetime = 10.0f;
    constexpr float DotThreshold = 0.05f;
    
	uint8 Depth = 0;

    FVector DoorLocation = Door->GetDoorMidLocation();
    DoorLocation.Z = Door->GetActorLocation().Z;
    
    FVector AlphaLocation = DoorLocation + DoorForward * ForwardOffset;
    FVector AlphaLocation_Adjusted = AlphaLocation;
    FVector FirstLocation = AlphaLocation;
    
    if (!Internal_ProjectPointToNav(AlphaLocation, AlphaLocation_Adjusted))
    {
        return;
    }
    
    float DoorWidth = 100.0f;
    if (Door->GetDoorSize().Y > 75.0f)
        DoorWidth += Door->GetDoorSize().Y-75.0f;

    DoorWidth += (75*Door->GetActorScale3D().Y)-75.0f;

    /*
    {
        int32 i = 0;
        while (Internal_ProjectPointToNav(AlphaLocation, AlphaLocation_Adjusted) &&
                FVector::Distance(FirstLocation, AlphaLocation) < DoorWidth && i < 20) // cap iterations
        {
            AlphaLocation = AlphaLocation + DoorRight * 20.0f; // keep searching right
            i++;
        }
    }
    */
    
    {
        uint8 i = 0;
        while (Internal_ProjectPointToNav(AlphaLocation, AlphaLocation_Adjusted) &&
               FVector::Distance(FirstLocation, AlphaLocation_Adjusted) < DoorWidth && i < 20) // cap iterations
        {
            DrawDebugPoint(GetWorld(), AlphaLocation, PointSize, FColor::Blue, false, Lifetime);
            AlphaLocation = AlphaLocation + DoorRight * 20.0f; // keep searching right
            i++;
        }

        // move forward once again
        if (i == 20 || FVector::Distance(FirstLocation, AlphaLocation_Adjusted) < 50.0f)
        {
            AlphaLocation = FirstLocation + DoorForward * (ElementSpacing/2); // keep searching right
            FirstLocation = AlphaLocation;

            i = 0;
            while (Internal_ProjectPointToNav(AlphaLocation, AlphaLocation_Adjusted) &&
                    FVector::Distance(FirstLocation, AlphaLocation) < DoorWidth && i < 20) // cap iterations
            {
                DrawDebugPoint(GetWorld(), AlphaLocation, PointSize, FColor::Blue, false, Lifetime);
                AlphaLocation = AlphaLocation + DoorRight * 20.0f; // keep searching right
                i++;
            }
        }
    }

    if (FVector::Distance(FirstLocation, AlphaLocation_Adjusted) < 25.0f)
    {
        // Standing in front of the door... not good.
        return;
    }
    
    SpawnStackUpActorAtLocation(AlphaLocation_Adjusted, Door, ESquadPosition::SP_Alpha, bFront, bLeft, Depth);
    
    FBoxSphereBounds AlphaBounds = FBoxSphereBounds(AlphaLocation_Adjusted, FVector(ElementBounds), ElementBounds);

    bool bBetaSuccess = true;
    FVector BetaLocation = AlphaLocation_Adjusted + DoorRight * (ElementSpacing/2);
    FVector BetaLocation_Adjusted = BetaLocation;

    if (!Internal_ProjectPointToNav(BetaLocation + DoorForward * ElementSpacing, BetaLocation_Adjusted) ||
        !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted, 500.0f) ||
        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(BetaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds))
    {
        BetaLocation = BetaLocation + DoorForward * ElementSpacing;

        {
            uint8 i = 0;
            while ((!Internal_ProjectPointToNav(BetaLocation, BetaLocation_Adjusted) ||
                    !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted, 500.0f) ||
                    FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(BetaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds)) && i < 5)
            {
                BetaLocation = BetaLocation + DoorForward * 20.0f;
                i++;
            }

            // try expand right, then left
            if (i == 5)
            {
                {
                    BetaLocation = AlphaLocation_Adjusted + DoorRight * ElementSpacing;
                    
                    i = 0;
                    while ((!Internal_ProjectPointToNav(BetaLocation, BetaLocation_Adjusted) ||
                            !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted, 500.0f) ||
                            FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(BetaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds)) && i < 3)
                    {
                        DrawDebugPoint(GetWorld(), BetaLocation, PointSize, FColor::Green, false, Lifetime);
                        BetaLocation = BetaLocation + DoorRight * ElementSpacing;
                        i++;
                    }
        
                    if (i == 3)
                    {
                        BetaLocation = AlphaLocation_Adjusted + DoorRight * ElementSpacing + DoorForward * (ElementSpacing/2);
                        
                        i = 0;
                        while ((!Internal_ProjectPointToNav(BetaLocation, BetaLocation_Adjusted) ||
                                !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted, 500.0f) ||
                                FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(BetaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds)) && i < 5)
                        {
                            DrawDebugPoint(GetWorld(), BetaLocation, PointSize, FColor::Green, false, Lifetime);
                            BetaLocation = BetaLocation + DoorRight * ElementSpacing + DoorForward * (ElementSpacing/2);
                            i++;
                        }

                        if (i == 5)
                        {
							Depth++;
                            BetaLocation = AlphaLocation_Adjusted + DoorForward * ElementSpacing;
                            
                            i = 0;
                            while ((!Internal_ProjectPointToNav(BetaLocation, BetaLocation_Adjusted) ||
                                !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted, 500.0f) ||
                                FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(BetaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds)) && i < 5)
                            {
                                DrawDebugPoint(GetWorld(), BetaLocation, PointSize, FColor::Green, false, Lifetime);
                                BetaLocation = BetaLocation + -DoorRight * 20.0f;
                                i++;
                            }
                            
                            if (i == 5)
                            {
                                bBetaSuccess = false;
                            }
                        }
						else
						{
							if (i > 0)
								Depth++;
						}
                    }
                }
            }
            else
            {
				Depth++;

                // keep searching right
                i = 0;
                while ((!Internal_ProjectPointToNav(BetaLocation, BetaLocation_Adjusted) ||
                        !Internal_FindPath(AlphaLocation_Adjusted, BetaLocation_Adjusted, 500.0f) ||
                        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(BetaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds)) && i < 5)
                {
                    DrawDebugPoint(GetWorld(), BetaLocation, PointSize, FColor::Green, false, Lifetime);
                    BetaLocation = BetaLocation + DoorRight * 20.0f;
                    i++;
                }
            }
        }
    }
	else
	{
		Depth++;
	}

    if (bBetaSuccess)
    {
        SpawnStackUpActorAtLocation(BetaLocation_Adjusted, Door, ESquadPosition::SP_Beta, bFront, bLeft, Depth);
    }
    else
    {
        return;
    }
    
    FBoxSphereBounds BetaBounds = FBoxSphereBounds(BetaLocation_Adjusted, FVector(ElementBounds), ElementBounds);

    bool bCharlieSuccess = true;
    FVector CharlieLocation = BetaLocation_Adjusted + DoorRight * (ElementSpacing/2);
    FVector CharlieLocation_Adjusted = CharlieLocation;

    if (!Internal_ProjectPointToNav(CharlieLocation + DoorForward * ElementSpacing, CharlieLocation_Adjusted) ||
        !Internal_FindPath(BetaLocation_Adjusted, CharlieLocation_Adjusted) ||
        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds))
    {
        CharlieLocation = BetaLocation_Adjusted + DoorForward * ElementSpacing;
        
		uint8 i = 0;
		while ((!Internal_ProjectPointToNav(CharlieLocation, CharlieLocation_Adjusted) ||
				!Internal_FindPath(BetaLocation_Adjusted, CharlieLocation_Adjusted) ||
				FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
				FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds)) && i < 5)
		{
			CharlieLocation = CharlieLocation + DoorForward * 20.0f;
			i++;
		}
		
		// try expand right, then left
		if (i == 5)
		{
			CharlieLocation = BetaLocation_Adjusted + DoorRight * ElementSpacing;
			
			i = 0;
			while ((!Internal_ProjectPointToNav(CharlieLocation, CharlieLocation_Adjusted) ||
					!Internal_FindPath(BetaLocation_Adjusted, CharlieLocation_Adjusted) ||
					FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
					FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds)) && i < 5)
			{
				CharlieLocation = CharlieLocation + DoorRight * 20.0f;
				i++;
			}
		
			if (i == 5)
			{
				CharlieLocation = BetaLocation_Adjusted + DoorForward * ElementSpacing;
				Depth++;
				
				i = 0;
				while ((!Internal_ProjectPointToNav(CharlieLocation, CharlieLocation_Adjusted) ||
						!Internal_FindPath(BetaLocation_Adjusted, CharlieLocation_Adjusted) ||
						FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
						FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds)) && i < 5)
				{
					CharlieLocation = CharlieLocation + -DoorRight * 20.0f;
					i++;
				}
				
				if (i == 5)
				{
					bCharlieSuccess = false;
				}
			}
		}
		else
		{
			Depth++;
		}
    }
	else
	{
		Depth++;
	}

    if (bCharlieSuccess)
    {
        SpawnStackUpActorAtLocation(CharlieLocation_Adjusted, Door, ESquadPosition::SP_Charlie, bFront, bLeft, Depth);
    }
    else
    {
        return;
    }

    FBoxSphereBounds CharlieBounds = FBoxSphereBounds(CharlieLocation_Adjusted, FVector(ElementBounds), ElementBounds);
    
    bool bDeltaSuccess = true;
    FVector DeltaLocation = CharlieLocation_Adjusted + DoorForward * ElementSpacing;
    FVector DeltaLocation_Adjusted = DeltaLocation;

	if (!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
        !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted) ||
        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||
        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
        (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
        Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront)))
    {
		DeltaLocation = CharlieLocation_Adjusted + DoorForward * ElementSpacing;
        
        FVector BaseDeltaLocation = CharlieLocation_Adjusted;
        
        uint8 i = 0;
        while ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
                !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted) ||
                FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
                FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||
                FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
                (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
                Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront))) && i < 3)
        {
            //DrawDebugPoint(GetWorld(), DeltaLocation_Adjusted, 20.0f, FColor::Yellow, false, 2.0f);
            DeltaLocation = DeltaLocation + DoorForward * ElementSpacing;
            i++;
        }

        // try expand right, then left
        if (i == 3)
        {
            DeltaLocation = BaseDeltaLocation;
            
            i = 0;
            while ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
                    !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted) ||
                    FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
                    FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||
                    FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
                    (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
                    Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront))) && i < 5)
            {
                //DrawDebugPoint(GetWorld(), DeltaLocation_Adjusted, 20.0f, FColor::Yellow, false, 2.0f);
                DeltaLocation = DeltaLocation + DoorRight * 50.0f;
                i++;
            }
            
            if (i == 5)
            {
                DeltaLocation = BaseDeltaLocation;
                
                i = 0;
                while ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
                        !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted) ||
                        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
                        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||
                        FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
                        (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
                        Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront))) && i < 5)
                {
                    //DrawDebugPoint(GetWorld(), DeltaLocation_Adjusted, 20.0f, FColor::Yellow, false, 2.0f);
                    DeltaLocation = DeltaLocation + DoorForward * 50.0f + DoorRight * 25.0f;
                    i++;
                }

                if (i == 5)
                {
                    DeltaLocation = BaseDeltaLocation;
                    
                    i = 0;
                    while ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
                            !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted) ||
                            FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
                            FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||
                            FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
                            (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
                            Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront))) && i < 5)
                    {
                        //DrawDebugPoint(GetWorld(), DeltaLocation_Adjusted, 20.0f, FColor::Yellow, false, 2.0f);
                        DeltaLocation = DeltaLocation + DoorForward * 50.0f + -DoorRight * 25.0f;
                        i++;
                    }

                    if (i == 5)
                    {
                        DeltaLocation = BaseDeltaLocation;
                        
                        i = 0;
                        while ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
                                !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted) ||
                                FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
                                FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||
                                FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
                                (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
                                Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront))) && i < 3)
                        {
                            //DrawDebugPoint(GetWorld(), DeltaLocation_Adjusted, 20.0f, FColor::Yellow, false, 2.0f);
                            DeltaLocation = DeltaLocation + -DoorRight * 50.0f;
                            i++;
                        }
                        
                        if (i == 3)
                        {
                            DeltaLocation = CharlieLocation_Adjusted + DoorRight * ElementSpacing;
                            
                            i = 0;
                            while ((!Internal_ProjectPointToNav(DeltaLocation, DeltaLocation_Adjusted) ||
                                    !Internal_FindPath(CharlieLocation_Adjusted, DeltaLocation_Adjusted) ||
                                    FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), AlphaBounds) ||
                                    FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), BetaBounds) ||
                                    FBoxSphereBounds::BoxesIntersect(FBoxSphereBounds(DeltaLocation_Adjusted, FVector(ElementBounds), ElementBounds), CharlieBounds) ||
                                    (FVector::DotProduct(DoorRight, (DeltaLocation_Adjusted - DoorLocation).GetSafeNormal2D()) < DotThreshold &&
                                    Internal_PointHasLineOfSightToDoor(DeltaLocation_Adjusted, Door, bFront))) && i < 5)
                            {
                                //DrawDebugPoint(GetWorld(), DeltaLocation_Adjusted, 20.0f, FColor::Yellow, false, 2.0f);
                                DeltaLocation = DeltaLocation + -DoorRight * 50.0f;
                                i++;
                            }

                            if (i == 5)
                                bDeltaSuccess = false;
                        }
                        else
                        {
                            Depth++;
                        }
                    }
                    else
                    {
                        Depth++;
                    }
                }
                else
                {
                    Depth++;
                }
            }
        }
        else
        {
            Depth++;
        }
    }
	else
	{
		Depth++;
	}

    if (bDeltaSuccess)
    {
        SpawnStackUpActorAtLocation(DeltaLocation_Adjusted, Door, ESquadPosition::SP_Delta, bFront, bLeft, Depth);
    }
}


void AWorldDataGenerator::GenerateDoorStackUps(ADoor* Door)
{
	/*
    return;
    FText Text = FText::FromString("Finding Stack Points for " + Door->GetName());
   
	FScopedSlowTask SlowTask(6, Text);
	SlowTask.MakeDialog();
    SlowTask.EnterProgressFrame();

    if (Door->GetSubDoor() && !Door->IsMainSubdoor())
    {
        GenerateDoorStackUps(Door->GetSubDoor());
        Door->FrontLeftStackUpPoints = Door->GetSubDoor()->BackRightStackUpPoints;
        Door->FrontRightStackUpPoints = Door->GetSubDoor()->BackLeftStackUpPoints;
        Door->BackLeftStackUpPoints = Door->GetSubDoor()->FrontRightStackUpPoints;
        Door->BackRightStackUpPoints = Door->GetSubDoor()->FrontLeftStackUpPoints;
        Door->RedTeamBackClearPoints = Door->GetSubDoor()->BlueTeamFrontClearPoints;
        Door->BlueTeamBackClearPoints = Door->GetSubDoor()->RedTeamFrontClearPoints;
        Door->RedTeamFrontClearPoints = Door->GetSubDoor()->BlueTeamBackClearPoints;
        Door->BlueTeamFrontClearPoints = Door->GetSubDoor()->RedTeamBackClearPoints;
        return;
    }
    
    // only gen on the main door... we'll copy the points across later
    if (Door->IsMainSubdoor()|| !Door->GetSubDoor())
    {        
        Door->FrontLeftStackUpPoints.Empty();
        Door->FrontRightStackUpPoints.Empty();
        Door->BackLeftStackUpPoints.Empty();
        Door->BackRightStackUpPoints.Empty();
      
        for (int32 y = 0; y < 2; y++)
        {
            for (int32 z = 0; z < 2; z++)
            {
                LoosenedStackupRestrictionAmount = 0.0f;
                FatalFunnelDistMultiplier = 0.0f;
                MinStackUpDistToDoor = Door->GetDoorway()->GetScaledBoxExtent().Y + 50.0f;
                
                TArray<FVector> SpawnLocations;
                TryGenerateStackUpLocations(Door, y == 0, z == 0,  SpawnLocations);
                
                while (true)
                {
                    // Test alpha position and make sure its not far from the door.. otherwise gen again with loosened restrictions
                    const FVector AlphaLocation = FindStackupPositionFromLocationArray(SpawnLocations, ESquadPosition::SP_Alpha, Door, y==0);

                    float Dist = (AlphaLocation - Door->GetDoorway()->GetComponentLocation()).Size();
                    if (Dist > 250.0f && LoosenedStackupRestrictionAmount < 130.0f)
                    {
                        //V_LOGM(LogReadyOrNot, "%s: Alpha Stack Dist: %.2f", *GetNameSafe(Door), Dist)
                        LoosenedStackupRestrictionAmount += 10.0f;
                        TryGenerateStackUpLocations(Door, y == 0, z == 0, SpawnLocations);
                    }
                    else
                    {
                        break;
                    }
                }

                TMap<ESquadPosition, FVector> SquadSpawnLocationMap;
                for (int32 i = 0; i < (int32)ESquadPosition::SP_Foxtrot; i++)
                {
                    const ESquadPosition SquadPosition =  (ESquadPosition)i;
                    const FVector SpawnLocation = FindStackupPositionFromLocationArray(SpawnLocations, SquadPosition, Door, y==0);
                    SpawnStackUpActorAtLocation(SpawnLocation, Door, SquadPosition, y==0, z==0);
                    RemoveStackupsWithinDist(SpawnLocations, SpawnLocation, MinimumSpacing, SpawnLocations);                  
                }        
            } 
        }       
    }
    SlowTask.EnterProgressFrame();
                        
    Door->FrontLeftStackUpPoints.Remove(nullptr);
    Door->FrontRightStackUpPoints.Remove(nullptr);
    Door->BackLeftStackUpPoints.Remove(nullptr);
    Door->BackRightStackUpPoints.Remove(nullptr);

    if ((Door->IsMainSubdoor() && Door->GetSubDoor()) || !Door->GetSubDoor())
    {
        // finally just copy the stackup points from front left/right its probably a tiny ass room we can't gen in
        if (Door->FrontRightStackUpPoints.Num() < 4 && Door->FrontLeftStackUpPoints.Num() < 4)
        {
            for (int32 i = 0; i < Door->FrontRightStackUpPoints.Num() && Door->FrontLeftStackUpPoints.Num() < 4; i++)
            {  
              Door->FrontLeftStackUpPoints.Add(Door->FrontRightStackUpPoints[i]);
            }
            
            Door->FrontRightStackUpPoints = Door->FrontLeftStackUpPoints;
        }
        
        if (Door->FrontLeftStackUpPoints.Num() < 4 && Door->FrontRightStackUpPoints.Num() == 4)
        {
            for (int32 i = 0; i < Door->FrontLeftStackUpPoints.Num(); i++)
            {
                Door->FrontLeftStackUpPoints[i]->Destroy();
            }
            
            Door->FrontLeftStackUpPoints = Door->FrontRightStackUpPoints;
        }
        
        if (Door->FrontRightStackUpPoints.Num() < 4 && Door->FrontLeftStackUpPoints.Num() == 4)
        {
            for (int32 i = 0; i < Door->FrontRightStackUpPoints.Num(); i++)
            {
                Door->FrontRightStackUpPoints[i]->Destroy();
            }
            
            Door->FrontRightStackUpPoints = Door->FrontLeftStackUpPoints;
        }
        
        if (Door->BackLeftStackUpPoints.Num() < 4 && Door->BackRightStackUpPoints.Num() == 4)
        {
            for (int32 i = 0; i < Door->BackLeftStackUpPoints.Num(); i++)
            {
                Door->BackLeftStackUpPoints[i]->Destroy();
            }
            
            Door->BackLeftStackUpPoints = Door->BackRightStackUpPoints;
        }
        
        if (Door->BackRightStackUpPoints.Num() < 4 && Door->BackLeftStackUpPoints.Num() == 4)
        {
            for (int32 i = 0; i < Door->BackRightStackUpPoints.Num(); i++)
            {
                Door->BackRightStackUpPoints[i]->Destroy();
            }
            
            Door->BackRightStackUpPoints = Door->BackLeftStackUpPoints;
        }
    }
    
    for (int32 i = 0; i < Door->FrontLeftStackUpPoints.Num(); i++)
    {
        Door->FrontLeftStackUpPoints[i]->SetSquadPosition(Door, (ESquadPosition)i);
    }
    
    for (int32 i = 0; i < Door->FrontRightStackUpPoints.Num(); i++)
    {
        Door->FrontRightStackUpPoints[i]->SetSquadPosition(Door, (ESquadPosition)i);
    }
    
    for (int32 i = 0; i < Door->BackLeftStackUpPoints.Num(); i++)
    {
        Door->BackLeftStackUpPoints[i]->SetSquadPosition(Door, (ESquadPosition)i);
    }
    
    for (int32 i = 0; i < Door->BackRightStackUpPoints.Num(); i++)
    {
        Door->BackRightStackUpPoints[i]->SetSquadPosition(Door, (ESquadPosition)i);
    }
	*/
}

void AWorldDataGenerator::DeleteDoorStackUps(ADoor* Door)
{
    // Temporarily reset these
    Door->bCanIssueOrdersOnFrontSide = true;
    Door->bCanIssueOrdersOnBackSide = true;
    
    for (AStackUpActor* StackUpActor : Door->FrontLeftStackUpPoints)
    {
        if (StackUpActor)
            StackUpActor->Destroy();
    }
    
    for (AStackUpActor* StackUpActor : Door->FrontRightStackUpPoints)
    {
        if (StackUpActor)
            StackUpActor->Destroy();
    }
    
    for (AStackUpActor* StackUpActor : Door->BackLeftStackUpPoints)
    {
        if (StackUpActor)
            StackUpActor->Destroy();
    }
    
    for (AStackUpActor* StackUpActor : Door->BackRightStackUpPoints)
    {
        if (StackUpActor)
            StackUpActor->Destroy();
    }
    
    Door->FrontLeftStackUpPoints.Empty();
    Door->FrontRightStackUpPoints.Empty();
    Door->BackLeftStackUpPoints.Empty();
    Door->BackRightStackUpPoints.Empty();

    if (Door->GetSubDoor())
    {
        for (AStackUpActor* StackUpActor : Door->GetSubDoor()->FrontLeftStackUpPoints)
        {
            if (StackUpActor)
                StackUpActor->Destroy();
        }
        
        for (AStackUpActor* StackUpActor : Door->GetSubDoor()->FrontRightStackUpPoints)
        {
            if (StackUpActor)
                StackUpActor->Destroy();
        }
        
        for (AStackUpActor* StackUpActor : Door->GetSubDoor()->BackLeftStackUpPoints)
        {
            if (StackUpActor)
                StackUpActor->Destroy();
        }
        
        for (AStackUpActor* StackUpActor : Door->GetSubDoor()->BackRightStackUpPoints)
        {
            if (StackUpActor)
                StackUpActor->Destroy();
        }
        
        Door->GetSubDoor()->FrontLeftStackUpPoints.Empty();
        Door->GetSubDoor()->FrontRightStackUpPoints.Empty();
        Door->GetSubDoor()->BackLeftStackUpPoints.Empty();
        Door->GetSubDoor()->BackRightStackUpPoints.Empty();
    }
}

void AWorldDataGenerator::TryGenerateStackUpLocations(ADoor* Door, bool bFront, bool bLeft, TArray<FVector>& OutLocations)
{
    FVector DoorLocation = Door->GetDoorMidLocation();
    DoorLocation.Z = Door->GetActorLocation().Z;
    
    if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        for (int32 x = -10; x < 10; x++)
        {
            for (int32 y = -10; y < 10; y++)
            {
                FNavLocation NavPt1(bFront ? Door->GetActorLocation() + Door->GetActorForwardVector() * 100.0f : Door->GetActorLocation() + Door->GetActorForwardVector() * -100.0f);
                NavPt1.Location += FVector(x*100.0f, y*100.0f, 0.0f);
                
                FVector TestLocation = NavPt1.Location;
                if (!NavSys->ProjectPointToNavigation(TestLocation, NavPt1, FVector(100.0f, 100.0f, 300.0f)))
                {
                    continue;
                }

                
                FNavLocation DoorNavLocation(bFront ? DoorLocation + Door->GetActorForwardVector() * 25.0f : DoorLocation + Door->GetActorForwardVector() * -25.0f);
                NavSys->ProjectPointToNavigation(DoorNavLocation.Location, DoorNavLocation);
                
                FPathFindingQuery PathFindingQuery;
                PathFindingQuery.StartLocation = NavPt1.Location;
                PathFindingQuery.EndLocation = DoorNavLocation.Location;
                PathFindingQuery.SetAllowPartialPaths(false);
                TSubclassOf<UNavigationQueryFilter> FilterClass = UNavigationQueryFilter::StaticClass();
                FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
                PathFindingQuery.QueryFilter = QueryFilter;
                
                FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
                if (PathFindingResult.Result == ENavigationQueryResult::Success)
                {                       
                    if (!IsSuitablePositionForStackUpPoint(NavPt1.Location, Door))
                    {
                        //DrawDebugBox(GetWorld(), NavPt1.Location, FVector(5.0f), FColor::Red, false, 5.0f);
                        continue;
                    }

                    if (Door)
                    {
                        FVector v1 = Door->GetActorRightVector();
                        FVector DoorwayLoc = Door->GetDoorway()->GetComponentLocation();
                        FVector ThreatLoc = NavPt1.Location;
                        DoorwayLoc.Z = 0;
                        ThreatLoc.Z = 0;
                        FVector v2 = UKismetMathLibrary::FindLookAtRotation(DoorwayLoc, ThreatLoc).Vector();
                        float DoorCheckDelta = FVector2D::DotProduct(FVector2D(v1.X, v1.Y), FVector2D(v2.X, v2.Y));
                        //DrawDebugBox(GetWorld(), NavPt1.Location, FVector(5.0f), DoorCheckDelta < 0.0f ? FColor::Yellow : FColor::Cyan, false, 5.0f);
                        if (bLeft && DoorCheckDelta > 0.0f)
                            continue;

                        if (!bLeft && DoorCheckDelta < 0.0f)
                            continue;
                    }

                    if (Door)
                    {
                        FVector v1 = Door->GetActorForwardVector();
                        FVector DoorwayLoc = Door->GetDoorway()->GetComponentLocation();
                        FVector ThreatLoc = NavPt1.Location;
                        DoorwayLoc.Z = 0;
                        ThreatLoc.Z = 0;
                        FVector v2 = UKismetMathLibrary::FindLookAtRotation(DoorwayLoc, ThreatLoc).Vector();
                        float DoorCheckDelta = FVector2D::DotProduct(FVector2D(v1.X, v1.Y), FVector2D(v2.X, v2.Y));
                        //DrawDebugBox(GetWorld(), NavPt1.Location, FVector(5.0f), DoorCheckDelta < 0.0f ? FColor::Yellow : FColor::Cyan, false, 5.0f);
                        if (bFront && DoorCheckDelta < 0.0f)
                            continue;

                        if (!bFront && DoorCheckDelta > 0.0f)
                            continue;
                    }
                    
                    if (Door->GetSubDoor())
                    {
                        FVector v1 = Door->GetActorRightVector();
                        FVector DoorwayLoc = Door->GetSubDoor()->GetDoorway()->GetComponentLocation();
                        FVector ThreatLoc = NavPt1.Location;
                        DoorwayLoc.Z = 0;
                        ThreatLoc.Z = 0;
                        FVector v2 = UKismetMathLibrary::FindLookAtRotation(DoorwayLoc, ThreatLoc).Vector();
                        float DoorCheckDelta = FVector2D::DotProduct(FVector2D(v1.X, v1.Y), FVector2D(v2.X, v2.Y));
                        //DrawDebugBox(GetWorld(), NavPt1.Location, FVector(5.0f), DoorCheckDelta < 0.0f ? FColor::Yellow : FColor::Cyan, false, 5.0f);
                        if (bLeft && DoorCheckDelta > 0.0f)
                            continue;

                        if (!bLeft && DoorCheckDelta < 0.0f)
                            continue;
                    }

                    
                    OutLocations.Add(NavPt1.Location);
                }
            }
        }
    }
}

FVector AWorldDataGenerator::FindStackupPositionFromLocationArray(TArray<FVector> InLocations, ESquadPosition SquadPosition, ADoor* Door, bool bFront)
{
    FVector DoorLocation = Door->GetDoorMidLocation();
    DoorLocation.Z = Door->GetActorLocation().Z;
    
    // This is pretty slow, but its required for optimal path points (ie we don't want ones that are on the other side of thin walls)
    InLocations.Sort([Door](const FVector& Lhs, const FVector& Rhs)
    {
         float LhsDist = (Door->GetActorLocation() - Lhs).Size();
         float RhsDist = (Door->GetActorLocation() - Rhs).Size();
         return LhsDist < RhsDist;
    });

    for (int32 i =0 ; i < InLocations.Num(); i++)
    {
        if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            FNavLocation DoorNavLocation(bFront ? DoorLocation + Door->GetActorForwardVector() * 25.0f : DoorLocation + Door->GetActorForwardVector() * -25.0f);
            FPathFindingQuery PathFindingQuery;
            PathFindingQuery.StartLocation = InLocations[i];
            PathFindingQuery.EndLocation = DoorNavLocation.Location;
            PathFindingQuery.SetAllowPartialPaths(false);
            TSubclassOf<UNavigationQueryFilter> FilterClass = UNavigationQueryFilter::StaticClass();
            FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
            PathFindingQuery.QueryFilter = QueryFilter;
            FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
            if (PathFindingResult.Result == ENavigationQueryResult::Success)
            {
                // Don't allow a close path point that might go around a long wall
                float PathLength = PathFindingResult.Path->GetLength();
                float ActualDist = (InLocations[i] - DoorNavLocation).Size();
                //V_LOGM(LogReadyOrNot, "%s %d Path: %f Actual: %f", *GetNameSafe(Door), i, PathLength, ActualDist);
                if (PathLength < ActualDist + 250.0f)
                {
                    if (SquadPosition == ESquadPosition::SP_Alpha && ActualDist > (((int32)SquadPosition+1) * 200.0f))
                        continue;
                    
                    return InLocations[i];
                }

            }
        }
    }


    return  FVector::ZeroVector;
}

void AWorldDataGenerator::RemoveStackupsWithinDist(TArray<FVector> InLocations, FVector TestLocation, float Dist,
                                                   TArray<FVector>& OutLocation)
{
    OutLocation.Empty();
    for (int32 i = 0; i < InLocations.Num(); i++)
    {
        float TestDist = (InLocations[i] - TestLocation).Size();
        if (TestDist > Dist)
        {
            OutLocation.Add(InLocations[i]);
        }
    }
}

void AWorldDataGenerator::LinkSubdoors()
{
    TArray<ADoor*> Doors;
    for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
    {
        Doors.Add(*It);
    }

    for (int32 i = 0; i < Doors.Num(); i++)
    {
        ADoor* Door = Doors[i];
        Door->SetSubDoor(nullptr, false);
    }

    for (int32 i = 0; i < Doors.Num(); i++)
    {
        for (int32 y = 0; y < Doors.Num(); y++)
        {
            ADoor* DoorA = Doors[i];
            ADoor* DoorB = Doors[y];
            if (DoorA != DoorB)
            {
                float Dist = (DoorA->GetActorLocation() - DoorB->GetActorLocation()).Size();

                if (Dist < 300.0f)
                {
                    FCollisionObjectQueryParams CollisionObjectQuery;
                    CollisionObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
                    CollisionObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);

                    FCollisionQueryParams CollisionQueryParams;
                    CollisionQueryParams.AddIgnoredActor(DoorA);
                    CollisionQueryParams.AddIgnoredActor(DoorB);

                    FHitResult Hit;
                    GetWorld()->LineTraceSingleByObjectType(Hit, DoorA->GetDoorway()->GetComponentLocation(), DoorB->GetDoorway()->GetComponentLocation(), CollisionObjectQuery, CollisionQueryParams);

                    if (!Hit.bBlockingHit)
                    {
                        FRotator DoorARot = DoorA->GetActorRotation();
                        FRotator DoorBRot = DoorB->GetActorRotation();

                        FRotator DeltaRotator = UKismetMathLibrary::NormalizedDeltaRotator(DoorARot, DoorBRot);
                        if (FMath::IsNearlyEqual(FMath::Abs(DeltaRotator.Yaw), 180.0f, 3.0f))
                        {
                            V_LOGM(LogReadyOrNot, "%s linked to %s", *GetNameSafe(DoorA), *GetNameSafe(DoorB));
                            DoorA->SetSubDoor(DoorB, true);
                            DoorB->SetSubDoor(DoorA, false);
                        }
                    }
                   
                }
            }
        }
    }
}

bool AWorldDataGenerator::IsSuitablePositionForStackUpPoint(FVector Location, ADoor* Door)
{
    return false;
    /*
    if (Location == FVector::ZeroVector)
    {
        //V_LOGM(LogReadyOrNot, "Zero Vector Passed In")
        return false;
    }

    if (Door)
    {
        FTransform Transform;
        Transform.SetLocation(Door->GetDoorway()->GetComponentLocation());
        Transform.SetRotation(Door->GetActorRotation().Quaternion());
        bool bPointInBox =  UKismetMathLibrary::IsPointInBoxWithTransform(Location, Transform, Door->GetNoStackupsExtent( FMath::Clamp(140.0f - LoosenedStackupRestrictionAmount, 0.0f, 140.0f)));
        //DrawDebugBox(GetWorld(), Door->GetDoorway()->GetComponentLocation(), Door->NoStackupsExtent,  FColor::Green, false, 10.0f, 0, 30);
        //DrawDebugPoint(GetWorld(), Location , 25.0f, bPointInBox ? FColor::Red : FColor::Green, false, 10.0f, 0);
        if (bPointInBox)
            return false;
    }
    
    if (Door->GetSubDoor())
    {
        ADoor* SubDoor = Door->GetSubDoor();
        FTransform Transform;
        Transform.SetLocation(SubDoor->GetDoorway()->GetComponentLocation());
        Transform.SetRotation(SubDoor->GetActorRotation().Quaternion());
        bool bPointInBox =  UKismetMathLibrary::IsPointInBoxWithTransform(Location, Transform, SubDoor->GetNoStackupsExtent(FMath::Clamp(140.0f - LoosenedStackupRestrictionAmount, 0.0f, 140.0f)));
        //DrawDebugBox(GetWorld(), SubDoor->GetDoorway()->GetComponentLocation(), SubDoor->NoStackupsExtent,  FColor::Green, false, 1.0f, 0, 30);
        //DrawDebugPoint(GetWorld(), Location , 25.0f, bPointInBox ? FColor::Red : FColor::Green, false, 10.0f, 0);
        if (bPointInBox)
            return false;
    }
	
    return true;
    */
}

AStackUpActor* AWorldDataGenerator::SpawnStackUpActorAtLocation(FVector Location, ADoor* Door, ESquadPosition SquadPosition, bool bFront, bool bLeft, uint8 Depth)
{
    if (!Door)
        return nullptr;

    if (Location == FVector::ZeroVector)
        return nullptr;

    Location = FVector(FIntVector(Location));
    
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.bNoFail = true;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AStackUpActor* StackUpActor = GetWorld()->SpawnActor<AStackUpActor>(AStackUpActor::StaticClass(), Location, FRotator::ZeroRotator, SpawnParameters);
    StackUpActor->SetSquadPosition(Door, SquadPosition);
    StackUpActor->SetActorEnableCollision(false);
    StackUpActor->SetCanBeDamaged(false);
	StackUpActor->Depth = Depth;

    if (bFront)
    {
        bLeft ? Door->FrontLeftStackUpPoints.Add(StackUpActor) : Door->FrontRightStackUpPoints.Add(StackUpActor);
    }
	else
    {
        bLeft ? Door->BackLeftStackUpPoints.Add(StackUpActor) : Door->BackRightStackUpPoints.Add(StackUpActor);
    }

	#if WITH_EDITOR
    StackUpActor->SetFolderPath("GeneratedStackUpActors");
    FString OutLabel = FString(ENUM_TO_STRING(ESquadPosition, SquadPosition, false)) + "_" + (bFront ? "Front_" : "Back_") + (bLeft ? "Left" : "Right");
    StackUpActor->SetActorLabel(OutLabel);
	#endif

    return StackUpActor;
}

void AWorldDataGenerator::AddAnyThreatsThatContainOurs(AThreatAwarenessActor* Threat, TArray<AThreatAwarenessActor*>& VisibleThreatAwarenessActors)
{
    for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
    {
        AThreatAwarenessActor* t = *It;
        if (t->PathableThreatAwarenessActors.Contains(Threat))
        {
            VisibleThreatAwarenessActors = t->PathableThreatAwarenessActors;
        }
    }
}

void AWorldDataGenerator::SortAndTrimMaxVisibleThreats(const TArray<AThreatAwarenessActor*>& InThreats)
{
    int32 TotalThreats = GetThreatAwarenessActorCount();
    
    FScopedSlowTask SlowTask(TotalThreats,NSLOCTEXT("SortingThreatAwarenessActors", "Sorting Threat Awareness Actors", "Sorting Threat Awareness Actors"));
    SlowTask.MakeDialog();
    
    int32 Idx = 0;
    //for (AThreatAwarenessActor* t : InThreats)
    for (TActorIterator<AThreatAwarenessActor> t(GetWorld()); t; ++t)
    {
        SlowTask.EnterProgressFrame();
        
        SlowTask.DefaultMessage = FText::FromString(FString::Format(TEXT("{0}/{1} Sorting {2} (Total Pathable: {3}, Total Visible: {4})"), {Idx, TotalThreats, GetNameSafe(*t),t->PathableThreatAwarenessActors.Num() }));
        Idx++;
        
        if (t->GetThreatLevel() == EThreatLevel::TL_Stairs)
            continue;
        
        FVector Location = t->GetActorLocation();
        UWorld* World = GetWorld();
        
        t->PathableThreatAwarenessActors.Remove(nullptr);
        t->PathableThreatAwarenessActors.Sort([World, Location](const AThreatAwarenessActor& Lhs, const AThreatAwarenessActor& Rhs)
        {
            float LhsDist = BIG_DIST;
            float RhsDist = BIG_DIST;
            if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
            {
               FPathFindingQuery PathFindingQuery;
               PathFindingQuery.StartLocation = Location;
               PathFindingQuery.EndLocation = Lhs.GetActorLocation();
               PathFindingQuery.SetAllowPartialPaths(false);
               TSubclassOf<UNavigationQueryFilter> FilterClass = UNavigationQueryFilter::StaticClass();
               FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
               PathFindingQuery.QueryFilter = QueryFilter;
               FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
               if (PathFindingResult.Result == ENavigationQueryResult::Success)
               {
                    LhsDist = PathFindingResult.Path->GetLength();
               }
               else
               {
                   LhsDist = BIG_DIST;
               }
                
               PathFindingQuery.StartLocation = Location;
               PathFindingQuery.EndLocation = Rhs.GetActorLocation();
               PathFindingQuery.SetAllowPartialPaths(false);
               PathFindingQuery.QueryFilter = QueryFilter;
               PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Regular);
               if (PathFindingResult.Result == ENavigationQueryResult::Success)
               {
                    RhsDist = PathFindingResult.Path->GetLength();
               }
               else
               {
                   RhsDist = BIG_DIST;
               }
            }
            
            return LhsDist < RhsDist;
        });

        t->PathableThreatAwarenessActors.Remove(*t);
        
        if (t->IsDoorThreat())
        {
            if (t->bFrontDoorThreat)
                t->GetAttachedDoor()->FrontThreatAwarenessPoints = t->PathableThreatAwarenessActors;
            else
                t->GetAttachedDoor()->BackThreatAwarenessPoints = t->PathableThreatAwarenessActors;
        }
    }
}

void AWorldDataGenerator::ReportAllUnreachableSpawns()
{
    const APlayerStart* PlayerStart = nullptr;
    for (const TActorIterator<APlayerStart> It(GetWorld()); It;)
    {
        PlayerStart = *It;
        break;
    }
    
    if (PlayerStart)
    {
        TMap<ADoor*, FTransform> OriginalTransformMap; 
        
        for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
        {
            ADoor* OtherDoor = *It;
            
            OriginalTransformMap.Add(OtherDoor, OtherDoor->GetActorTransform());
            
            OtherDoor->SetActorTransform(FTransform(FVector(0.0f, 0.0f, -50000.0f)));
            
            FNavigationSystem::OnActorBoundsChanged(*OtherDoor);
            FNavigationSystem::UpdateActorAndComponentData(*OtherDoor);
        }
        
        GEngine->Exec(GetWorld(), TEXT("rebuildnavigation"));

        TArray<AAISpawn*> UnreachableSpawns;
        TArray<AAISpawn*> ReachableSpawns;
    
        for (TActorIterator<AAISpawn> It(GetWorld()); It; ++It)
        {
            AAISpawn* AISpawn = *It;
            
            FVector Start = FVector::ZeroVector, End = FVector::ZeroVector;
            Internal_ProjectPointToNav(PlayerStart->GetActorLocation(), Start, FVector(100.0f, 100.0f, 200.0f));
            Internal_ProjectPointToNav(AISpawn->GetActorLocation(), End, FVector(100.0f, 100.0f, 200.0f));
            
            if (!Internal_FindPath_RoomGen(Start, End))
            {
                UnreachableSpawns.AddUnique(AISpawn);
            }
            else
            {
                ReachableSpawns.AddUnique(AISpawn);
            }
        }
        
        for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
        {
            ADoor* OtherDoor = *It;
            
            OtherDoor->SetActorTransform(*OriginalTransformMap.Find(OtherDoor));
            
            FNavigationSystem::OnActorBoundsChanged(*OtherDoor);
            FNavigationSystem::UpdateActorAndComponentData(*OtherDoor);
        }

        GEngine->Exec(GetWorld(), TEXT("rebuildnavigation"));

        if (UnreachableSpawns.Num() > 0)
        {
            FString UnreachableSpawnStrings = LINE_TERMINATOR;
            for (const AAISpawn* Spawn : UnreachableSpawns)
            {
                UnreachableSpawnStrings += LINE_TERMINATOR;
                UnreachableSpawnStrings += Spawn->GetName();
            }

            FString Blah = " spawners are unreachable";
            if (UnreachableSpawns.Num() == 1)
                Blah = " spawner is unreachable";
            
            const FText Msg = FText::FromString("Failure\n\n" + FString::FromInt(UnreachableSpawns.Num()) + Blah + UnreachableSpawnStrings);
            FMessageDialog::Debugf(Msg);
            
            V_LOGM(LogReadyOrNot, "%i Unreachable Spawns", UnreachableSpawns.Num());
            for (int32 i = 0; i < UnreachableSpawns.Num(); i++)
            {
                V_LOGM(LogReadyOrNot, "%s", *UnreachableSpawns[i]->GetName());
            }
        }
        else
        {
            FMessageDialog::Debugf(FText::FromString("Success\n\nAll spawns are reachable!"));
        }
    }
    else
    {
        FMessageDialog::Debugf(FText::FromString("Unable to verify\n\nAt least one Player Start actor must be in the level"));
    }
}

void AWorldDataGenerator::RemoveAllOverlappingThreats(TArray<AThreatAwarenessActor*> InThreats)
{
    TArray<AActor*> OutActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AThreatAwarenessActor::StaticClass(), OutActors);
    TArray<AActor*> ActorsToKeep;
    TArray<AActor*> ActorsToRemove;
    for (int32 i = 0; i < OutActors.Num(); i++)
    {
        if (ActorsToRemove.Contains(OutActors[i]))
            continue;

        ActorsToKeep.AddUnique(OutActors[i]);     
        
        for (int32 y = 0; y < OutActors.Num(); y++)
        {
            if (OutActors[i] == OutActors[y])
                continue;
            
            if (ActorsToKeep.Contains(OutActors[y]))
                continue;

            float Dist = (OutActors[y]->GetActorLocation() - OutActors[i]->GetActorLocation()).Size();
            if (Dist < 100.0f)
            {
                AThreatAwarenessActor* TAA = Cast<AThreatAwarenessActor>(OutActors[i]);
                AThreatAwarenessActor* OtherTAA = Cast<AThreatAwarenessActor>(OutActors[y]);
                
                if ((OtherTAA->IsDoorThreat() || OtherTAA->GetThreatLevel() >= EThreatLevel::TL_Extreme) &&
                    ((TAA->IsDoorThreat() || TAA->GetThreatLevel() >= EThreatLevel::TL_Extreme)))
                {
                    continue;
                }
                
                if (OtherTAA->IsDoorThreat() || OtherTAA->GetThreatLevel() >= EThreatLevel::TL_Extreme)
                {
                    ActorsToRemove.AddUnique(OutActors[i]);
                }
                else
                {
                    ActorsToRemove.AddUnique(OutActors[y]);
                }
            }
        }
    }
    
    for (AActor* t : ActorsToRemove)
    {
        t->Destroy();
    }

    /*
    InThreats.RemoveAll([](AThreatAwarenessActor* A)
    {
        return !IsValid(A);
    });
    */
    
    //FMessageDialog::Debugf(FText::FromString("Total Threats To Remove: "+ FString::FromInt(ActorsToRemove.Num()) + " Total Actors To Keep: " + FString::FromInt(ActorsToKeep.Num())));
}

void AWorldDataGenerator::PlacePatrolPointsOnAllThreats(const TArray<AThreatAwarenessActor*>& InThreats)
{
    AllPatrolPoints.Empty();
    AllPatrolPoints.Reserve(2000);
    
    for (AThreatAwarenessActor* It : InThreats)
    {
        if (!It->bIsOutside)
        {
            FPatrolPoint p;
            p.Location = FIntVector(It->GetActorLocation());
            
            if (It->IsDoorThreat())
            {
                p.Tags.Add("DoorPatrolPoint");
                continue;
            }
            
            AllPatrolPoints.Add(p);
        }
    }
}

FVector MakePseudoUniqueIDForDoor(ADoor* Door)
{
    return Door->GetActorLocation();
    
    // since no two doors will ever be in the same location, this makes for a great unique id!
    //uint64 PositionId = (uint64)FMath::Abs(Door->GetActorLocation().X) + (uint64)FMath::Abs(Door->GetActorLocation().Y) + (uint64)FMath::Abs(Door->GetActorLocation().Z);
    //return PositionId;// * 2205;
}

FVector MakePseudoUniqueIDForThreat(AThreatAwarenessActor* Threat)
{
    return Threat->GetActorLocation();
    // since no two threats will ever be in the same location, this makes for a great unique id!
    //uint64 PositionId = (uint64)FMath::Abs(Threat->GetActorLocation().X) + (uint64)FMath::Abs(Threat->GetActorLocation().Y) + (uint64)FMath::Abs(Threat->GetActorLocation().Z);
    //return PositionId; //* 2205;
}

FSavedThreatAwarenessActor ThreatActorToSavedThreatActor(AThreatAwarenessActor* Threat)
{
    FSavedThreatAwarenessActor s;
    s.UniqueID = MakePseudoUniqueIDForThreat(Threat);
    s.Location = Threat->GetActorLocation();
    s.ThreatLevel = Threat->GetThreatLevel();
    s.OwningRoom = Threat->OwningRoom.ToString();
    s.bIsOutside = Threat->bIsOutside;
    s.Door = Threat->DoorThreat ? MakePseudoUniqueIDForDoor(Threat->DoorThreat) : FVector::ZeroVector;
    s.bFrontDoorThreat = Threat->bFrontDoorThreat;

    TArray<ADoor*> Doors;
    if (Threat->GetUniqueExtis(Doors))
    {
        for (ADoor* D : Doors)
        {
            s.UniqueExits.AddUnique(MakePseudoUniqueIDForDoor(D));
        }
    }
    
    for (const FExitData& E : Threat->Exits)
    {
        FSavedExitData e;
        e.Location = E.Location;
        
        for (const FExitRoute& R : E.PossibleRoutes)
        {
            FSavedExitRoute r;
            r.Location = R.Location;
            r.PathCost = R.PathCost;
            
            for (ADoor* D : R.Doors)
            {
                r.Doors.Add(MakePseudoUniqueIDForDoor(D));
            }
            
            for (AThreatAwarenessActor* T : R.ThreatsOnRoute)
            {
                r.ThreatsOnRoute.Add(MakePseudoUniqueIDForThreat(T));
            }

            e.PossibleRoutes.Add(r);
        }
        
        s.Exits.Add(e);
    }

    for (const FLookAtPoint& P : Threat->SwatLookAtPoints)
    {
        FSavedSwatLookAtPoint L;
        L.Location = P.Location;
        L.LinkedDoorID = P.LinkedDoor ? MakePseudoUniqueIDForDoor(P.LinkedDoor) : FVector::ZeroVector;

        s.SwatLookAtPoints.Add(L);
    }
    
    for (AThreatAwarenessActor* T : Threat->PathableThreatAwarenessActors)
    {
        if (T)
        {
            s.PathableThreats.AddUnique(MakePseudoUniqueIDForThreat(T));
        }
    }
    
    return s;
}

FSavedStackUpActor StackUpActorToSavedStackUpActor(AStackUpActor* StackUp)
{
    FSavedStackUpActor s;

    s.Location = StackUp->GetActorLocation();
    s.Depth = StackUp->Depth;
    s.StackUpPosition = StackUp->GetSquadPosition();
    s.LinkedDoorID = StackUp->GetLinkedDoor() ? MakePseudoUniqueIDForDoor(StackUp->GetLinkedDoor()) : FVector::ZeroVector;
    
    return s;
}

FSavedCoverActor CoverActorToSavedCoverActor(ACoverPoint* Cover)
{
    FSavedCoverActor s;

    s.Name = Cover->GetName();
    s.Transform = Cover->GetActorTransform();
    s.CoverObjectName = Cover->CoverActor.IsValid() ? Cover->CoverActor->GetName() : "";
    s.CoverRail = Cover->CoverRail;
    s.StandCoverDirection = Cover->StandCoverDirection;
    s.CrouchCoverDirection = Cover->CrouchCoverDirection;
	s.StandCoverType = Cover->GetStandCoverType();
	s.CrouchCoverType = Cover->GetCrouchCoverType();
    s.Index = Cover->Index;
    s.bIsCrouchOnlyCover = Cover->bIsCrouchOnlyCover;

    return s;
}

FSavedClearPoint ClearPointToSavedClearPoint(const FClearPoint& P)
{
    FSavedClearPoint s;

    s.Location_Relative = P.Location_Relative;
    s.Location = P.Location;
    s.Direction = P.Direction;
    s.Stage = P.Stage;
    s.bHasLineOfSightToDoor = P.bHasLineOfSightToDoor;

    for (const ACoverLandmark* L : P.CoverLandmarks)
    {
        s.CoverLandmarks.AddUnique(L->GetName());
    }
    
    return s;
}

FSavedRoomData RoomDataToSavedRoomData(const FRoom& Room)
{
    FSavedRoomData s;

    s.Location = Room.Location;
    s.Name = Room.Name;
    s.RootDoorID = Room.RootDoor ? MakePseudoUniqueIDForDoor(Room.RootDoor) : FVector::ZeroVector;
    s.ConnectingRooms = Room.ConnectingRooms;

    for (ADoor* D : Room.AdditionalRootDoors)
    {
        if (D)
        {
            s.AdditionalRootDoors.AddUnique(MakePseudoUniqueIDForDoor(D));
        }
    }

    for (AThreatAwarenessActor* T : Room.Threats)
    {
        if (T)
        {
            s.Threats.AddUnique(MakePseudoUniqueIDForThreat(T));
        }
    }
    
    return s;
}

FClearPoint AWorldDataGenerator::MakeClearPointFromSavedClearPoint(const FSavedClearPoint& s) const
{
    FClearPoint p;
    p.Location_Relative = s.Location;
    p.Location = s.Location;
    p.Direction = s.Direction;
    p.Stage = s.Stage;
    p.bHasLineOfSightToDoor = s.bHasLineOfSightToDoor;
    
    for (const FString LandmarkName : s.CoverLandmarks)
    {
        p.CoverLandmarks.AddUnique(FindLandmarkActorFromName(LandmarkName));
    }

    return p;
}

ADoor* AWorldDataGenerator::FindDoorFromID(FVector ID) const
{
    for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
    {
        if (MakePseudoUniqueIDForDoor(*It) == ID)
        {
            return *It;
        }
    }

    return nullptr;
}

AThreatAwarenessActor* AWorldDataGenerator::FindThreatFromID(FVector ID) const
{
    for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
    {
        if (MakePseudoUniqueIDForThreat(*It) == ID)
        {
            return *It;
        }
    }

    return nullptr;
}

AActor* AWorldDataGenerator::FindActorFromName(FString Name) const
{
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        if (It->GetName() == Name)
        {
            return *It;
        }
    }

    return nullptr;
}

AStackUpActor* AWorldDataGenerator::FindStackUpActorFromName(FString Name) const
{
    for (TActorIterator<AStackUpActor> It(GetWorld()); It; ++It)
    {
        if (It->GetName() == Name)
        {
            return *It;
        }
    }

    return nullptr;
}

ACoverLandmark* AWorldDataGenerator::FindLandmarkActorFromName(FString Name) const
{
    for (TActorIterator<ACoverLandmark> It(GetWorld()); It; ++It)
    {
        if (It->GetName() == Name)
        {
            return *It;
        }
    }

    return nullptr;
}

ACoverPoint* AWorldDataGenerator::FindCoverActorFromName(FString Name) const
{
    for (TActorIterator<ACoverPoint> It(GetWorld()); It; ++It)
    {
        if (It->GetName() == Name)
        {
            return *It;
        }
    }

    return nullptr;
}

bool AWorldDataGenerator::WriteGenerationToFile(bool bForce)
{
    FString Name = GetNameSafe(GetWorld()) + "_WorldGen_" + UBpGameplayHelperLib::GetProjectVersion();
    int32 Index = UBpGameplayHelperLib::GetProjectVersionAsInt();
    
	UWorldGenSave* WorldGenSave = Cast<UWorldGenSave>(UGameplayStatics::LoadGameFromSlot(Name, Index));
    
    if (WorldGenSave && !bForce)
        return false;

    V_LOGM(LogReadyOrNot, "Missing WorldGen save file... Attempting to generate world %s", *GetWorld()->GetName());
    GenerateWorld();
    
    WorldGenSave = Cast<UWorldGenSave>(UGameplayStatics::CreateSaveGameObject(UWorldGenSave::StaticClass()));
    if (!WorldGenSave)
        return true;

    for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
    {
		// ##UE5UPGRADE##
        if (!IsValid(*It) || It->IsActorBeingDestroyed())
            continue;

        if (It->OwningRoom == NAME_None)
            continue;
        
        WorldGenSave->SavedThreatAwarenessActors.Add(ThreatActorToSavedThreatActor(*It));
    }

    for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
    {
        FSavedDoorActor s;

        s.bHasFrame = It->bHasFrame;
        s.bCanIssueOrdersOnFrontSide = It->bCanIssueOrdersOnFrontSide;
        s.bCanIssueOrdersOnBackSide = It->bCanIssueOrdersOnBackSide;

        s.FrontRoomPosition = It->FrontRoomPosition;
        s.BackRoomPosition = It->BackRoomPosition;
        
        s.FrontDoorThreat = ThreatActorToSavedThreatActor(It->FrontThreat);
        s.BackDoorThreat = ThreatActorToSavedThreatActor(It->BackThreat);

        // TAA points
        It->FrontThreatAwarenessPoints.Remove(nullptr);
        It->BackThreatAwarenessPoints.Remove(nullptr);
        
        for (AThreatAwarenessActor* t : It->FrontThreatAwarenessPoints)
        {
            s.FrontThreats.Add(ThreatActorToSavedThreatActor(t));
        }
        
        for (AThreatAwarenessActor* t : It->BackThreatAwarenessPoints)
        {
            s.BackThreats.Add(ThreatActorToSavedThreatActor(t));
        }

        // stack up points
        It->FrontLeftStackUpPoints.Remove(nullptr);
        It->FrontRightStackUpPoints.Remove(nullptr);
        It->BackLeftStackUpPoints.Remove(nullptr);
        It->BackRightStackUpPoints.Remove(nullptr);
        
        for (AStackUpActor* S : It->FrontLeftStackUpPoints)
        {
            s.FrontLeftStackups.Add(StackUpActorToSavedStackUpActor(S));
        }
        
        for (AStackUpActor* S : It->FrontRightStackUpPoints)
        {
            s.FrontRightStackups.Add(StackUpActorToSavedStackUpActor(S));
        }
        
        for (AStackUpActor* S : It->BackLeftStackUpPoints)
        {
            s.BackLeftStackups.Add(StackUpActorToSavedStackUpActor(S));
        }

        for (AStackUpActor* S : It->BackRightStackUpPoints)
        {
            s.BackRightStackups.Add(StackUpActorToSavedStackUpActor(S));
        }

        // clear points
        for (FClearPoint& P : It->FrontLeftClearPoints)
        {
            s.FrontLeftClearPoints.Add(ClearPointToSavedClearPoint(P));
        }
        
        for (FClearPoint& P : It->FrontRightClearPoints)
        {
            s.FrontRightClearPoints.Add(ClearPointToSavedClearPoint(P));
        }
        
        for (FClearPoint& P : It->BackLeftClearPoints)
        {
            s.BackLeftClearPoints.Add(ClearPointToSavedClearPoint(P));
        }
        
        for (FClearPoint& P : It->BackRightClearPoints)
        {
            s.BackRightClearPoints.Add(ClearPointToSavedClearPoint(P));
        }

        s.UniqueID = MakePseudoUniqueIDForDoor(*It);

        WorldGenSave->SavedDoorActors.Add(s);
    }
    
    for (TActorIterator<ACoverPoint> It(GetWorld()); It; ++It)
    {
        WorldGenSave->SavedCoverActors.Add(CoverActorToSavedCoverActor(*It));
    }

    for (FRoom& R : RoomData.Rooms)
    {
        WorldGenSave->SavedRooms.Add(RoomDataToSavedRoomData(R));
    }

    UGameplayStatics::SaveGameToSlot(WorldGenSave, Name, Index);

    for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
    {
        It->Init();
    }
    
    return true;
}

void AWorldDataGenerator::LoadGenerationFromFile()
{
    if (WriteGenerationToFile())
        return;

    FString Name = GetNameSafe(GetWorld()) + "_WorldGen_" + UBpGameplayHelperLib::GetProjectVersion();
    int32 Index = UBpGameplayHelperLib::GetProjectVersionAsInt();
    
    UWorldGenSave* WorldGenSave = Cast<UWorldGenSave>(UGameplayStatics::LoadGameFromSlot(Name, Index));

    // if we have a world gen saved to disk then relink the world data together...
    // if any data seems to have been corrupted, force regen the world again
    if (WorldGenSave)
    {
        bIsGenerating = true;
        
        for (TActorIterator<AThreatAwarenessActor> It(GetWorld()); It; ++It)
        {
            AThreatAwarenessActor* a = *It;
            a->Destroy();
        }

        for (TActorIterator<AStackUpActor> It(GetWorld()); It; ++It)
        {
            It->Destroy();
        }
        
        for (TActorIterator<ACoverPoint> It(GetWorld()); It; ++It)
        {
            It->Destroy();
        }

        int32 TotalDoors = 0;
        for (TActorIterator<ADoor> It(GetWorld()); It; ++It)
        {
            TotalDoors++;
        }
        
        if (WorldGenSave->SavedDoorActors.Num() != TotalDoors)
        {
            WriteGenerationToFile(true);
            return;
        }

        LOG_NUMBER(WorldGenSave->SavedThreatAwarenessActors.Num());
        LOG_NUMBER(WorldGenSave->SavedDoorActors.Num());
        LOG_NUMBER(WorldGenSave->SavedCoverActors.Num());
        LOG_NUMBER(WorldGenSave->SavedRooms.Num());
        
        ClearNullReferences();
        
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.bNoFail = true;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        
        for (FSavedThreatAwarenessActor& Threat : WorldGenSave->SavedThreatAwarenessActors)
        {
            if (AThreatAwarenessActor* T = GetWorld()->SpawnActor<AThreatAwarenessActor>(AThreatAwarenessActor::StaticClass(), Threat.Location, FRotator::ZeroRotator, SpawnParameters))
            {
                T->CheckIsOutside();
                
                T->Tags.AddUnique("generated");
                
                T->DoorThreat = Threat.Door != FVector::ZeroVector ? FindDoorFromID(Threat.Door) : nullptr;
                T->bFrontDoorThreat = Threat.bFrontDoorThreat;
                
                T->SetThreatLevel(Threat.ThreatLevel);

                //ensureAlways(!Threat.OwningRoom.IsEmpty());
                
                T->OwningRoom = FName(Threat.OwningRoom);
                T->bIsOutside = Threat.bIsOutside;
                
                for (const FVector ID : Threat.UniqueExits)
                {
                    T->UniqueExits.AddUnique(FindDoorFromID(ID));
                }

                for (const FSavedSwatLookAtPoint S : Threat.SwatLookAtPoints)
                {
                    FLookAtPoint P;
                    P.Location = S.Location;
                    P.LinkedDoor = FindDoorFromID(S.LinkedDoorID);
                    
                    T->SwatLookAtPoints.Add(P);
                }
            }
        }
        
        for (FSavedThreatAwarenessActor& Threat : WorldGenSave->SavedThreatAwarenessActors)
        {
            if (AThreatAwarenessActor* T = FindThreatFromID(Threat.UniqueID))
            {
                for (const FSavedExitData& E : Threat.Exits)
                {
                    FExitData e;
                    e.Location = E.Location;

                    for (const FSavedExitRoute& R : E.PossibleRoutes)
                    {
                        FExitRoute r;
                        r.Location = R.Location;
                        r.PathCost = R.PathCost;
                        
                        for (const FVector ID : R.Doors)
                        {
                            r.Doors.AddUnique(FindDoorFromID(ID));
                        }
                        
                        for (const FVector ID : R.ThreatsOnRoute)
                        {
                            r.ThreatsOnRoute.AddUnique(FindThreatFromID(ID));
                        }
                        
                        e.PossibleRoutes.Add(r);
                    }
                    
                    T->Exits.Add(e);
                }

                for (const FVector ID : Threat.PathableThreats)
                {
                    T->PathableThreatAwarenessActors.AddUnique(FindThreatFromID(ID));
                }
            }
        }
        
        for (const FSavedDoorActor& SavedDoor : WorldGenSave->SavedDoorActors)
        {
            //ULog::Vector(SavedDoor.UniqueID, false, "Saved Door ");

            if (ADoor* Door = FindDoorFromID(SavedDoor.UniqueID))
            {
                //ULog::Info("Found");
                
                Door->FrontLeftStackUpPoints.Empty(4);
                Door->FrontRightStackUpPoints.Empty(4);
                Door->BackLeftStackUpPoints.Empty(4);
                Door->BackRightStackUpPoints.Empty(4);

                Door->FrontLeftClearPoints.Empty(30);
                Door->FrontRightClearPoints.Empty(30);
                Door->BackLeftClearPoints.Empty(30);
                Door->BackRightClearPoints.Empty(30);

                Door->FrontThreatAwarenessPoints.Empty(50);
                Door->BackThreatAwarenessPoints.Empty(50);
                
                Door->bCanIssueOrdersOnFrontSide = SavedDoor.bCanIssueOrdersOnFrontSide;
                Door->bCanIssueOrdersOnBackSide = SavedDoor.bCanIssueOrdersOnBackSide;
                Door->bHasFrame = SavedDoor.bHasFrame;

                Door->FrontRoomPosition = SavedDoor.FrontRoomPosition;
                Door->BackRoomPosition = SavedDoor.BackRoomPosition;

                Door->FrontThreat = FindThreatFromID(SavedDoor.FrontDoorThreat.UniqueID);
                Door->BackThreat = FindThreatFromID(SavedDoor.BackDoorThreat.UniqueID);

                for (const FSavedThreatAwarenessActor& s : SavedDoor.FrontThreats)
                {
                    Door->FrontThreatAwarenessPoints.Add(FindThreatFromID(s.UniqueID));
                }
                
                for (const FSavedThreatAwarenessActor& s : SavedDoor.BackThreats)
                {
                    Door->BackThreatAwarenessPoints.Add(FindThreatFromID(s.UniqueID));
                }

                // stack up points
                for (const FSavedStackUpActor& s : SavedDoor.FrontLeftStackups)
                {
                   SpawnStackUpActorAtLocation(s.Location, Door, s.StackUpPosition, true, true, s.Depth);
                }
                
                for (const FSavedStackUpActor& s : SavedDoor.FrontRightStackups)
                {
                    SpawnStackUpActorAtLocation(s.Location, Door, s.StackUpPosition, true, false, s.Depth);
                }
                
                for (const FSavedStackUpActor& s : SavedDoor.BackLeftStackups)
                {
                  SpawnStackUpActorAtLocation(s.Location, Door, s.StackUpPosition, false, true, s.Depth);
                }
                
                for (const FSavedStackUpActor& s : SavedDoor.BackRightStackups)
                {
                   SpawnStackUpActorAtLocation(s.Location, Door, s.StackUpPosition, false, false, s.Depth);
                }

                // clear points
                for (const FSavedClearPoint& s : SavedDoor.FrontLeftClearPoints)
                {
                    Door->FrontLeftClearPoints.Add(MakeClearPointFromSavedClearPoint(s));
                }
                
                for (const FSavedClearPoint& s : SavedDoor.FrontRightClearPoints)
                {
                    Door->FrontRightClearPoints.Add(MakeClearPointFromSavedClearPoint(s));
                }
                
                for (const FSavedClearPoint& s : SavedDoor.BackLeftClearPoints)
                {
                    Door->BackLeftClearPoints.Add(MakeClearPointFromSavedClearPoint(s));
                }
                
                for (const FSavedClearPoint& s : SavedDoor.BackRightClearPoints)
                {
                    Door->BackRightClearPoints.Add(MakeClearPointFromSavedClearPoint(s));
                }
            }
        }
        
        for (const FSavedCoverActor& Saved : WorldGenSave->SavedCoverActors)
        {
            if (ACoverPoint* CoverPoint = GetWorld()->SpawnActor<ACoverPoint>(ACoverPoint::StaticClass(), Saved.Transform, SpawnParameters))
            {
                CoverPoint->CoverActor = FindActorFromName(Saved.CoverObjectName);
                CoverPoint->CoverRail = Saved.CoverRail;
                CoverPoint->bIsCrouchOnlyCover = Saved.bIsCrouchOnlyCover;
                CoverPoint->SetStandCoverType(Saved.StandCoverType);
                CoverPoint->SetCrouchCoverType(Saved.CrouchCoverType);
                CoverPoint->StandCoverDirection = Saved.StandCoverDirection;
                CoverPoint->CrouchCoverDirection = Saved.CrouchCoverDirection;
                CoverPoint->Index = Saved.Index;
            }
        }
        
        RoomData = FRoomGenData();
        
        for (const FSavedRoomData& Saved : WorldGenSave->SavedRooms)
        {
            FRoom r;
            r.Location = Saved.Location;
            r.Name = Saved.Name;
            r.ConnectingRooms = Saved.ConnectingRooms;
            r.RootDoor = FindDoorFromID(Saved.RootDoorID);

            for (FVector ID : Saved.AdditionalRootDoors)
            {
                r.AdditionalRootDoors.Add(FindDoorFromID(ID));
            }
            
            for (FVector ID : Saved.Threats)
            {
                r.Threats.Add(FindThreatFromID(ID));
            }
            
            RoomData.Rooms.Add(r);
        }

        bIsGenerating = false;

        if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
        {
            GS->RoomData = &RoomData;
        }
        
        for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
        {
            It->Init();
        }
        
        if (UThreatAwarenessSubsystem* Subsystem = GetWorld()->GetSubsystem<UThreatAwarenessSubsystem>())
        {
            Subsystem->OnWorldGenerated();
        }
    }
}

// Void Interactive, 2020

#include "ThreatAwarenessActor.h"

#include "Door.h"
#include "NavigationSystem.h"
#include "NavLinkCustomComponent.h"
#include "WorldDataGenerator.h"

#include "Characters/CyberneticController.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Navigation/NavLinkProxy.h"
#include "Navigation/ReadyOrNotNavQueries.h"

DECLARE_CYCLE_STAT(TEXT("ThreatAwarenesActor ~ Tick"), STAT_ThreatAwarenesActorTick, STATGROUP_ThreatAwarenesActor);
DECLARE_CYCLE_STAT(TEXT("ThreatAwarenesActor ~ Editor Tick"), STAT_ThreatAwarenesActorEditorTick, STATGROUP_ThreatAwarenesActor);
DECLARE_CYCLE_STAT(TEXT("ThreatAwarenesActor ~ Editor Tick (Exit Routes)"), STAT_ThreatAwarenesActorEditorTick_ExitRoutes, STATGROUP_ThreatAwarenesActor);
DECLARE_CYCLE_STAT(TEXT("ThreatAwarenesActor ~ Calculate Police Prescence"), STAT_TAACalculatePolicePrescence, STATGROUP_ThreatAwarenesActor);

TAutoConsoleVariable<int32> CVarRonDrawThreatAwareness(TEXT("a.RonDrawThreatAwareness"), 1, TEXT("0 = No draw threat awareness actors. 1 = Draw all threat awareness actors"));

#if WITH_EDITORONLY_DATA
static UTexture2D* LowThreatSprite = nullptr;
static UTexture2D* MediumThreatSprite = nullptr;
static UTexture2D* HighThreatSprite = nullptr;
static UTexture2D* ExtremeThreatSprite = nullptr;
static UTexture2D* DoorThreatSprite = nullptr;
#endif

AThreatAwarenessActor::AThreatAwarenessActor()
{
	#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.066f;
	#else
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 1.0f;
	#endif
	
	bCanBeInCluster = false;

	bFindCameraComponentWhenViewTarget = false;
	SetCanBeDamaged(false);
	
	DefaultScene = CreateDefaultSubobject<USceneComponent>("DefaultScene");
	DefaultScene->SetMobility(EComponentMobility::Static);
	SetRootComponent(DefaultScene);
	
	#if WITH_EDITORONLY_DATA
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("BillboardComp");
	BillboardComponent->ScreenSize = 0.004f;
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->SetMobility(EComponentMobility::Static);
	BillboardComponent->SetRelativeLocation(FVector::ZeroVector);
	BillboardComponent->SetRelativeScale3D(FVector(0.3f));
	BillboardComponent->SetupAttachment(DefaultScene);
	BillboardComponent->SetCachedMaxDrawDistance(1000.0f);
	
	static ConstructorHelpers::FObjectFinder<UTexture2D> LowThreat(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/THREAT_LEVEL_LOW.THREAT_LEVEL_LOW'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> MediumThreat(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/THREAT_LEVEL_MEDIUM.THREAT_LEVEL_MEDIUM'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> HighThreat(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/THREAT_LEVEL_HIGH.THREAT_LEVEL_HIGH'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> ExtremeThreat(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/THREAT_LEVEL_EXTREME.THREAT_LEVEL_EXTREME'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> DoorThreatObj(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/THREAT_LEVEL_DOOR.THREAT_LEVEL_DOOR'"));
	LowThreatSprite = LowThreat.Object;
	MediumThreatSprite = MediumThreat.Object;
	HighThreatSprite = HighThreat.Object;
	ExtremeThreatSprite = ExtremeThreat.Object;
	DoorThreatSprite = DoorThreatObj.Object;
	#endif
}

#if WITH_EDITOR
void AThreatAwarenessActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	SetSpriteFromThreatLevel();
}

void AThreatAwarenessActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);	
	SetSpriteFromThreatLevel();
}

void AThreatAwarenessActor::PostActorCreated()
{
	Super::PostActorCreated();
	SetSpriteFromThreatLevel();
}

bool AThreatAwarenessActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AThreatAwarenessActor::EditorTick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_ThreatAwarenesActorEditorTick)

	if (CVarRonDrawThreatAwareness.GetValueOnAnyThread() <= 0)
	{
		BillboardComponent->SetVisibility(false);
		return;
	}

	BillboardComponent->SetVisibility(true);

	PathableThreatAwarenessActors.Remove(nullptr);

	{
		SCOPE_CYCLE_COUNTER(STAT_ThreatAwarenesActorEditorTick_ExitRoutes)

		if (IsSelectedInEditor())
		{
			for (FExitData Exit : Exits)
			{
				for (FExitRoute Route : Exit.PossibleRoutes)
				{
					Route.ThreatsOnRoute.Remove(nullptr);
					if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
					{
						FPathFindingQuery PathFindingQuery;
						PathFindingQuery.StartLocation = GetActorLocation();
						PathFindingQuery.EndLocation = Exit.Location;
						PathFindingQuery.SetAllowPartialPaths(false);
						const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_NoiseCheck::StaticClass();
						const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
						PathFindingQuery.QueryFilter = QueryFilter;
						const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Hierarchical);
						if (PathFindingResult.Result == ENavigationQueryResult::Success)
						{					
							for (int32 i = 1; i < PathFindingResult.Path->GetPathPoints().Num(); i++)
							{
								DrawDebugLine(GetWorld(),  PathFindingResult.Path->GetPathPoints()[i-1], PathFindingResult.Path->GetPathPoints()[i], FColor::White, false, DeltaTime + 0.001f, 0, 1);
							}
						}
					}
				
					for (int32 i = 1; i < Route.ThreatsOnRoute.Num(); i++)
					{
						DrawDebugLine(GetWorld(),  Route.ThreatsOnRoute[i-1]->GetActorLocation(), Route.ThreatsOnRoute[i]->GetActorLocation(), FColor::Red, false, DeltaTime + 0.001f, 0, 1);
					}
				}
			}

			for (const FLookAtPoint& LookAtPoint : SwatLookAtPoints)
			{
				if (LookAtPoint.Location != FIntVector::ZeroValue)
				{
					DrawDebugLine(GetWorld(), GetActorLocation(), FVector(LookAtPoint.Location), FColor::Blue, false, 0.65f, 0, 1);
				}
			}
		}
	}

	if (BillboardComponent->GetRelativeLocation() != FVector::ZeroVector)
		BillboardComponent->SetRelativeLocation(FVector::ZeroVector);
}
#endif

void AThreatAwarenessActor::BeginPlay()
{
	Super::BeginPlay();
	
	DefaultScene->SetMobility(EComponentMobility::Static);

	PathableThreatAwarenessActors.Remove(nullptr);

	if (const AWorldDataGenerator* Gen = AWorldDataGenerator::Get(GetWorld()))
	{
		if (!Gen->bIsGenerating)
		{
			TryDestroyIfInvalid();
		}
	}
}

void AThreatAwarenessActor::CheckIsOutside()
{
	bIsOutside = !GetWorld()->LineTraceTestByObjectType(GetActorLocation() + FVector(0.0f, 0.0f, 100.0f), GetActorLocation() + FVector(0.0f, 0.0f, 15000.0f), FCollisionObjectQueryParams(ECC_WorldStatic));
	//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + FVector(0.0f, 0.0f, 100.0f), bIsOutside ? FColor::Red : FColor::Green, false, 120.0f, 0, 1);
}

void AThreatAwarenessActor::GenerateUniqueExits()
{
	UniqueExits.Empty();
	for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
	{
		if (It->IsDoorThreat() && It->PathableThreatAwarenessActors.Contains(this))
		{
			UniqueExits.AddUnique(It->GetAttachedDoor());
		}
	}
}

void AThreatAwarenessActor::GeneratePossibleRoutes()
{
	for (int32 i = 0; i < Exits.Num(); i++)
	{
		FExitData* e = &Exits[i];
		for (ADoor* d : UniqueExits)
		{
			FExitRoute Route;
			if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
			{
			
				FPathFindingQuery PathFindingQuery;
				PathFindingQuery.StartLocation = d->NavLinkStart;
				PathFindingQuery.EndLocation = e->Location;
				PathFindingQuery.SetAllowPartialPaths(false);
				const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_NoiseCheck::StaticClass();
				const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
				PathFindingQuery.QueryFilter = QueryFilter;
				const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Hierarchical);
				if (PathFindingResult.Result == ENavigationQueryResult::Success)
				{
					for (int32 y = 0; y < PathFindingResult.Path->GetPathPoints().Num(); y++)
					{
						if (PathFindingResult.Path->GetPathPoints()[y].CustomLinkId)
						{
							// we only need the first door to determine unique exits
							for (TActorIterator<ANavLinkProxy>NavIt(GetWorld()); NavIt; ++NavIt)
							{
								if (NavIt->GetSmartLinkComp()->GetLinkId() == PathFindingResult.Path->GetPathPoints()[y].CustomLinkId)
								{
									Route.Doors.AddUnique(Cast<ADoor>(NavIt->GetOwner()));
								}
							}			
						}
						if (Route.Doors.Num() > 0)
						{
							if (PathFindingResult.Path->GetPathPoints().IsValidIndex(y+1))
							{
								Route.ThreatsOnRoute.AddUnique(AWorldDataGenerator::Get(GetWorld())->GetNearestThreat(PathFindingResult.Path->GetPathPoints()[y+1].Location, false, false));
							} else if (PathFindingResult.Path->GetPathPoints().IsValidIndex(y))
							{
								Route.ThreatsOnRoute.AddUnique(AWorldDataGenerator::Get(GetWorld())->GetNearestThreat(PathFindingResult.Path->GetPathPoints()[y].Location, false, false));
							}
						}
					}
					Route.PathCost = PathFindingResult.Path->GetLength();
				}
			}
			if (Route.Doors.Num() > 0 && Route.PathCost != -1.0f)
			{
				e->PossibleRoutes.AddUnique(Route);
			}
		}
	}
	for (int32 i = 0; i < Exits.Num(); i++)
	{
		Exits[i].PossibleRoutes.Sort([](const FExitRoute& Lhs, const FExitRoute& Rhs)
		{
			return Lhs.PathCost < Rhs.PathCost;
		});
		
		if (Exits[i].PossibleRoutes.Num() == 0)
		{
			Exits.RemoveAt(i);
			if (i>0)
				i--;
		}
		
	}
}

void AThreatAwarenessActor::RemoveAnyVisibleExits()
{
	for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
	{
		AThreatAwarenessActor* Threat = *It;
		if (Threat == this)
			continue;

		if (!UniqueExitsMatch(Threat))
		{
			// test for visibility
			FHitResult Hit;
			GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Threat->GetActorLocation(), ECC_Visibility);
			if (!Hit.bBlockingHit)
			{
				for (AThreatAwarenessActor* t : Threat->PathableThreatAwarenessActors)
				{
					for (int32 i = 0; i < Exits.Num(); i++)
					{
						for (int32 y = 0; y < Exits[i].PossibleRoutes.Num(); y++)
						{
							if (Exits.IsValidIndex(i) )
							{
								if (Exits[i].PossibleRoutes[y].ThreatsOnRoute.Contains(Threat))
								{
									Exits[i].PossibleRoutes.RemoveAt(y);
									y = FMath::Max(0, y-1);
								}
								if (Exits[i].PossibleRoutes.Num() == 0)
								{
									Exits.RemoveAt(i);
									y = FMath::Max(0, i-1);
								}
							}
						}
					}
				}
			}
		}
	}
}

bool AThreatAwarenessActor::UniqueExitsMatch(AThreatAwarenessActor* OtherThreat)
{
	if (!OtherThreat)
		return false;

	if (OtherThreat->UniqueExits.Num() != UniqueExits.Num())
		return false;
	
	for (ADoor* Door : OtherThreat->UniqueExits)
	{
		if (!UniqueExits.Contains(Door))
			return false;
	}
	for (ADoor* Door : UniqueExits)
	{
		if (!OtherThreat->UniqueExits.Contains(Door))
			return false;
	}
	return true;
}

void AThreatAwarenessActor::SetSpriteFromThreatLevel()
{
	#if WITH_EDITORONLY_DATA
	if (DoorThreat)
	{
		BillboardComponent->SetSprite(DoorThreatSprite);
		return;
	}
	
	switch(ThreatLevel)
	{
		case EThreatLevel::TL_None:		BillboardComponent->SetSprite(nullptr);	break;
		case EThreatLevel::TL_Low:		BillboardComponent->SetSprite(LowThreatSprite); break;
		case EThreatLevel::TL_Medium:	BillboardComponent->SetSprite(MediumThreatSprite); break;
		case EThreatLevel::TL_High:		BillboardComponent->SetSprite(HighThreatSprite); break;
		case EThreatLevel::TL_Extreme:	BillboardComponent->SetSprite(ExtremeThreatSprite); break;
		case EThreatLevel::TL_Stairs:	BillboardComponent->SetSprite(ExtremeThreatSprite); break;
		default: ;
	}
	#endif
}

void AThreatAwarenessActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SCOPE_CYCLE_COUNTER(STAT_ThreatAwarenesActorTick)

	#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		EditorTick(DeltaTime);
		return;
	}
	#endif

	SetActorTickEnabled(false);
	PrimaryActorTick.UnRegisterTickFunction();
}

void AThreatAwarenessActor::SetThreatLevel(EThreatLevel tl)
{
	//#if WITH_EDITORONLY_DATA
	//BillboardComponent->ScreenSize = BillboardComponent->ScreenSize * (uint8)ThreatLevel;
	//#endif

	ThreatLevel = tl;
	SetSpriteFromThreatLevel();
}

void AThreatAwarenessActor::GenerateLookAtPoints()
{
	AWorldDataGenerator::Get(GetWorld())->GenerateSwatLookAtPoint(this);
}

void AThreatAwarenessActor::CalculateExits()
{
	Exits.Empty();
	TArray<ADoor*> FirstDoors;

	TArray<AThreatAwarenessActor*> IgnoredThreats;
	AWorldDataGenerator::Get(GetWorld())->GetOrCreateIgnoredExitThreats(IgnoredThreats);
	for (TActorIterator<AThreatAwarenessActor>It(GetWorld()); It; ++It)
	{
		AThreatAwarenessActor* Threat = *It;

		if (IgnoredThreats.Contains(Threat))
		 	continue;
		
		FPathFindingQuery PathFindingQuery;
		if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			if (NavSys->MainNavData)
			{
				bool bSkip = false;
				for (FExitData e : Exits)
				{
					float Dist = (e.Location - Threat->GetActorLocation()).Size();
					// don't try and stack heaps of exits that are visible to each other on top of each other
					FHitResult LOSTest;
					FCollisionObjectQueryParams CollisionObjectQueryParams;
					CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
					CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOORWAY);
					GetWorld()->LineTraceSingleByObjectType(LOSTest, e.Location, Threat->GetActorLocation(), CollisionObjectQueryParams);
					if (Dist < 1500.0f && !LOSTest.bBlockingHit)
					{
						bSkip = true;
					}
				}
				if (bSkip)
					continue;
				
				PathFindingQuery.StartLocation = GetActorLocation();
				PathFindingQuery.EndLocation = Threat->GetActorLocation();
				PathFindingQuery.SetAllowPartialPaths(false);
				const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_NoiseCheck::StaticClass();
				const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
				PathFindingQuery.QueryFilter = QueryFilter;
				const FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Hierarchical);
				if (PathFindingResult.Result == ENavigationQueryResult::Success)
				{
					FExitData SuspectExit; 
					SuspectExit.Location = PathFindingQuery.EndLocation;
					Exits.Add(SuspectExit);				
				}
			}
		}
	}
}

bool AThreatAwarenessActor::GetRandomExitDoor(ADoor*& Door) const
{
	if (UniqueExits.Num() > 0)
	{
		Door = UniqueExits[FMath::RandRange(0, UniqueExits.Num() - 1)];
		return true;
	}
	return false;
}

bool AThreatAwarenessActor::HasExit() const
{
	if (UniqueExits.Num() <= 1)
	{
		return false;
	}
	for (ADoor* Door : UniqueExits)
	{
		if (Door->IsLocked() || Door->IsJammed())
		{
			continue;
		}
		return true;
	}
	return false;
}

void AThreatAwarenessActor::TryDestroyIfInvalid()
{
	if (!DoorThreat)
	{
		//ensureAlways(OwningRoom != NAME_None);
		if (OwningRoom == NAME_None)
		{
			Destroy();
		}
	}
}

bool FExitRoute::IsValidRoute() const
{
	for (const ADoor* d : Doors)
	{
		if (d)
		{
			if (d->IsLocked() || d->IsJammed())
				return false;
		}
	}

	return true;
}

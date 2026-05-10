// Copyright Void Interactive, 2021

#include "AISpawn.h"

#include "AIController.h"
#include "Actors/Door.h"

#include "NavigationSystem.h"

#include "Info/WorldBuildingPlacementActor.h"

#if WITH_EDITOR
#include "EditorViewportClient.h"
#endif

TAutoConsoleVariable<int32> CVarRonDrawAISpawns(TEXT("a.RonDrawAISpawns"), 1, TEXT("0 = No draw AI spawners. 1 = Draw all AI spawners"));

#if WITH_EDITORONLY_DATA
static UTexture2D* CivSpawnTexture = nullptr;
static UTexture2D* CivSpawnOffTexture = nullptr;
static UTexture2D* SusSpawnTexture = nullptr;
static UTexture2D* SusSpawnOffTexture = nullptr;
#endif

AAISpawn::AAISpawn()
{
	#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.0167f;
	#else
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 1.0f;
	#endif

	bEnabled = true;
	
	DefaultScene = CreateDefaultSubobject<USceneComponent>("DefaultScene");
	DefaultScene->SetMobility(EComponentMobility::Static);
	SetRootComponent(DefaultScene);

	#if WITH_EDITORONLY_DATA
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillboardComponent"));
	BillboardComponent->ScreenSize = 0.004f;
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->SetMobility(EComponentMobility::Static);
	BillboardComponent->SetRelativeLocation(FVector::ZeroVector);
	BillboardComponent->SetRelativeScale3D(FVector(0.85f));
	BillboardComponent->SetupAttachment(RootComponent);
	BillboardComponent->SetCachedMaxDrawDistance(1000.0f);

	static ConstructorHelpers::FObjectFinder<UTexture2D> CivSpawn(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/CivSpawn.CivSpawn'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CivSpawnOff(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/CivSpawn_Off.CivSpawn_Off'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> SusSpawn(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/SusSpawn.SusSpawn'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> SusSpawnOff(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/SusSpawn_Off.SusSpawn_Off'"));
	SusSpawnTexture = SusSpawn.Object;
	SusSpawnOffTexture = SusSpawnOff.Object;
	CivSpawnTexture = CivSpawn.Object;
	CivSpawnOffTexture = CivSpawnOff.Object;

	UpdateTexture();
	#endif

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnDirection"));
	SpawnDirection->SetWorldScale3D(FVector::OneVector);
	SpawnDirection->SetupAttachment(RootComponent);
	
	bReplicates = false;
	SetCanBeDamaged(false);
	
	bFindCameraComponentWhenViewTarget = false;
}

ETeamType AAISpawn::GetSpawningTeamType(const FSpawnData& Sd)
{
	if (Sd.SpawnedAI.DataTable)
	{
		if (const FAIDataLookupTable* AIData = Sd.SpawnedAI.DataTable->FindRow<FAIDataLookupTable>(Sd.SpawnedAI.RowName, "GetSpawningTeamType", false))
		{
			return AIData->SpawningTeamType;
		}
	}
	
	return ETeamType::TT_NONE;
}

void AAISpawn::BeginPlay()
{
	Super::BeginPlay();

	// Handle old spawn data for modded maps, since we refactored this class a bit.
	if (SpawnArray.Num() == 0)
	{
		if (!SpawnData.SpawnedAI.IsNull())
		{
			SpawnArray.Add(SpawnData);
		}
		else
		{
			// fallback for modded maps if AI data is missing, better to spawn something rather than nothing
			#if !WITH_EDITOR
			const bool bSpawnCiv = FMath::FRand() < 0.35f; // random chance between suspects and civs
			FSpawnData Fallback;
			Fallback.bDeactivated = false;
			Fallback.SpawnedAI.DataTable = UBpGameplayHelperLib::GetAILookupDataTable();
			Fallback.SpawnedAI.RowName = bSpawnCiv ? "Civilians_Hotel_SusCivilian" : "Suspects_HotelRoster_LLP_L1";
			ArchetypeOverride = nullptr;
			SpawnData = Fallback;
			SpawnArray.Add(Fallback);
			#endif
		}
	}

	if (SpawnArray.Num() > 0)
	{
		SpawnData = SpawnArray[FMath::RandRange(0, SpawnArray.Num()-1)];
	}
}

void AAISpawn::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		EditorTick(DeltaTime);
		return;
	}
	#endif
	
	SetActorTickEnabled(false);
	SetActorTickInterval(1.0f);

	PrimaryActorTick.UnRegisterTickFunction();
}

#if WITH_EDITOR
void AAISpawn::PostLoad()
{
	Super::PostLoad();

	for (FSpawnData& s : SpawnArray)
		s.Name = s.SpawnedAI.RowName;
}

void AAISpawn::EditorTick(float DeltaTime)
{
	ISGAMEVIEWRETURN();
	
	#if WITH_EDITORONLY_DATA
	UpdateTexture();
	#endif

	BillboardComponent->SetRelativeLocation(FVector::ZeroVector);

	// Draw activity routes
	if (IsSelectedInEditor())
	{
		if (!bDisableBoundsCheck)
		{
			if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
			{
				FNavLocation NewLocationProjected(GetActorLocation());
				if (NavSys->ProjectPointToNavigation(GetActorLocation(), NewLocationProjected, FVector(75.0f, 75.0f, 200.0f)))
					SetActorLocation(FVector(NewLocationProjected.Location.X, NewLocationProjected.Location.Y, NewLocationProjected.Location.Z + 100.0f));
			}
		}

		for (FSpawnData& Data : SpawnArray)
		{
			FVector LastLocation = GetActorLocation();
			for (const FActivityRoute& Route : Data.ActivityRouteCollection.ActivityRoutes)
			{
				if (Route.WorldBuildingPlacementActor)
				{
					DrawDebugLine(GetWorld(), LastLocation, Route.WorldBuildingPlacementActor->GetActorLocation(), FColor::White, false, 0.05f, 0, 1.5f);
					LastLocation = Route.WorldBuildingPlacementActor->GetActorLocation();
				}
			}
		}

		/*
		for (FSpawnData& Sp : SpawnArray)
		{
			TArray<FActivityRoute>& ActivityRoutes = Sp.ActivityRouteCollection.ActivityRoutes;
			FColor& PathColor = Sp.ActivityRouteCollection.PathColor;

			// visualize world building paths
			if (ActivityRoutes.Num() > 0)
			{
				for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
				{
					ADoor* Door = *It;
					Door->EnableNavLink();
				}
				
				for (int32 i = 0; i < ActivityRoutes.Num(); i++)
				{
					FActivityRoute CurrentActivityRoute = ActivityRoutes[i];

					if (!CurrentActivityRoute.WorldBuildingPlacementActor)
						continue;

					FVector StartLocation = FVector::ZeroVector;
					FVector NextLocation = FVector::ZeroVector;

					if (i == 0)
					{
						StartLocation = GetActorLocation();
						NextLocation = CurrentActivityRoute.WorldBuildingPlacementActor->GetActorLocation();
					}
					else
					{
						FActivityRoute PrevActivityRoute = ActivityRoutes[i-1];

						StartLocation = PrevActivityRoute.WorldBuildingPlacementActor->GetActorLocation();
						NextLocation = CurrentActivityRoute.WorldBuildingPlacementActor->GetActorLocation();
					}

					if (UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
					{
						FNavLocation StartLocationProjected, EndLocationProjected;

						NavSys->ProjectPointToNavigation(StartLocation, StartLocationProjected, FVector(75.0f, 75.0f, 100.0f));
						NavSys->ProjectPointToNavigation(NextLocation, EndLocationProjected, FVector(75.0f, 75.0f, 100.0f));

						FPathFindingQuery PathFindingQuery;

						PathFindingQuery.StartLocation = StartLocationProjected;
						PathFindingQuery.EndLocation = EndLocationProjected;
						PathFindingQuery.SetAllowPartialPaths(true);

						TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_Suspect::StaticClass();
						FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavSys->MainNavData, FilterClass);
						PathFindingQuery.QueryFilter = QueryFilter;
						
						FPathFindingResult PathFindingResult = NavSys->FindPathSync(PathFindingQuery, EPathFindingMode::Hierarchical);
						
						if (PathFindingResult.Result == ENavigationQueryResult::Success)
						{
							FNavPathSharedPtr NavPath = PathFindingResult.Path;
							TArray<FNavPathPoint> PathPoints = NavPath->GetPathPoints();
							for (int32 y = 1; y < PathPoints.Num(); y++)
							{
								DrawDebugLine(GetWorld(), PathPoints[y-1].Location + FVector(0.0f, 0.0f, 30.0f), PathPoints[y].Location + FVector(0.0f, 0.0f, 30.0f), PathColor, false, DeltaTime+0.02f, 0, 3);
							}
						}
					}
				}
				
				for (TActorIterator<ADoor>It(GetWorld()); It; ++It)
				{
					ADoor* Door = *It;
					Door->DisableNavLink();
				}
			}
		}
		*/
	}
}

void AAISpawn::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UpdateTexture();

	for (FSpawnData& s : SpawnArray)
	{
		s.Name = s.SpawnedAI.RowName;
	}
}

void AAISpawn::PostEditMove(const bool bFinished)
{
	Super::PostEditMove(bFinished);
	
	UpdateTexture();
}
#endif

bool AAISpawn::GetCivilianSpawnData(TArray<const FSpawnData*>& OutSpawnData) const
{
	for (const FSpawnData& s : SpawnArray)
	{
		if (GetSpawningTeamType(s) == ETeamType::TT_CIVILIAN)
		{
			OutSpawnData.AddUnique(&s);
		}
	}
	
	return OutSpawnData.Num() > 0;
}

bool AAISpawn::GetSuspectSpawnData(TArray<const FSpawnData*>& OutSpawnData) const
{
	for (const FSpawnData& S : SpawnArray)
	{
		if (GetSpawningTeamType(S) == ETeamType::TT_SUSPECT)
		{
			OutSpawnData.AddUnique(&S);
		}
	}
	
	return OutSpawnData.Num() > 0;
}

#if WITH_EDITORONLY_DATA
void AAISpawn::UpdateTexture()
{
	BillboardComponent->SetVisibility(CVarRonDrawAISpawns.GetValueOnAnyThread() > 0);

	if (SpawnArray.Num() > 0)
	{
		switch (GetSpawningTeamType(SpawnArray[0]))
		{
			case ETeamType::TT_SUSPECT:
			{
				if (bEnabled)
				{
					if (BillboardComponent->Sprite != SusSpawnTexture)
						BillboardComponent->SetSprite(SusSpawnTexture);
				}
				else
				{
					if (BillboardComponent->Sprite != SusSpawnOffTexture)
						BillboardComponent->SetSprite(SusSpawnOffTexture);
				}
			}
			break;
			
			case ETeamType::TT_CIVILIAN:
			{
				if (bEnabled)
				{
					if (BillboardComponent->Sprite != CivSpawnTexture)
						BillboardComponent->SetSprite(CivSpawnTexture);
				}
				else
				{
					if (BillboardComponent->Sprite != CivSpawnOffTexture)
						BillboardComponent->SetSprite(CivSpawnOffTexture);
				}
			}
			break;
			
			default:
			break;
		}
	}
}
#endif

bool AAISpawn::DoSpawn()
{
	if (!bEnabled)
	{
		#if !UE_BUILD_SHIPPING
		ULog::Info("Could not spawn AI (" + GetName() + "). Spawner is disabled");
		#endif

		return false;
	}
	
	if (!SpawnData.SpawnedAI.DataTable)
	{
		#if !UE_BUILD_SHIPPING
		ULog::Error("Could not spawn AI (" + GetName() + "). No AI Data Table specified on the spawner");
		#endif

		return false;
	}
	
	if (GetActorLocation() == FVector::ZeroVector)
	{
		#if !UE_BUILD_SHIPPING
		ULog::Error("Spawning AI at world location (0, 0, 0) is not allowed");
		#endif
		
		return false;
	}
	
	if (!HasAuthority())
	{
		#if WITH_EDITOR
		ULog::Warning("Could not spawn AI (" + GetName() + "). It does not have network authority");
		#endif
		
		return false;
	}

	if (GlobalWeaponOverride != nullptr)
	{
		SpawnData.ForceWeaponOverride = GlobalWeaponOverride;
	}

	if (const FAIDataLookupTable* AIData = SpawnData.SpawnedAI.GetRow<FAIDataLookupTable>("DoSpawn"))
	{
		if (!AIData->CharacterClass)
		{
			#if !UE_BUILD_SHIPPING
			ULog::Error("Could not spawn AI from " + GetName() + ". 'CharacterClass' in row (" + SpawnData.SpawnedAI.RowName.ToString() + ") is invalid");
			#endif

			return false;
		}
		
		if (AIData->SpawningTeamType == ETeamType::TT_NONE)
		{
			#if !UE_BUILD_SHIPPING
			ULog::Error("Could not spawn AI from " + GetName() + ". Cannot spawn an AI with no team");
			#endif

			return false;
		}
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.bNoFail = true;
		SpawnParams.bDeferConstruction = true;
		SpawnParams.bAllowDuringConstructionScript = false;

		if (ACyberneticCharacter* ai = GetWorld()->SpawnActor<ACyberneticCharacter>(AIData->CharacterClass, FTransform(SpawnDirection->GetComponentRotation(), GetActorLocation()), SpawnParams))
		{
			if (SpawnedCharacter)
			{
				SpawnedCharacter->Destroy();
				SpawnedCharacter = nullptr;
			}
			
			SpawnedCharacter = ai;
			
			ai->SetDefaultTeam(AIData->SpawningTeamType); // so this doesn't screw things up later down the line
			ai->DestroyPlayerMarkerComponent();

			#if !UE_BUILD_SHIPPING
			ai->bDeactivated = SpawnData.bDeactivated;
			#else
			ai->bDeactivated = false;
			#endif

			ai->FinishSpawning(ai->GetActorTransform());
			ai->FinishAISpawning(this, AIData);

			return true;
		}
	}
	else
	{
		#if !UE_BUILD_SHIPPING
		ULog::Error("Could not find row (" + SpawnData.SpawnedAI.RowName.ToString() + "). Make sure that the row exists in " + SpawnData.SpawnedAI.DataTable->GetName());
		#endif

		return false;
	}

	return true;
}

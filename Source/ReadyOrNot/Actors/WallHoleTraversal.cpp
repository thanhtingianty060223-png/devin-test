// Copyright Void Interactive, 2022

#include "Actors/WallHoleTraversal.h"

#include "NavigationSystem.h"
#include "NavLinkCustomComponent.h"
#include "Navigation/NavLinkProxy.h"
#include "Navigation/ReadyOrNotNavAreas.h"
#include "Navigation/ReadyOrNotNavQueries.h"

AWallHoleTraversal::AWallHoleTraversal()
{
	#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	PrimaryActorTick.TickInterval = 0.0f;
	#else
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	PrimaryActorTick.TickInterval = 0.033f;
	#endif
	
	SetCanBeDamaged(false);
	SetActorEnableCollision(false);
	bFindCameraComponentWhenViewTarget = false;
	bBlockInput = true;
	bRelevantForLevelBounds = false;
	AutoReceiveInput = EAutoReceiveInput::Disabled;
	bEnableAutoLODGeneration = false;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));
	SetRootComponent(SceneComponent);
	SceneComponent->SetMobility(EComponentMobility::Static);
	SceneComponent->SetCanEverAffectNavigation(false);

	#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> GoodCover_Icon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/CoverIcon.CoverIcon'"));

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard Component"));
	BillboardComponent->SetupAttachment(GetRootComponent());
	BillboardComponent->SetRelativeLocation(FVector::ZeroVector);
	BillboardComponent->SetWorldScale3D(FVector(0.85f));
	BillboardComponent->SetMobility(EComponentMobility::Static);
	BillboardComponent->SetCanEverAffectNavigation(false);
	BillboardComponent->bEnableAutoLODGeneration = false;
	BillboardComponent->bReceiveMobileCSMShadows = false;
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->ScreenSize = 0.0035f;
	BillboardComponent->SetSprite(GoodCover_Icon.Object);

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow Component"));
	ArrowComponent->SetupAttachment(GetRootComponent());
	ArrowComponent->SetRelativeLocation(FVector::ZeroVector);
	#endif

	bEnabled = true;
}

void AWallHoleTraversal::AddCooldownFor(AController* InController, const float InCooldownTime)
{
	CooldownMap.Remove(InController);
	
	if (InCooldownTime > 0.0f)
		CooldownMap.Add(InController, InCooldownTime);
}

bool AWallHoleTraversal::IsCooldownActiveFor(const AController* InController) const
{
	return CooldownMap.Find(InController) != nullptr;
}

void AWallHoleTraversal::BeginPlay()
{
	Super::BeginPlay();

	if (!bEnabled)
	{
		Destroy();
		return;
	}

	EntryTransform = CalculateEntryTransform();
	ExitTransform = CalculateExitTransform();

	// Create the navlink to help AI navigate through this hole
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		if (const ANavigationData* NavData = NavSys->GetNavDataForProps(AReadyOrNotCharacter::StaticClass()->GetDefaultObject<AReadyOrNotCharacter>()->GetNavAgentPropertiesRef()))
		{
			NavLinkProxy = GetWorld()->SpawnActorDeferred<ANavLinkProxy>(ANavLinkProxy::StaticClass(), FTransform());
			if (NavLinkProxy)
			{
				NavLinkProxy->FinishSpawning(NavLinkProxy->GetActorTransform());
				
				const TSubclassOf<UNavigationQueryFilter> FilterClass = UNavQuery_Suspect::StaticClass();
				
				NavLinkProxy->SetOwner(this);
				NavLinkProxy->GetSmartLinkComp()->SetNavigationRelevancy(true);
				// ##UE5.3UPGRADE##
				// NavLinkProxy->GetSmartLinkComp()->UpdateLinkId(GetUniqueID());
				// ##UE5.3UPGRADE##
				NavLinkProxy->bSmartLinkIsRelevant = true;
				
				const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, this, FilterClass);
				//const FVector Extent = FVector(50.0f, 50.0f, 50.0f);
				const FVector Extent = FVector::ZeroVector;

				FNavLocation RelativeStart(EntryTransform.GetLocation());
				FNavLocation RelativeEnd(ExitTransform.GetLocation());
				
				NavSys->ProjectPointToNavigation(EntryTransform.GetLocation(), RelativeStart, Extent, NavData, QueryFilter);
				NavSys->ProjectPointToNavigation(ExitTransform.GetLocation(), RelativeEnd, Extent, NavData, QueryFilter);
				
				const FVector EntryLocation = EntryTransform.GetLocation() + (GetActorLocation() - EntryTransform.GetLocation()).GetSafeNormal2D() * -NavLinkProxyDistance;
				const FVector ExitLocation = ExitTransform.GetLocation() + (GetActorLocation() - ExitTransform.GetLocation()).GetSafeNormal2D() * -NavLinkProxyDistance;
				
				NavLinkProxy->PointLinks.Empty();
				NavLinkProxy->GetSmartLinkComp()->SetLinkData(EntryLocation, ExitLocation, ENavLinkDirection::BothWays);

				NavSys->UpdateActorInNavOctree(*NavLinkProxy);
				NavSys->UpdateComponentInNavOctree(*NavLinkProxy->GetSmartLinkComp());
				NavSys->RegisterCustomLink(*NavLinkProxy->GetSmartLinkComp());
				
				NavLinkProxy->GetSmartLinkComp()->SetEnabledArea(UNavArea_WallTraversalHole::StaticClass());
				NavLinkProxy->SetSmartLinkEnabled(true);
				NavLinkProxy->GetSmartLinkComp()->Activate();
			}
		}
	}
	
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllWallHoles.AddUnique(this);
	}
}

void AWallHoleTraversal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllWallHoles.Remove(this);
	}
}

void AWallHoleTraversal::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	#if WITH_EDITOR
	if (!IsSelectedInEditor())
		return;

	DrawDebug();
	#endif
	
	TArray<AController*> KeysToRemoveFromMap;
	for (auto& Element : CooldownMap)
	{
		Element.Value -= DeltaTime;
		
		if (Element.Value <= 0.0f)
			KeysToRemoveFromMap.AddUnique(Element.Key);
	}

	for (const AController* Controller : KeysToRemoveFromMap)
	{
		CooldownMap.Remove(Controller);
	}
}

#if WITH_EDITOR
void AWallHoleTraversal::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.GetPropertyName() == "Name")
	{
		SetActorLabel("HoleTraversal_" + Name.Replace(TEXT(" "), TEXT("_")) + "_" + FString::FromInt(GetUniqueID()));
	}
}
#endif

#if !UE_BUILD_SHIPPING
void AWallHoleTraversal::DrawDebug()
{
	FVector EntryStartLocation = GetActorLocation() + EntryTriggerBoxTransform.GetLocation();
	FVector ExitStartLocation = GetActorLocation() + ExitTriggerBoxTransform.GetLocation();

	const float EntryX = GetActorLocation().X + -FVector2D::Distance(FVector2D(EntryStartLocation), FVector2D(GetActorLocation())) * FMath::Cos(FMath::DegreesToRadians(GetActorRotation().Yaw));
	const float EntryY = GetActorLocation().Y + -FVector2D::Distance(FVector2D(EntryStartLocation), FVector2D(GetActorLocation())) * FMath::Sin(FMath::DegreesToRadians(GetActorRotation().Yaw));
	
	const float ExitX = GetActorLocation().X + FVector2D::Distance(FVector2D(ExitStartLocation), FVector2D(GetActorLocation())) * FMath::Cos(FMath::DegreesToRadians(GetActorRotation().Yaw));
	const float ExitY = GetActorLocation().Y + FVector2D::Distance(FVector2D(ExitStartLocation), FVector2D(GetActorLocation())) * FMath::Sin(FMath::DegreesToRadians(GetActorRotation().Yaw));
	
	EntryStartLocation = FVector(EntryX, EntryY, EntryStartLocation.Z);
	ExitStartLocation = FVector(ExitX, ExitY, ExitStartLocation.Z);

	FRotator EntryLookAt = UKismetMathLibrary::FindLookAtRotation(EntryStartLocation, GetActorLocation());
	EntryLookAt.Roll = 0.0f;
	EntryLookAt.Pitch = 0.0f;
	
	FRotator ExitLookAt = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ExitStartLocation);
	ExitLookAt.Roll = 0.0f;
	ExitLookAt.Pitch = 0.0f;

	const FQuat EntryBoxRotation = EntryLookAt.Quaternion();
	const FQuat ExitBoxRotation = ExitLookAt.Quaternion();
	
	DrawDebugBox(GetWorld(), EntryStartLocation, EntryTriggerBoxExtent, EntryBoxRotation, FColor::Red);
	DrawDebugBox(GetWorld(), ExitStartLocation, ExitTriggerBoxExtent, ExitBoxRotation, FColor::Red);

	DrawDebugLine(GetWorld(), EntryStartLocation, ExitStartLocation, FColor::White);

	DrawDebugDirectionalArrow(GetWorld(), EntryStartLocation, EntryStartLocation + EntryBoxRotation.Rotator().Vector() * 50.0f, 100.0f, FColor::Yellow, false, 0.033f, 0, 1.5f);
	DrawDebugDirectionalArrow(GetWorld(), ExitStartLocation, ExitStartLocation + ExitBoxRotation.Rotator().Vector() * 50.0f, 100.0f, FColor::Yellow, false, 0.033f, 0, 1.5f);
}
#endif

FTransform AWallHoleTraversal::CalculateEntryTransform() const
{
	FVector EntryStartLocation = GetActorLocation() + EntryTriggerBoxTransform.GetLocation();

	const float EntryX = GetActorLocation().X + -FVector2D::Distance(FVector2D(EntryStartLocation), FVector2D(GetActorLocation())) * FMath::Cos(FMath::DegreesToRadians(GetActorRotation().Yaw));
	const float EntryY = GetActorLocation().Y + -FVector2D::Distance(FVector2D(EntryStartLocation), FVector2D(GetActorLocation())) * FMath::Sin(FMath::DegreesToRadians(GetActorRotation().Yaw));
	
	EntryStartLocation = FVector(EntryX, EntryY, EntryStartLocation.Z);

	FRotator EntryLookAt = UKismetMathLibrary::FindLookAtRotation(EntryStartLocation, GetActorLocation());
	EntryLookAt.Roll = 0.0f;
	EntryLookAt.Pitch = 0.0f;
	
	return FTransform(EntryLookAt, EntryStartLocation);
}

FTransform AWallHoleTraversal::CalculateExitTransform() const
{
	FVector ExitStartLocation = GetActorLocation() + ExitTriggerBoxTransform.GetLocation();

	const float ExitX = GetActorLocation().X + FVector2D::Distance(FVector2D(ExitStartLocation), FVector2D(GetActorLocation())) * FMath::Cos(FMath::DegreesToRadians(GetActorRotation().Yaw));
	const float ExitY = GetActorLocation().Y + FVector2D::Distance(FVector2D(ExitStartLocation), FVector2D(GetActorLocation())) * FMath::Sin(FMath::DegreesToRadians(GetActorRotation().Yaw));
	
	ExitStartLocation = FVector(ExitX, ExitY, ExitStartLocation.Z);

	FRotator ExitLookAt = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ExitStartLocation);
	ExitLookAt.Roll = 0.0f;
	ExitLookAt.Pitch = 0.0f;

	return FTransform(ExitLookAt, ExitStartLocation);
}

void AWallHoleTraversal::TestForMeshes()
{
	EntryTransform = CalculateEntryTransform();
	ExitTransform = CalculateExitTransform();
	
	// Auto find mesh actors
	TArray<FHitResult> HitResults;
	UKismetSystemLibrary::BoxTraceMulti(this, EntryTransform.GetLocation() + FVector(0.0f, 0.0f, 25.0f), ExitTransform.GetLocation() + FVector(0.0f, 0.0f, 25.0f), FVector(50.0f), GetActorRotation(), UEngineTypes::ConvertToTraceType(ECC_WorldStatic), true, {}, EDrawDebugTrace::ForDuration, HitResults, true);
	for (FHitResult& Hit : HitResults)
	{
		if (const AStaticMeshActor* Mesh = Cast<AStaticMeshActor>(Hit.GetActor()))
			IgnoredMeshActors.AddUnique(Mesh);
	}
}

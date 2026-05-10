// Copyright Void Interactive, 2023

#include "Actors/InterestOverrideZone.h"

#include "Characters/CyberneticController.h"

#include "Door.h"

AInterestOverrideZone::AInterestOverrideZone()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.1f;

	OnActorBeginOverlap.AddDynamic(this, &AInterestOverrideZone::OnBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &AInterestOverrideZone::OnBeginOverlap);

	SetCanBeDamaged(false);
	bFindCameraComponentWhenViewTarget = false;
	
	SetActorEnableCollision(true);
	
	bGenerateOverlapEventsDuringLevelStreaming = true;

	GetBrushComponent()->SetCollisionResponseToChannel(ECC_COVER, ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_OCCLUSION, ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_SOUND, ECR_Ignore);
	GetBrushComponent()->SetCollisionObjectType(ECC_VOLUME);

	#if WITH_EDITORONLY_DATA
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard Component"));
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->SetMobility(EComponentMobility::Static);
	BillboardComponent->SetRelativeLocation(FVector::ZeroVector);
	BillboardComponent->SetRelativeScale3D(FVector(0.8f));
	BillboardComponent->SetupAttachment(GetRootComponent());
	BillboardComponent->SetCachedMaxDrawDistance(1000.0f);
	
	static ConstructorHelpers::FObjectFinder<UTexture2D> ViewIcon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/T_View.T_View'"));
	BillboardComponent->SetSprite(ViewIcon.Object);
	#endif
}

void AInterestOverrideZone::OnBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(OtherActor))
	{
		if (!AI->IsActive() && !AI->IsOnSWATTeam())
		{
			OnEndOverlap(OverlappedActor, OtherActor);
			return;
		}
		
		OnAIOverlap(AI);
	}
}

void AInterestOverrideZone::OnEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (ACyberneticCharacter* AI = Cast<ACyberneticCharacter>(OtherActor))
	{
		CharactersInZone.Remove(AI);
		
		if (const ACyberneticController* Controller = AI->GetCyberneticsController())
		{
			Controller->GetTargetingComp()->CurrentInterestZone = nullptr;
			
			FocusIndexMap.Remove(AI);
			MoveIndexMap.Remove(AI);
		}
	}
}

void AInterestOverrideZone::GiveMove(ACyberneticCharacter* AI)
{
	if (AI->IsOnSWATTeam())
		return;
	
	bool bAnotherAIHasOccupiedCurrentMoveSlot = false;
	for (const ACyberneticCharacter* C : CharactersInZone)
	{
		if (C != AI)
		{
			if (const uint8* Index1 = MoveIndexMap.Find(C))
			{
				if (const uint8* Index2 = MoveIndexMap.Find(AI))
				{
					if (*Index1 == *Index2)
					{
						bAnotherAIHasOccupiedCurrentMoveSlot = true;
						break;
					}
				}
			}
		}
	}

	if (bAnotherAIHasOccupiedCurrentMoveSlot)
	{
		return;
	}
	
	if (const uint8* Index = MoveIndexMap.Find(AI))
	{
		if (StationPoints.IsValidIndex(*Index))
		{
			const FInterestStationPoint& Point = StationPoints[*Index];
			
			const FVector MoveLocation = GetActorTransform().TransformPosition(Point.Location);

			if (AI->GetCyberneticsController())
				AI->GetCyberneticsController()->GiveMoveTo(MoveLocation);
		}
	}
}

bool AInterestOverrideZone::GetCurrentInterestInfo(ACyberneticCharacter* AI, FVector& OutLocation, AActor*& OutActor) const
{
	if (const uint8* Index = FocusIndexMap.Find(AI))
	{
		if (InterestPoints.IsValidIndex(*Index))
		{
			const FInterestPoint& Point = InterestPoints[*Index];

			OutActor = nullptr;

			switch (Point.Type)
			{
				case EInterestPointType::Manual:			OutLocation = GetActorTransform().TransformPosition(Point.Location); break;
				case EInterestPointType::Threat:			OutLocation = Point.Threat ? Point.Threat->GetActorLocation() : FVector::ZeroVector; OutActor = Point.Threat.Get(); break;
				case EInterestPointType::Door:				OutLocation = Point.Door ? Point.Door->GetDoorMidLocation() : FVector::ZeroVector; OutActor = Point.Door.Get(); break;
				case EInterestPointType::Spawner:			OutLocation = Point.Spawner ? Point.Spawner->GetActorLocation() : FVector::ZeroVector; OutActor = Point.Spawner.Get(); break;
				case EInterestPointType::CustomActor:		OutLocation = Point.CustomActor ? Point.CustomActor->GetActorLocation() : FVector::ZeroVector; OutActor = Point.CustomActor.Get(); break;
				default:									OutLocation = FVector::ZeroVector; break;
			}

			return true;
		}
	}

	return false;
}

void AInterestOverrideZone::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->OnWorldBeginPlay.AddUObject(this, &AInterestOverrideZone::OnWorldBeginPlay);
}

void AInterestOverrideZone::OnWorldBeginPlay()
{
	const TArray<ACyberneticCharacter*>& AllAI = GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters;

	// handle the case where the AI is spawned inside this volume where OnActorBeginOverlap doesnt handle this case
	for (ACyberneticCharacter* AI : AllAI)
	{
		if (AI->IsOnSWATTeam())
			continue;
		
		FVector StartLocation = AI->GetActorLocation();
		FVector EndLocation = GetBrushComponent()->GetComponentLocation();

		const float ZHeightDifference = FMath::Abs(StartLocation.Z - EndLocation.Z);
		
		if (ZHeightDifference < 150.0f)
		{
			if (FVector::Distance(StartLocation, EndLocation) < 500.0f)
			{
				//DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::White, false, 2.0f);
				if (!GetWorld()->LineTraceTestByChannel(StartLocation, EndLocation, ECC_Visibility))
				{
					OnAIOverlap(AI);
				}
			}
		}
	}
}

void AInterestOverrideZone::OnAIOverlap(ACyberneticCharacter* AI)
{
	if (AI->IsActive() && !AI->IsOnSWATTeam())
	{
		CharactersInZone.AddUnique(AI);

		if (const ACyberneticController* Controller = AI->GetCyberneticsController())
		{
			Controller->GetTargetingComp()->CurrentInterestZone = this;

			FocusIndexMap.Add(AI, 0);
			MoveIndexMap.Add(AI, 0);

			GiveMove(AI);
		}
	}
}

void AInterestOverrideZone::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	#if WITH_EDITOR
	GetBrushComponent()->bHiddenInGame = !bDrawInGame;
	
	if (IsSelectedInEditor())
	{
		for (FInterestPoint& P : InterestPoints)
		{
			DrawDebugLine(GetWorld(), GetActorLocation(), GetActorTransform().TransformPosition(P.Location), FColor::White, false, DeltaSeconds + 0.025f, 0, 1.5f);
		}
	}
	#endif

	CharactersInZone.RemoveAll([](ACyberneticCharacter* AI)
	{
		return !AI->IsActive();
	});
	
	for (ACyberneticCharacter* AI : CharactersInZone)
	{
		if (AI->IsOnSWATTeam() ||
			AI->GetCyberneticsController()->GetTrackedTarget() || AI->GetCyberneticsController()->BestAction != nullptr ||
			AI->GetCyberneticsController()->BestCombatMoveAction != nullptr)
		{
			FocusIndexMap.Remove(AI);
			MoveIndexMap.Remove(AI);
			continue;
		}
		
		if (uint8* Index = &FocusIndexMap.FindOrAdd(AI))
		{
			if (InterestPoints.IsValidIndex(*Index))
			{
				FInterestPoint& Point = InterestPoints[*Index];
				
				const float Time = Point.TimeFocusing.FindOrAdd(AI) += DeltaSeconds;

				if (Time > Point.RequiredTimeFocusing)
				{
					(*Index)++;
					
					Point.TimeFocusing.FindOrAdd(AI) = 0.0f;
				}
			}
			else
			{
				*Index = 0;
			}
		}

		if (uint8* Index = &MoveIndexMap.FindOrAdd(AI))
		{
			if (StationPoints.IsValidIndex(*Index))
			{
				FInterestStationPoint& Point = StationPoints[*Index];
				
				const float Time = Point.TimeStationing.FindOrAdd(AI) += DeltaSeconds;

				if (Time > Point.RequiredTimeStationing)
				{
					(*Index)++;
					
					Point.TimeStationing.FindOrAdd(AI) = 0.0f;

					GiveMove(AI);
				}
			}
			else
			{
				*Index = 0;
				
				GiveMove(AI);
			}
		}
	}
}

#if WITH_EDITOR
void AInterestOverrideZone::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	for (FInterestPoint& P : InterestPoints)
	{
		if (P.Type == EInterestPointType::Manual)
		{
			P.Threat = nullptr;
			P.Door = nullptr;
			P.Spawner = nullptr;
			P.CustomActor = nullptr;
		}
		else if (P.Type == EInterestPointType::Door)
		{
			P.Threat = nullptr;
			P.Spawner = nullptr;
			P.CustomActor = nullptr;

			if (P.Door)
			{
				P.Location = GetActorTransform().InverseTransformPosition(P.Door->GetDoorMidLocation());
			}
		}
		else if (P.Type == EInterestPointType::Threat)
		{
			P.Door = nullptr;
			P.Spawner = nullptr;
			P.CustomActor = nullptr;
			
			if (P.Threat)
			{
				P.Location = GetActorTransform().InverseTransformPosition(P.Threat->GetActorLocation());
			}
		}
		else if (P.Type == EInterestPointType::Spawner)
		{
			P.Door = nullptr;
			P.Threat = nullptr;
			P.CustomActor = nullptr;
			
			if (P.Spawner)
			{
				P.Location = GetActorTransform().InverseTransformPosition(P.Spawner->GetActorLocation());
			}
		}
		else if (P.Type == EInterestPointType::CustomActor)
		{
			P.Door = nullptr;
			P.Threat = nullptr;
			P.Spawner = nullptr;
			
			if (P.CustomActor)
			{
				P.Location = GetActorTransform().InverseTransformPosition(P.CustomActor->GetActorLocation());
			}
		}
	}
}
#endif

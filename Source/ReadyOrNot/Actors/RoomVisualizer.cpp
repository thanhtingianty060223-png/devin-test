// Copyright Void Interactive, 2023

#include "Actors/RoomVisualizer.h"

#include "Door.h"
#include "ThreatAwarenessActor.h"
#include "WorldDataGenerator.h"

ARoomVisualizer::ARoomVisualizer()
{
	bIsEditorOnlyActor = true;
	
	#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.1f;
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
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->SetMobility(EComponentMobility::Static);
	BillboardComponent->SetRelativeLocation(FVector::ZeroVector);
	BillboardComponent->SetRelativeScale3D(FVector(0.8f));
	BillboardComponent->SetupAttachment(DefaultScene);
	BillboardComponent->SetCachedMaxDrawDistance(1000.0f);
	
	static ConstructorHelpers::FObjectFinder<UTexture2D> SmallRoomIcon_(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/RT_SMALLROOM.RT_SMALLROOM'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> MediumRoomIcon_(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/RT_MEDIUMROOM.RT_MEDIUMROOM'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> LargeRoomIcon_(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/RT_HUGEROOM.RT_HUGEROOM'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> CorridorRoomIcon_(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/RT_CORRIDOR.RT_CORRIDOR'"));
	SmallRoomIcon = SmallRoomIcon_.Object;
	MediumRoomIcon = MediumRoomIcon_.Object;
	LargeRoomIcon = LargeRoomIcon_.Object;
	CorridorRoomIcon = CorridorRoomIcon_.Object;
	#endif
}

void ARoomVisualizer::SetRoomSize(const ERoomSize& InSize)
{
	Size = InSize;

	#if WITH_EDITORONLY_DATA
	switch (InSize)
	{
		case ERoomSize::Small:		BillboardComponent->SetSprite(SmallRoomIcon); break;
		case ERoomSize::Medium:		BillboardComponent->SetSprite(MediumRoomIcon); break;
		case ERoomSize::Large:		BillboardComponent->SetSprite(LargeRoomIcon); break;
		case ERoomSize::Corridor:	BillboardComponent->SetSprite(CorridorRoomIcon); break;
		default:					BillboardComponent->SetSprite(SmallRoomIcon); break;
	}
	#endif
}

void ARoomVisualizer::BeginPlay()
{
	Super::BeginPlay();

	Destroy();
}

void ARoomVisualizer::Tick(float DeltaTime)
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
	PrimaryActorTick.UnRegisterTickFunction();
}

#if WITH_EDITOR
bool ARoomVisualizer::ShouldTickIfViewportsOnly() const
{
	return true;
}

void ARoomVisualizer::EditorTick(const float DeltaTime)
{
	if (!IsSelectedInEditor())
		return;

	if (!AWorldDataGenerator::Get(GetWorld()))
		return;
	
	FRoom* RoomPtr = AWorldDataGenerator::Get(GetWorld())->RoomData.Rooms.FindByPredicate([&](const FRoom& Room)
	{
		return Room.Name == OwningRoom;
	});
	
	if (RoomPtr)
	{
		Threats = RoomPtr->Threats;
		Doors = RoomPtr->AdditionalRootDoors;

		for (const FName& N : RoomPtr->ConnectingRooms)
		{
			for (TActorIterator<ARoomVisualizer> It(GetWorld()); It; ++It)
			{
				if (It->OwningRoom == N)
				{
					ConnectingRooms.AddUnique(*It);
					break;
				}
			}
		}
		
        DrawDebugBox(GetWorld(), FVector(RoomPtr->Location), FVector(15.0f), FColor::Yellow, false, DeltaTime+0.1f);
		
        for (const ADoor* Door : RoomPtr->AdditionalRootDoors)
        {
            DrawDebugLine(GetWorld(), GetActorLocation(), Door->GetDoorMidLocation(), FColor::Cyan, false, DeltaTime+0.1f);
        }

		for (AThreatAwarenessActor* TAA : RoomPtr->Threats)
		{
			FColor Color = FColor::Cyan;
			if (TAA->GetThreatLevel() == EThreatLevel::TL_Medium)
				Color = FColor::Turquoise;
			else if (TAA->GetThreatLevel() == EThreatLevel::TL_High)
				Color = FColor::Orange;
			else if (TAA->GetThreatLevel() >= EThreatLevel::TL_Extreme)
				Color = FColor::Red;
			
			DrawDebugBox(GetWorld(), TAA->GetActorLocation(), FVector(15.0f), Color, false, DeltaTime+0.1f);
		}
	}
}
#endif

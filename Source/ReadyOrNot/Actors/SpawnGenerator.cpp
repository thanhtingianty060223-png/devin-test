// Void Interactive, 2020

#include "SpawnGenerator.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#include "Editor.h"
#endif

ASpawnGenerator::ASpawnGenerator()
{
#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
#else
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
#endif

	bFindCameraComponentWhenViewTarget = false;
	bIsEditorOnlyActor = false;
	bAlwaysRelevant = true;
	
	SetCanBeDamaged(false);

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));
	SetRootComponent(SceneComponent);
	
	#if WITH_EDITOR
	static ConstructorHelpers::FObjectFinder<UTexture2D> AI_SpawnPoint(TEXT("Texture2D'/Engine/EditorResources/Ai_Spawnpoint.Ai_Spawnpoint'"));

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard Component"));
	BillboardComponent->SetSprite(AI_SpawnPoint.Object);
	BillboardComponent->SetupAttachment(RootComponent);
	#endif
}

void ASpawnGenerator::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	#if WITH_EDITOR
	if (bShowNodes)
	{
		for (const FVector& NodeLocation : Nodes)
		{
			DrawDebugSphere(GetWorld(), NodeLocation, 50.0f, 12, FColor::Green, false, -1.0f, 0, 1.0f);
		}
	}
	#endif
}

void ASpawnGenerator::RefreshSpawns()
{
	DestroyAllPlayerStartActors();

	ClearNodes();
	CreateNodes();

	SpawnPlayerStartActors();

	UpdatePlayerStartLocations();
	UpdatePlayerStartTags();
}

#if WITH_EDITOR
void ASpawnGenerator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == "Rows" || PropertyChangedEvent.GetPropertyName() == "Columns")
	{
		ClearNodes();
		CreateNodes();
		
		SpawnPlayerStartActors();
	}
	else if (PropertyChangedEvent.GetPropertyName() == "RowSpacing" || PropertyChangedEvent.GetPropertyName() == "ColumnSpacing")
	{
		ClearNodes();
		CreateNodes();
	}
	else if (PropertyChangedEvent.GetPropertyName() == "PlayerStartTags")
	{
		UpdatePlayerStartTags();
	}

	UpdatePlayerStartLocations();
}

void ASpawnGenerator::PostEditUndo()
{
	Super::PostEditUndo();

	DestroyAllPlayerStartActors();
	
	RefreshSpawns();
}

void ASpawnGenerator::K2_DestroyActor()
{
	DestroyAllPlayerStartActors();

	Super::K2_DestroyActor();
}

bool ASpawnGenerator::ShouldTickIfViewportsOnly() const
{
	return true;
}
#endif

void ASpawnGenerator::CreateNodes()
{
	float NewSpacingX = 0.0f;
	float NewSpacingY = 0.0f;

	for (int32 y = 0; y < Columns; ++y)
	{
		for (int32 x = 0; x < Rows; ++x)
		{
			FVector NodeLocation = GetActorLocation() - FVector(NewSpacingX, NewSpacingY, 0.0f);
			NodeLocation.Z = GetActorLocation().Z;
			
			Nodes.Add(NodeLocation);

			NewSpacingX += RowSpacing;
		}

		NewSpacingY += ColumnSpacing;
		NewSpacingX = 0.0f;
	}
}

void ASpawnGenerator::ClearNodes()
{
	Nodes.Empty(Columns * Rows);
}

void ASpawnGenerator::SpawnPlayerStartActors()
{
	if (Nodes.Num() == 0)
		return;

	DestroyAllPlayerStartActors();
	
	for	(const FVector& Node : Nodes)
	{
		if (APlayerStart* PlayerStart = GetWorld()->SpawnActor<APlayerStart>(Node, GetActorRotation()))
		{
			PlayerStart->PlayerStartTag = "Default";

			PlayerStarts.Add(PlayerStart);
			PlayerStarts_NonUproperty.Add(PlayerStart);
		}
	}
}

void ASpawnGenerator::SelectAll()
{
	#if WITH_EDITOR
	GEditor->SelectNone(false, true, true);

	for (APlayerStart* PlayerStart : PlayerStarts)
	{
		GEditor->SelectActor(PlayerStart, true, true);
	}
	#endif
}

void ASpawnGenerator::UpdatePlayerStartLocations()
{
	if (Nodes.Num() == 0)
		return;

	PlayerStarts.RemoveAll([](APlayerStart* Actor)
	{
		return Actor == nullptr;
	});
	
	int32 i = 0;
	for (APlayerStart* PlayerStart : PlayerStarts)
	{
		PlayerStart->SetActorLocation(Nodes[i]);
		i++;
	}
}

void ASpawnGenerator::UpdatePlayerStartTags()
{
	//if (PlayerStartTags.Num() == 0 || PlayerStartTags.Num() > PlayerStarts.Num())
	//	return;

	if (SpawnTeam == ETeamType::TT_NONE)
		return;
	
	PlayerStarts.Remove(nullptr);

	for (APlayerStart* PlayerStart : PlayerStarts)
	{
		if (PlayerStart)
			PlayerStart->PlayerStartTag = (SpawnTeam == ETeamType::TT_SERT_BLUE ? "SERT_BLUE" : "SERT_RED");
	}

	//const int32 Chunks = PlayerStartTags.Num();
	//const int32 PlayerStartsPerChunk = PlayerStarts.Num()/Chunks;
	//const int32 LeftOverPlayerStarts = PlayerStarts.Num()%PlayerStartsPerChunk;
	//
	//for (int32 i = 0; i < Chunks; i++)
	//{
	//	for (int32 j = i*PlayerStartsPerChunk; j < PlayerStartsPerChunk*(i+1); j++)
	//	{
	//		PlayerStarts[j]->PlayerStartTag = PlayerStartTags[i];
	//	}
	//}
	//
	//for (int32 i = PlayerStarts.Num()-LeftOverPlayerStarts; i < PlayerStarts.Num(); i++)
	//{
	//	PlayerStarts[i]->PlayerStartTag = PlayerStartTags[Chunks-1];
	//}
}

void ASpawnGenerator::DestroyAllPlayerStartActors()
{
	PlayerStarts_NonUproperty.Remove(nullptr);
	PlayerStarts.Remove(nullptr);

	for	(APlayerStart* Actor : PlayerStarts_NonUproperty)
	{
		Actor->Destroy();
	}
	
	for	(APlayerStart* Actor : PlayerStarts)
	{
		Actor->Destroy();
	}

	PlayerStarts.Empty(Columns * Rows);
	PlayerStarts_NonUproperty.Empty(Columns * Rows);
}

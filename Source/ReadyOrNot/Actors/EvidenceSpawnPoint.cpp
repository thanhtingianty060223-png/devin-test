// Void Interactive, 2020

#include "EvidenceSpawnPoint.h"

#include "GameModes/IncriminationGM.h"
#include "GameModes/IncriminationGS.h"

AEvidenceSpawnPoint::AEvidenceSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bFindCameraComponentWhenViewTarget = false;

	bReplicates = true;
	SetCanBeDamaged(false);
	bAlwaysRelevant = true;

	#if WITH_EDITOR
	static ConstructorHelpers::FObjectFinder<UTexture2D> S_WindDirectional(TEXT("Texture2D'/Game/ReadyOrNot/UI/HUD_Revised/Matt/BaggingEvidence_Frame3.BaggingEvidence_Frame3'"));
	BillboardComponent->SetSprite(S_WindDirectional.Object);
	#endif

	bHasVisited = false;
}

void AEvidenceSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	GAMEMODE_CHECK(AIncriminationGM, AIncriminationGS)
}

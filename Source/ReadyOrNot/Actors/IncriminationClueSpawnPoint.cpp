// Void Interactive, 2020

#include "IncriminationClueSpawnPoint.h"

#include "GameModes/IncriminationGM.h"
#include "GameModes/IncriminationGS.h"

#include "Actors/Gameplay/IncriminationClue.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

AIncriminationClueSpawnPoint::AIncriminationClueSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.033f;

	bFindCameraComponentWhenViewTarget = false;

	bReplicates = true;
	SetCanBeDamaged(false);
	bAlwaysRelevant = true;

	#if WITH_EDITOR
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_MagnifyingGlass(TEXT("Texture2D'/Game/ReadyOrNot/UI/PlanningV4/Icon_MagnifyingGlass.Icon_MagnifyingGlass'"));
	BillboardComponent->SetSprite(T_MagnifyingGlass.Object);
	#endif

	static ConstructorHelpers::FClassFinder<AActor> BP_ClueFlare(TEXT("Blueprint'/Game/Blueprints/Actors/BP_ClueFlare.BP_ClueFlare_C'"));
	ClueFlareClass = BP_ClueFlare.Class;

	bHasVisited = false;
	bClueTimerStarted = false;
}

void AIncriminationClueSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	GAMEMODE_CHECK(AIncriminationGM, AIncriminationGS)
}

void AIncriminationClueSpawnPoint::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetWorld() && GetWorld()->bIsTearingDown)
		return;

	if (!bClueTimerStarted)
	{
		if (AIncriminationGS* IncrimGS = GetWorld()->GetGameState<AIncriminationGS>())
		{
			if (IncrimGS->MatchState == EMatchState::MS_Playing)
			{
				if (ClueNumber-1 <= 0)
				{
					StartClueTimer();
					return;
				}

				bool bSuccess;
				if (AIncriminationClue* Clue = IncrimGS->GetClue(ClueNumber-1, bSuccess))
				{
					if (Clue->IsClueFound())
					{
						StartClueTimer();
					}
				}
			}
		}
	}
}

void AIncriminationClueSpawnPoint::StartClueTimer()
{
	if (ShowObjectiveMarkerIn <= -1.0f)
		return;

	if (ShowObjectiveMarkerIn <= 0.0f)
	{
		OnClueTimerExpired();
	}
	else
	{
		UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_ClueTimerExpiry, this, &AIncriminationClueSpawnPoint::OnClueTimerExpired, ShowObjectiveMarkerIn);
		bClueTimerStarted = true;

		//ULog::Info("Clue " + FString::FromInt(ClueNumber) + "  timer started");
	}
}

void AIncriminationClueSpawnPoint::StopClueTimer()
{
	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, TH_ClueTimerExpiry);
	bClueTimerStarted = false;
}

void AIncriminationClueSpawnPoint::OnClueTimerExpired()
{
	SpawnClueFlare();
}

void AIncriminationClueSpawnPoint::SpawnClueFlare()
{
	if (GetWorld())
	{
		GetWorld()->SpawnActor<AActor>(ClueFlareClass, GetActorLocation(), GetActorRotation()); // Auto destroys itself
	}
}

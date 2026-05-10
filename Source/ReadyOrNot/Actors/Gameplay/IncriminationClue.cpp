// Void Interactive, 2020

#include "IncriminationClue.h"

#include "GameModes/IncriminationGM.h"
#include "GameModes/IncriminationGS.h"

#include "Components/ObjectiveMarkerComponent.h"
#include "Components/MapActorComponent.h"
#include "Components/InteractableComponent.h"

#include "Actors/IncriminationClueSpawnPoint.h"

#include "lib/ReadyOrNotFunctionLibrary.h"

AIncriminationClue::AIncriminationClue()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = true;
	bFindCameraComponentWhenViewTarget = false;

	SetCanBeDamaged(false);
	bReplicates = true;
	bAlwaysRelevant = true;

	MapActorComponent = CreateDefaultSubobject<UMapActorComponent>(TEXT("Map Actor Component"));
	
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromString("Reveal Clue"));
	InteractableComponent->SetAnimatedIconName("Reveal Clue");

	ObjectiveMarkerComponent->bStartHidden = true;
}

void AIncriminationClue::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AIncriminationClue, ClueNumber);
	DOREPLIFETIME(AIncriminationClue, ShowObjectiveMarkerIn);
	DOREPLIFETIME(AIncriminationClue, ClueName);
	DOREPLIFETIME(AIncriminationClue, ClueFoundMessage);
	DOREPLIFETIME(AIncriminationClue, NextClue);
	DOREPLIFETIME(AIncriminationClue, SpawnPointOwner);
	DOREPLIFETIME(AIncriminationClue, bClueFound);
	DOREPLIFETIME(AIncriminationClue, bClueTimerExpired);
	DOREPLIFETIME(AIncriminationClue, ClueState);
}

void AIncriminationClue::Init(class AIncriminationClueSpawnPoint* OwningSpawn, const uint8 InClueNumber, const FText& InClueName, const FText& InClueFoundMessage, const float InShowObjectiveMarkerIn)
{
	SpawnPointOwner = OwningSpawn;
	ClueNumber = InClueNumber;
	ClueName = InClueName;
	ClueFoundMessage = InClueFoundMessage;
	ShowObjectiveMarkerIn = InShowObjectiveMarkerIn;
	
	bClueFound = false;
	bClueTimerExpired = false;

	MapActorComponent->SetIconText(ClueName);
	
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromString("Reveal " + ClueName.ToString()));
	InteractableComponent->SetAnimatedIconName("Reveal Clue");
}

void AIncriminationClue::BeginPlay()
{
	Super::BeginPlay();

	GAMEMODE_CHECK(AIncriminationGM, AIncriminationGS)

	MapActorComponent->DisableMapActor();
	
	ClueState = EClueState::Unclaimed;
}

void AIncriminationClue::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	InteractableComponent->ActionSlot1.ActionText = FText::FromString("Reveal " + ClueName.ToString());
	InteractableComponent->ActionSlot1.bCondition = !IsHidden();
	
	if (AIncriminationGS* IncrimGS = GetWorld()->GetGameState<AIncriminationGS>())
	{
		if (IncrimGS->IntelState == EEvidenceActorState::Unclaimed)
		{
			if (IncrimGS->ActiveClue == this && NextClue && IsHidden() && ClueState == EClueState::Unclaimed)
			{
				ShowClue();
			}
		}
		else
		{
			HideClue();
			return;
		}
	}

	if (PickupInstigator)
	{
		if (ClueState == EClueState::Collected)
		{
			SetActorLocation(PickupInstigator->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
			HideClue();
		}
		else
		{
			ShowClue(false);
		}
	}
}

void AIncriminationClue::ActorPickedUp(AActor* InPickupInstigator)
{
	Super::ActorPickedUp(InPickupInstigator);

	if (!IsHidden())
	{
		if (AIncriminationGS* IncrimGS = GetWorld()->GetGameState<AIncriminationGS>())
		{
			if (IncrimGS->IntelState == EEvidenceActorState::Unclaimed)
			{
				OnClueFound();
			}
		}
	}
}

void AIncriminationClue::ActorDropped(AActor* InDroppedInstigator)
{
	Super::ActorDropped(InDroppedInstigator);

	ClueState = EClueState::Dropped;

	if (PickupInstigator)
	{
		const float X = PickupInstigator->GetActorLocation().X + 200.0f * FMath::Cos(FMath::RandRange(0.0f, 2.0f*PI));
		const float Y = PickupInstigator->GetActorLocation().Y + 200.0f * FMath::Sin(FMath::RandRange(0.0f, 2.0f*PI));

		FHitResult SweepHitResult;
		SetActorLocation(FVector(X, Y, PickupInstigator->GetActorLocation().Z), true, &SweepHitResult, ETeleportType::TeleportPhysics);

		if (SweepHitResult.bBlockingHit)
		{
			SetActorLocation(FVector(SweepHitResult.Location.X, SweepHitResult.Location.Y, PickupInstigator->GetActorLocation().Z), false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	if (GetLocalRole() >= ROLE_Authority)
	{
		OnRep_OnClueStateChanged();
	}
}

void AIncriminationClue::RevealNextClue()
{
	HideClue();

	if (AIncriminationGS* IncrimGS = GetWorld()->GetGameState<AIncriminationGS>())
	{
		if (IncrimGS->IntelState == EEvidenceActorState::Unclaimed)
		{
			if (NextClue && NextClue != this)
			{
				if (NextClue->IsClueFound())
				{
					NextClue->RevealNextClue();
				}
				else
				{
					NextClue->ShowClue();
				}
			}
		}
	}
}

void AIncriminationClue::ShowClue(const bool bStartCountdown)
{
	SetActorHiddenInGame(false);

	InteractableComponent->EnableInteractable();

	bClueFound = false;
	bClueTimerExpired = false;

	if (bStartCountdown)
	{
		if (ShowObjectiveMarkerIn <= -1.0f)
			return;

		if (ShowObjectiveMarkerIn <= 0.0f)
		{
			OnClueTimerExpired();
		}
		else
		{
			UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_ClueTimerExpiry, this, &AIncriminationClue::OnClueTimerExpired, ShowObjectiveMarkerIn);
		}
	}
}

void AIncriminationClue::HideClue()
{
	SetActorHiddenInGame(true);

	HideObjectiveMarker();
	
	InteractableComponent->DisableInteractable();
	MapActorComponent->DisableMapActor();

	bClueTimerExpired = false;

	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, TH_ClueTimerExpiry);
}

void AIncriminationClue::SetNextClue(AIncriminationClue* NewNextClue)
{
	NextClue = NewNextClue;
}

void AIncriminationClue::SetClueNumber(const uint8 NewClueNumber)
{
	ClueNumber = FMath::Clamp(NewClueNumber, uint8(1), MAX_uint8);
}

void AIncriminationClue::OnClueFound_Implementation()
{
	MapActorComponent->DisableMapActor();
	
	ClueState = EClueState::Collected;
	bClueFound = true;
	bClueTimerExpired = false;

	ENQUEUE_INGAMELOG_MESSAGE_PVP(FInGameLogMessage_PVP(Cast<APlayerCharacter>(PickupInstigator), EPVPEvent::IncrimClueFound, ClueFoundMessage));

	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, TH_ClueTimerExpiry);

	Delegate_OnClueFound.Broadcast(this, PickupInstigator);

	RevealNextClue();
}

void AIncriminationClue::OnClueTimerExpired()
{
	//ShowObjectiveMarker();

	//MapActorComponent->EnableMapActor();

	bClueTimerExpired = true;

	UReadyOrNotFunctionLibrary::StopCallbackTimer(this, TH_ClueTimerExpiry);
}

void AIncriminationClue::OnRep_OnClueFound()
{
	ENQUEUE_INGAMELOG_MESSAGE_PVP(FInGameLogMessage_PVP(Cast<APlayerCharacter>(PickupInstigator), EPVPEvent::IncrimClueFound, ClueFoundMessage));
}

void AIncriminationClue::OnRep_OnClueStateChanged()
{
	if (AIncriminationGS* IncrimGS = GetWorld()->GetGameState<AIncriminationGS>())
	{
		switch (ClueState)
		{
			case EClueState::Unclaimed:
			if (IncrimGS->ActiveClue == this)
			{
				ShowClue(true);
			}
			break;

			case EClueState::Collected:
			HideClue();
			break;

			case EClueState::Dropped:
			ShowClue(false);
			// TODO: Report dropped to in-game log
			break;

			default:
			break;
		}
	}
}

bool AIncriminationClue::IsClueFound() const
{
	return bClueFound;
}

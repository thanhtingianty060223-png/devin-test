// Void Interactive, 2020

#include "PVPTriggerBox.h"

#include "Components/ObjectiveMarkerComponent.h"

#include "ReadyOrNotGameMode_PVP.h"

APVPTriggerBox::APVPTriggerBox()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	
	SetActorTickEnabled(true);

	bAlwaysRelevant = true;

	OnActorBeginOverlap.AddDynamic(this, &APVPTriggerBox::OnBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &APVPTriggerBox::OnEndOverlap);

	GetCollisionComponent()->SetCollisionProfileName("Trigger");
	GetCollisionComponent()->CanCharacterStepUpOn = ECB_No;

	SetRootComponent(GetCollisionComponent());

	ObjectiveMarkerComponent = CreateDefaultSubobject<UObjectiveMarkerComponent>(TEXT("Objective Marker Component"));
	ObjectiveMarkerComponent->SetRelativeLocation(FVector::ZeroVector);
	ObjectiveMarkerComponent->SetIsReplicated(true);
	
#if WITH_EDITOR
	TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Text Render"));
	TextRender->SetRelativeLocation({0.0f, 0.0f, 100.0f});
	TextRender->SetRelativeRotation({0.0f, -90.0f, 0.0f});
	TextRender->HorizontalAlignment = EHTA_Center;
	TextRender->VerticalAlignment = EVRTA_TextCenter;
	TextRender->SetWorldSize(80.0f);
	TextRender->Text = FText::FromString("PVP Trigger");
	TextRender->SetHiddenInGame(false);
	TextRender->SetupAttachment(RootComponent);

	AActor::SetActorHiddenInGame(false);
	if (GetCollisionComponent())
	{
		GetCollisionComponent()->SetHiddenInGame(true);
	}
	if (GetSpriteComponent())
	{
		GetSpriteComponent()->SetHiddenInGame(true);
	}
#endif
	
	bLogDebugInfo = false;

	OnlyAcceptActorsWithTags.Empty();

	OnlyAcceptTeams.Empty();
	OnlyAcceptTeams.Add(ETeamType::TT_SERT_BLUE);
	OnlyAcceptTeams.Add(ETeamType::TT_SERT_RED);
}

void APVPTriggerBox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APVPTriggerBox, TimeElapsed);
	DOREPLIFETIME(APVPTriggerBox, TimeNeededToStay_Active);
	DOREPLIFETIME(APVPTriggerBox, PreviousTimeElapsed);
	DOREPLIFETIME(APVPTriggerBox, bEntered);
	DOREPLIFETIME(APVPTriggerBox, CharactersInTriggerBox);
}

void APVPTriggerBox::BeginPlay()
{
	Super::BeginPlay();
	
	bool bIsPvPMode = false;
	if (AReadyOrNotGameState* RONGS = Cast<AReadyOrNotGameState>(UGameplayStatics::GetGameState(this)))
	{
		bIsPvPMode = RONGS->bPvPMode;
	}

	// This trigger box should only work in PVP gamemodes
	if (!bIsPvPMode)
	{
		Destroy();
		
		return;
	}

#if WITH_EDITOR
	TimeNeededToStay_Active = TimeNeededToStay_Editor;
#else
	TimeNeededToStay_Active = TimeNeededToStay;
#endif
}

void APVPTriggerBox::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (bEntered)
	{
		TimeElapsed += DeltaTime;

		#if WITH_EDITOR
		if (bLogDebugInfo)
			ULog::Time(TimeElapsed, DLTU_Seconds, false, CUR_CLASS_FUNC_2 + "Time elapsed: ");
		#endif
	}
}

void APVPTriggerBox::OnBeginOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!HasAuthority())
		return;

	if (!OtherActor)
		return;
	
	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
	if (IsPlayerOnAcceptedTeam(Player))
	{
		if (OtherActor != this)
		{
			if ((!GetWorldTimerManager().IsTimerActive(TH_TimerEventExpiry) && DoesActorHaveAnyAcceptedTags(Player)) || !GetWorldTimerManager().IsTimerActive(TH_TimerEventExpiry))
			{
				StartTimerEvent();

				bEntered = true;
			}

			CharactersInTriggerBox.Add(Player);
		
			#if WITH_EDITOR
			if (bLogDebugInfo)
				ULog::Info(Player->GetName() + " has entered " + GetName());
			#endif
		}
	}
}

void APVPTriggerBox::OnEndOverlap_Implementation(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!HasAuthority())
		return;
	
	if (!OtherActor)
		return;
	
	APlayerCharacter* Player = Cast<APlayerCharacter>(OtherActor);
	if (IsPlayerOnAcceptedTeam(Player))
	{
		if (OtherActor != this)
		{
			CharactersInTriggerBox.Remove(Player);
			
			if (CharactersInTriggerBox.Num() == 0 || DoesActorHaveAnyAcceptedTags(Player) || GetWorldTimerManager().IsTimerActive(TH_TimerEventExpiry))
			{
				bEntered = false;
				PreviousTimeElapsed = TimeElapsed;
				TimeElapsed = 0.0f;

				CancelTimerEvent();
			}
			
			#if WITH_EDITOR
			if (bLogDebugInfo)
				ULog::Info(Player->GetName() + " has exited " + GetName() + ". Time to trigger box: " + FString::SanitizeFloat(PreviousTimeElapsed) + " seconds");
			#endif
		}
	}
}

void APVPTriggerBox::OnTimerExpired_Implementation()
{
	#if WITH_EDITOR
	if (HasAuthority())
	{
		if (bLogDebugInfo)
			ULog::Info(GetName() + "'s timer has expired. Executing event...");
	}
	#endif
}

void APVPTriggerBox::OnRep_CharactersInTriggerBoxUpdated_Implementation()
{
}

void APVPTriggerBox::StartTimerEvent()
{
#if WITH_EDITOR
	if (TimeNeededToStay_Editor > 0)
	{
		GetWorldTimerManager().SetTimer(TH_TimerEventExpiry, this, &APVPTriggerBox::OnTimerExpired, TimeNeededToStay_Editor);
		TimeNeededToStay_Active = TimeNeededToStay_Editor;
	}
#else
	if (TimeNeededToStay > 0)
	{
		GetWorldTimerManager().SetTimer(TH_TimerEventExpiry, this, &APVPTriggerBox::OnTimerExpired, TimeNeededToStay);
		TimeNeededToStay_Active = TimeNeededToStay;
	}
#endif
}

void APVPTriggerBox::CancelTimerEvent()
{
	GetWorldTimerManager().ClearTimer(TH_TimerEventExpiry);
}

void APVPTriggerBox::PauseTimerEvent()
{
	GetWorldTimerManager().PauseTimer(TH_TimerEventExpiry);
}

void APVPTriggerBox::ResumeTimerEvent()
{
	GetWorldTimerManager().UnPauseTimer(TH_TimerEventExpiry);
}

bool APVPTriggerBox::DoesActorHaveAnyAcceptedTags(AActor* OtherActor) const
{
	if (!OtherActor)
		return false;
	
	for (const FName& Tag : OnlyAcceptActorsWithTags)
	{
		if (OtherActor->ActorHasTag(Tag))
		{
			return true;
		}
	}

	return false;
}

bool APVPTriggerBox::IsPlayerOnAcceptedTeam(APlayerCharacter* Player) const
{
	if (!Player)
		return false;
	
	for (auto Team : OnlyAcceptTeams)
	{
		if (Player->GetTeam() == Team)
			return true;
	}

	return false;
}

bool APVPTriggerBox::IsActorInTriggerBox(AActor* InActor) const
{
	if (!InActor)
		return false;
	
	for (auto Character : CharactersInTriggerBox)
	{
		if (InActor == Character)
		{
			return true;
		}
	}

	return false;
}

void APVPTriggerBox::ToggleObjectiveMarker()
{
	if (ObjectiveMarkerComponent->IsVisible() || !ObjectiveMarkerComponent->bHiddenInGame)
	{
		HideObjectiveMarker();
	}
	else
	{
		ShowObjectiveMarker();
	}
}

void APVPTriggerBox::ShowObjectiveMarker()
{
	if (!ObjectiveMarkerComponent)
		return;

	ObjectiveMarkerComponent->ShowObjectiveMarker();
}

void APVPTriggerBox::HideObjectiveMarker()
{
	if (!ObjectiveMarkerComponent)
		return;

	ObjectiveMarkerComponent->HideObjectiveMarker();
}

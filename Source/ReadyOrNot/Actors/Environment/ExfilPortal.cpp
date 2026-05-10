// Copyright Void Interactive, 2023

#include "ExfilPortal.h"
#include "Components/InteractableComponent.h"
#include "Characters/AI/SWATCharacter.h"
#include "GameModes/CoopGM.h"
#include "Info/ScoringManager.h"
#include "Components/ScoringComponent.h"
#include "GameModes/CoopGS.h"
#include "Info/SWATManager.h"

AExfilPortal::AExfilPortal()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f;

	// Primary action slot for standalone or squad leader in networked
	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Interactable Component"));
	InteractableComponent->RequiredLookAtPercentage = 0.6f;
	InteractableComponent->ShowPromptAtDistance = 600;
	InteractableComponent->bDistanceFadeIcon = false;
	InteractableComponent->AnimatedIconName = "Peek Door";
	InteractableComponent->InteractIconSize = 200;
	InteractableComponent->bImprintIconOnHUDUponInteraction = true;
	InteractableComponent->bHideUponInteraction = true;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "ExfilWithNoMembers"));
	InteractableComponent->bClientInteract = true;

	// Blocked action slot for networked non squad leader
	InteractableComponent->ActionSlot2.bUseCustomActionText = true;
	InteractableComponent->ActionSlot2.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "ExfilNotSquadLeader");
	InteractableComponent->ActionSlot2.bCondition = false;

	InteractableComponent->bEnableActionPromptBackground = true;

	SetRootComponent(InteractableComponent);
	
	ScoringComponent = CreateDefaultSubobject<UScoringComponent>(TEXT("Scoring Component"));
	
	bReplicates = true;

	bIsDrawingOutline = false;

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Component"));
	CollisionComponent->InitBoxExtent(FVector(500,500,250));
	//CollisionComponent->CreateSceneProxy();
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionObjectType(ECC_WorldStatic);
	CollisionComponent->SetCollisionProfileName("Trigger", true);
	CollisionComponent->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel13);
	CollisionComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	CollisionComponent->SetCanEverAffectNavigation(false);
	CollisionComponent->SetupAttachment(GetRootComponent());

	bReplicates = true;
	bAlwaysRelevant = true;
}

// void AExfilPortal::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
// {
// 	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
// }

void AExfilPortal::BeginPlay()
{
	Super::BeginPlay();
	
	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Exfil", OutActors);
	for (AActor* a : OutActors)
	{
		TArray<UStaticMeshComponent*> OutStatics;
		a->GetComponents(OutStatics);
		CompsToOutline.Append(OutStatics);

	}
	
	if (AReadyOrNotGameMode* GM = GetWorld()->GetAuthGameMode<AReadyOrNotGameMode>())
	{
		InteractableComponent->ActionSlot1.bCondition = GM->GetIsExfilEnabled();
		GM->OnExfilEnabledChange.AddDynamic(this, &AExfilPortal::OnExfilEnabledChange);

		if (GM->GetIsExfilEnabled())
		{
			InteractableComponent->EnableInteractable();
		}
		else
		{
			InteractableComponent->DisableInteractable();
		}
	}
	
	OnActorBeginOverlap.AddDynamic(this, &AExfilPortal::ActorBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &AExfilPortal::ActorEndOverlap);
}

void AExfilPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (!InteractableComponent->bEnabled)
	{
		if (bIsDrawingOutline)
			DisableExfilOutline();
		return;
	}

	if (InteractableComponent->IsBeingLookedAt(UReadyOrNotStatics::GetReadyOrNotPlayerController(), InteractableComponent->ShowPromptAtDistance, InteractableComponent->RequiredLookAtPercentage))
	{
		if (!bIsDrawingOutline)
		{
			DrawExfilOutline();
			bIsDrawingOutline = true;
		}
	} else
	{
		if (bIsDrawingOutline)
		{
			DisableExfilOutline();
			bIsDrawingOutline = false;
		}
	}

	// Only squad leader can exfiltrate
	// TODO: Move to delegate called when squad leader is updated, once SwatManager rewrite is finished
	if (AReadyOrNotPlayerState* ps = UBpGameplayHelperLib::GetLocalPlayerState(GetWorld()))
	{
		InteractableComponent->ActionSlot1.bCondition = ps->IsSquadLeader();
		InteractableComponent->ActionSlot2.bCondition = !ps->IsSquadLeader();
	}

	if (HasAuthority())
	{
		USWATManager* sm = USWATManager::Get(GetWorld());
		SetOwner(sm->GetSquadLeader());
	}
	///////
}

void AExfilPortal::DrawExfilOutline()
{
	for (UStaticMeshComponent* s : CompsToOutline)
	{
		if (s)
		{
			s->SetRenderCustomDepth(true);
			s->SetCustomDepthStencilValue(2);
		}
	}
}

void AExfilPortal::DisableExfilOutline()
{
	for (UStaticMeshComponent* s : CompsToOutline)
	{
		if (s)
		{
			s->SetRenderCustomDepth(false);
			s->SetCustomDepthStencilValue(2);
		}
	}
}

void AExfilPortal::ServerTriggerExfil_Implementation()
{
	ExfiltrateMission();
}

void AExfilPortal::MulticastEnableExfil_Implementation(bool bEnable)
{
	bEnable ? InteractableComponent->EnableInteractable() : InteractableComponent->DisableInteractable();
}

void AExfilPortal::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	bShowWarningDialogue ? OnPlayerInteracted() : ExfiltrateMission();
}

bool AExfilPortal::CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const
{
	return true;
}

void AExfilPortal::ExfiltrateMission()
{
	if (!HasAuthority())
	{
		ServerTriggerExfil(); // wont this not work since we're calling it on an actor we don't own?
		//						^^ Plan was for squad leader to own exfil portal, irrelevant now since Exfil only for singleplayer
		return;
	}
	
	AReadyOrNotGameMode* GM = GetWorld()->GetAuthGameMode<AReadyOrNotGameMode>();
	if (!GM || !GM->GetIsExfilEnabled())
		return;

	AReadyOrNotPlayerController* PlayerController = UReadyOrNotStatics::GetReadyOrNotPlayerController();
	if (!IsValid(PlayerController))
		return;

	if (ACoopGS* GameState = GetWorld()->GetGameState<ACoopGS>())
	{
		// If the mission is soft completed, we don't trigger an official exfil, we just treat it like player voted to end mission
		if (GameState->bMissionSoftCompleted)
		{
			// There is a very slim possibility that the user requests to exfil the mission when the mission is soft completed, but the mission end vote hasn't begun
			// Vote yes if voting is active so voting counter updates etc, otherwise set vote state to majority yes
			if (GameState->MissionEndVoteState == EMissionEndVoteState::VS_InProgress)
			{
				PlayerController->Vote(true);
			}
			else
			{
				GameState->MissionEndVoteState = EMissionEndVoteState::VS_MajorityYes;
			}

			GM->SetExfilEnabled(false);

			return;
		}
	}

	// Default behaviour for exfil, for in-progress missions and for training
	ScoringComponent->GiveCustomPenalty(AScoringManager::PENALTY_EXFILTRATED_MISSION, 500, true, 0);
	GM->ExfiltrateMission(OverlappingSwatMembers);

}

void AExfilPortal::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	ASWATCharacter* Character = Cast<ASWATCharacter>(OtherActor);
	if (!Character || Character->IsDeadNotUnconscious())
	{
		return;
	}
	
	UpdateOverlappingOfficer(Character, true);
}

void AExfilPortal::ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	ASWATCharacter* Character = Cast<ASWATCharacter>(OtherActor);
	if (!Character)
	{
		return;
	}

	UpdateOverlappingOfficer(Character, false);
}

/** if bAddOfficer is true, adds Officer to list to exfil. Removes otherwise*/
void AExfilPortal::UpdateOverlappingOfficer(ASWATCharacter* Officer, bool bAddOfficer)
{
	if (!IsValid(Officer))
		return;

	if (!bAddOfficer)
	{
		OverlappingSwatMembers.Remove(Officer);
		Officer->OnCharacterKilled.RemoveDynamic(this, &AExfilPortal::OnExfilSwatMemberKilled);
	}
	else if (!OverlappingSwatMembers.Contains(Officer))
	{
		OverlappingSwatMembers.Add(Officer);
		Officer->OnCharacterKilled.AddDynamic(this, &AExfilPortal::OnExfilSwatMemberKilled);
	}

	FText ExfilText = FText::Format(FText::FromStringTable("ActionPromptTable", "ExfilWithXMembers"), OverlappingSwatMembers.Num());
	InteractableComponent->ActionSlot1.ActionText = ExfilText;
}

void AExfilPortal::OnExfilSwatMemberKilled(AReadyOrNotCharacter* Killer, AReadyOrNotCharacter* KilledMember)
{
	UpdateOverlappingOfficer(Cast<ASWATCharacter>(KilledMember), false);
}

void AExfilPortal::OnExfilEnabledChange(bool bEnabled)
{
	MulticastEnableExfil(bEnabled);
}

UScoringComponent* AExfilPortal::GetScoringComponent_Implementation() const
{
	return ScoringComponent;
}

void AExfilPortal::OnMissionSoftComplete()
{
	InteractableComponent->ActionSlot1.ActionText = FText::FromStringTable("ActionPromptTable", "EndMission");
	OnActorBeginOverlap.RemoveDynamic(this, &AExfilPortal::ActorBeginOverlap);
	OnActorEndOverlap.RemoveDynamic(this, &AExfilPortal::ActorEndOverlap);
}

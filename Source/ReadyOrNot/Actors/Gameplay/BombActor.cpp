// Copyright Void Interactive, 2023

#include "Actors/Gameplay/BombActor.h"
#include "ReadyOrNotAIConfig.h"
#include "Actors/Items/Multitool.h"
#include "Commander/RosterManager.h"
#include "Components/InteractableComponent.h"
#include "Components/ScoringComponent.h"
#include "GameModes/DefusalGS.h"
#include "Info/SoundManager.h"

// Sets default values
ABombActor::ABombActor()
{
	bReplicates = true;
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bomb Mesh"));

	if (StaticMeshComponent->GetStaticMesh() == nullptr) 
	{
		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> SM_BombMesh(TEXT("StaticMesh'/Game/ThirdParty/HeistVOL2_Cops_N_Robbers/Meshes/SM_Bomb_01a.SM_Bomb_01a'"));
		StaticMeshComponent->SetStaticMesh(SM_BombMesh.Get());
		SetRootComponent(StaticMeshComponent);
	}

	if (BombTickEvent == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> FM_Event(TEXT("FMODEvent'/Game/FMOD/Events/Modes/Bomb_Threat/Bomb_Countdown.Bomb_Countdown'"));
		BombTickEvent = FM_Event.Object;
	}

	if (BombExplodeEvent == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> BE_Event(TEXT("FMODEvent'/Game/FMOD/Events/Modes/Bomb_Threat/Explosion.Explosion'"));
		BombExplodeEvent = BE_Event.Object;
	}

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Defuse Interactable Component"));
	InteractableComponent->bDistanceFadeIcon = false;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "EquipMultitool"));
	InteractableComponent->ActionSlot1.bCheckForDisallowedItems = false;
	InteractableComponent->ActionSlot2.Init("Fire", IE_Repeat, FText::FromStringTable("ActionPromptTable", "DefuseBomb"));
	InteractableComponent->ActionSlot3.bUseCustomActionText = true;
	InteractableComponent->ActionSlot3.CustomActionPromptText = FText::FromStringTable("ActionPromptTable", "DefusingBomb");
	InteractableComponent->ActionSlot3.bAnimate = true;
	InteractableComponent->ActionSlot3.bLoopAnimation = true;
	InteractableComponent->AnimatedIconName = "Defuse Bomb";
	InteractableComponent->ShowPromptAtDistance = 160.0f;
	InteractableComponent->SetInteractionIconSize(36.0f, 44.0f);
	InteractableComponent->SetupAttachment(StaticMeshComponent);

	ExplosionParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Explosion Particle"));
	ExplosionParticleComponent->bAutoActivate = false;
	ExplosionParticleComponent->SetupAttachment(StaticMeshComponent);
}

void ABombActor::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABombActor, BombState);
	DOREPLIFETIME(ABombActor, TimeUntilExplodes);
}

void ABombActor::Server_FinishedUsingMultitool_Implementation(AReadyOrNotCharacter* ToolOwner)
{
	BombState = EBombState::BS_Disabled;
	
	OnBombDefused.Broadcast(this);
}

void ABombActor::Client_FinishedUsingMultitool_Implementation(AReadyOrNotCharacter* ToolOwner)
{
	BombState = EBombState::BS_Disabled;

	OnBombDefused.Broadcast(this);
}

float ABombActor::GetMultitoolUseTime_Implementation()
{
	return MultitoolUseTime;
}
 
EMultitoolFunctions ABombActor::GetMultitoolUseType_Implementation()
{
	return EMultitoolFunctions::MF_Wirecutter;
}


void ABombActor::Multicast_PlayBombExplodeSFX_Implementation()
{
	UFMODBlueprintStatics::PlayEvent2D(GetWorld(), BombExplodeEvent, true);
	
	for (TActorIterator<AAISpawn>It(GetWorld()); It; ++It)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionParticleComponent->Template, It->GetActorLocation(), FRotator::ZeroRotator, ExplosionParticleComponent->GetComponentScale(), true);
	}
}

void ABombActor::BeginPlay()
{
	Super::BeginPlay();

	if (bPVPBombOnly)
	{
		if (UReadyOrNotFunctionLibrary::IsCoop(GetWorld()))
		{
			Destroy();
		}
	}
	
	TimeUntilExplodes = AI_CONFIG_GET_FLOAT("BombTimer");

	// If the value is not set in the config, default to 5 minutes.
	if(TimeUntilExplodes == 0.0f)
		TimeUntilExplodes = 60*5;

	float TraitAdditionalTime = URosterManager::GetSquadTraitValue("EOD", GetWorld());
	TimeUntilExplodes += TraitAdditionalTime;
	
	BombState = EBombState::BS_Active;
}

void ABombActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (BombSoundSource)
	{
		BombSoundSource->Stop();
	}
}

void ABombActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (UReadyOrNotFunctionLibrary::HasStartedMatch(GetWorld()))
	{
		if (BombState == EBombState::BS_Active)
		{
			if (!BombSoundSource)
			{
				BombSoundSource = USoundSource::CreateThirdPersonSound(GetWorld(), BombTickEvent, GetActorTransform(), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
				BombSoundSource->Play();
			}
			
			TimeUntilExplodes -= DeltaSeconds;
			if (TimeUntilExplodes <= 0.0f && HasAuthority())
			{
				Explode();
			}

			if(BombSoundSource)
				BombSoundSource->SetParameter("BombTime", TimeUntilExplodes < 0.0f ? 3.0f : ( TimeUntilExplodes < 3.0f ? 3.0f : ( TimeUntilExplodes < 30.0f ? 2.0f : 1.0f) ));

		} 
		else if (BombState == EBombState::BS_Disabled)
		{
			if (BombSoundSource)
				BombSoundSource->SetParameter("BombTime", 4.0f);
			
		} else if (BombState == EBombState::BS_HiddenAndFullyDisabled)
		{
			if (BombSoundSource)
			{
				BombSoundSource->Stop();
				BombSoundSource = nullptr;
			}
		}
	}

	APlayerCharacter* PlayerCharacter = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());
	if (PlayerCharacter)
	{
		InteractableComponent->bEnabled = PlayerCharacter->HasLockpick();
		// Update lockpicking information
		if (const AMultitool* Multitool = Cast<AMultitool>(PlayerCharacter->GetEquippedItem()))
		{
			bool bCanUseMultitoolOnBomb = true;
			if (Multitool->CurrentToolKit == EMultitoolFunctions::MF_Wirecutter)
			{
				InteractableComponent->SetAnimatedIconName(Multitool->GetCurrentOperatingTime() > 0.0f && BombState == EBombState::BS_Active ? "Empty" : BombState == EBombState::BS_Active ? "Defuse Bomb" : "Empty");
				InteractableComponent->bOverrideTickInterval = true;
				InteractableComponent->SetComponentTickInterval(0.0167f);
				InteractableComponent->CurrentProgress = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, Multitool->GetMaxOperatingTime()), FVector2D(0.0f, 1.0f), Multitool->GetCurrentOperatingTime());
				InteractableComponent->ShowPromptAtDistance = 160.0f;
				InteractableComponent->ActionSlot1.bCondition = false;
				InteractableComponent->ActionSlot2.bCondition = BombState == EBombState::BS_Active && InteractableComponent->CurrentProgress <= 0.0f;
				InteractableComponent->ActionSlot3.bCondition = InteractableComponent->CurrentProgress > 0.0f;
				InteractableComponent->bShowIconWhenActionsLocked = true;
			}
			else
			{
				InteractableComponent->SetAnimatedIconName(BombState == EBombState::BS_Active ? "Defuse Bomb" : "Empty");
				InteractableComponent->ActionSlot2.bCondition = false;
			}
		}
		else
		{
			InteractableComponent->SetAnimatedIconName(BombState == EBombState::BS_Active ? "Defuse Bomb" : "Empty");
			InteractableComponent->bOverrideTickInterval = false;
			InteractableComponent->ShowPromptAtDistance = 160.0f;
			InteractableComponent->CurrentProgress = 0.0f;
			InteractableComponent->ActionSlot1.bCondition =  BombState == EBombState::BS_Active;
			InteractableComponent->ActionSlot2.bCondition = false;
			InteractableComponent->ActionSlot3.bCondition = false;
			InteractableComponent->bShowIconWhenActionsLocked = false;
		}
	}
}

void ABombActor::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (BombState == EBombState::BS_Active)
	{
		APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
		if (InteractCharacter)
		{
			InteractCharacter->Server_EquipMultitool_Implementation(EMultitoolFunctions::MF_Wirecutter);
		}
	}
}

void ABombActor::Fire_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!InteractInstigator || !InInteractableComponent)
		return;

	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		InteractCharacter->StartUsingMultitool(this);
	}
}

void ABombActor::EndFire_Implementation(AReadyOrNotCharacter* InteractInstigator,
	UInteractableComponent* InInteractableComponent)
{
	if (!InteractInstigator || !InInteractableComponent)
		return;

	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		InteractCharacter->StopUsingMultitool(this);
	}
}

void ABombActor::OnFocusLost_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	EndFire_Implementation(InteractInstigator, InInteractableComponent);
}

bool ABombActor::CanInteract_Implementation() const
{
	return BombState == EBombState::BS_Active;
}

void ABombActor::Explode()
{
	BombState = EBombState::BS_Exploded;
	Multicast_PlayBombExplodeSFX();
	
	ExplosionParticleComponent->Activate(true);

	// Kill all players.
	for (TActorIterator<AReadyOrNotCharacter>It(GetWorld()); It; ++It)
	{
		if(FVector::Distance(GetActorLocation(), It->GetActorLocation()) <= ExplosionRadius)
		{
			It->TakeDamage(999999.0f, FDamageEvent(UDamageType::StaticClass()), It->GetController(), this);
		}
	}

	// Remove all scores.
	for (TObjectIterator<UScoringComponent>It; It; ++It)
	{
		It->TakeAllScores();
	}
}



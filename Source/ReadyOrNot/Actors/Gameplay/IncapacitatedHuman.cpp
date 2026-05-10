// Void Interactive, 2020

#include "IncapacitatedHuman.h"

#include "Animation/AnimSingleNodeInstance.h"
#include "Characters/AI/SWATController.h"

#include "Components/InteractableComponent.h"
#include "Components/ScoringComponent.h"
#include "DamageTypes/BulletDamageType.h"
#include "GameModes/CoopGM.h"

#include "Info/ScoringManager.h"
#include "Senses/ReadyOrNotAISense_Sight.h"

AIncapacitatedHuman::AIncapacitatedHuman()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.1f;

	NetUpdateFrequency = 1.0f;
	MinNetUpdateFrequency = 0.1f;
	bReplicates = true;
	bAlwaysRelevant = true;

	DefaultScene = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultScene"));
	SetRootComponent(DefaultScene);

	HumanMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HumanMesh"));
	HumanMesh->SetupAttachment(DefaultScene);
	HumanMesh->SetCollisionProfileName("CharacterMesh");
	HumanMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	HumanMesh->SetReceivesDecals(false);
	HumanMesh->bComponentUseFixedSkelBounds = true;
	HumanMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;

	HumanMeshFace = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HumanMeshFace"));
	HumanMeshFace->SetupAttachment(HumanMesh);
	HumanMeshFace->SetMasterPoseComponent(HumanMesh);
	HumanMeshFace->SetCollisionProfileName("CharacterMesh");
	HumanMeshFace->SetReceivesDecals(false);
	HumanMeshFace->bComponentUseFixedSkelBounds = false;
	HumanMeshFace->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CapsuleComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
	CapsuleComponent->SetCapsuleRadius(10.0f);
	CapsuleComponent->SetCapsuleHalfHeight(70.0f);
	CapsuleComponent->SetupAttachment(HumanMesh);
	CapsuleComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	CapsuleComponent->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f));

	ReportInteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComp"));
	ReportInteractableComponent->ShowPromptAtDistance = 500.0f;
	ReportInteractableComponent->RequiredLookAtPercentage = 0.98f;
	ReportInteractableComponent->bDistanceFadeIcon = false;
	ReportInteractableComponent->bImprintIconOnHUDUponInteraction = true;
	ReportInteractableComponent->bHideUponPlayerMovement = false;
	ReportInteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "ReportIncapacitated"));
	ReportInteractableComponent->ActionSlot1.DisallowedItems.Empty();
	ReportInteractableComponent->ActionSlot2.DisallowedItems.Empty();
	ReportInteractableComponent->ActionSlot3.DisallowedItems.Empty();
	ReportInteractableComponent->ActionSlot4.DisallowedItems.Empty();
	ReportInteractableComponent->SetupAttachment(DefaultScene);

	ScoringComponent = CreateDefaultSubobject<UScoringComponent>(TEXT("Scoring Component"));
	ScoringComponent->bAutoAddToScorePool = false;
	ScoringComponent->ScoreGroupName = "IncapacitatedBodiesReported";
	ScoringComponent->ObjectiveLevel = EObjectiveLevel::SecondaryObjective;

	static ConstructorHelpers::FObjectFinder<UPhysicsAsset> RagdollHumanShared(TEXT("PhysicsAsset'/Game/ReadyOrNot/Character/PHYSICS/Shared/PHYS_Human_Shared_V2.PHYS_Human_Shared_V2'"));
	RagdollPhysicsAsset = RagdollHumanShared.Object;
	
	PerceptionStimuliComp = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionComp"));
}

void AIncapacitatedHuman::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AIncapacitatedHuman, bHasBeenReported);
	DOREPLIFETIME(AIncapacitatedHuman, bIsDead);
	DOREPLIFETIME(AIncapacitatedHuman, bIsInGroup);
	DOREPLIFETIME(AIncapacitatedHuman, bIsMasterOfGroup);
	DOREPLIFETIME(AIncapacitatedHuman, IncapacitatedHumansInGroup);
}

void AIncapacitatedHuman::BeginPlay()
{
	Super::BeginPlay();

	bHasBeenReported = false;
	bIsDead = bStartDead;

	if (DyingMontage)
	{
		HumanMesh->PlayAnimation(DyingMontage, true);
	}

	if (bIsInGroup)
	{
		SetupGroup(true);
	}
	
	if (bStartDead)
	{
		ScoringComponent->AddToScorePool(AScoringManager::SCORE_REPORT_DEAD_BODY);
	}
	else
	{
		ScoringComponent->AddToScorePool(AScoringManager::SCORE_REPORT_INCAPACITATED_BODY);

		IncapacitatedAudioComponent = UFMODBlueprintStatics::PlayEventAttached(FMODEventLoop, HumanMesh, "face", FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);
	}

	if (bAttachReportInteractableToMesh)
	{
		ReportInteractableComponent->AttachToComponent(HumanMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketToAttach);
	}

	//HumanMesh->SetMassOverrideInKg(NAME_None, 1000.0f);
	
	PerceptionStimuliComp->RegisterWithPerceptionSystem();
	PerceptionStimuliComp->RegisterForSense(UReadyOrNotAISense_Sight::StaticClass());

	if (HasAuthority())
	{
	}
}

void AIncapacitatedHuman::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		EditorTick(DeltaTime);
		return;
	}
#endif
	

	if (bIsDead)
	{
		if (!bStartDead)
		{
			HumanMesh->SetCollisionProfileName("PhysicsActor");

			HumanMesh->SetSimulatePhysics(true);
			HumanMesh->Stop();
		}

		if (bHasBeenReported)
		{
			ReportInteractableComponent->ActionSlot1.bCondition = false;

			SetActorTickInterval(1.0f);
			SetActorTickEnabled(false);
			return;
		}
	}

	HumanMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	HumanMesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	
	if (HasAuthority())
	{
		if (bCanEverDieByTime)
		{
			TimeRemainingUntilDead = FMath::Clamp(TimeRemainingUntilDead - DeltaTime, 0.0f, TimeRemainingUntilDead);

			if (TimeRemainingUntilDead <= 0.0f)
			{
				if (!bIsDead)
				{
					BecomeDead();

					ScoringComponent->ChangeScoreGroup(AScoringManager::SCORE_REPORT_DEAD_BODY);
				}
			}
		}
	}

	ReportInteractableComponent->SetAnimatedIconName(DetermineAnimatedIcon_Implementation());
	ReportInteractableComponent->ActionSlot1.ActionText = DetermineActionText_Implementation();
	ReportInteractableComponent->ActionSlot1.bCondition = !bHasBeenReported && (!bIsInGroup || (bIsInGroup && bIsMasterOfGroup)) && UReadyOrNotFunctionLibrary::IsCoop(GetWorld());
}

#if WITH_EDITOR
void AIncapacitatedHuman::EditorTick(float DeltaTime)
{
	if (UAnimSingleNodeInstance* SingleNodeInstance = HumanMesh->GetSingleNodeInstance())
	{
		if (DyingMontage && SingleNodeInstance->GetAnimationAsset() != DyingMontage)
		{
			HumanMesh->SetAnimation(DyingMontage);

			if (!HumanMesh->IsPlaying())
			{
				HumanMesh->PlayAnimation(DyingMontage, true);
			}
		}
	}
}

void AIncapacitatedHuman::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.GetPropertyName() == "bStartDead")
	{
		if (bIsMasterOfGroup)
		{
			for (AIncapacitatedHuman* IncapacitatedHuman : IncapacitatedHumansInGroup)
			{
				if (IncapacitatedHuman)
				{
					IncapacitatedHuman->bStartDead = bStartDead;
				}
			}
		}
	}
	else if (PropertyChangedEvent.GetPropertyName() == "bIsInGroup")
	{
		if (!bIsInGroup)
		{
			if (MasterHumanInGroup)
			{
				MasterHumanInGroup->IncapacitatedHumansInGroup.RemoveAll([&](AIncapacitatedHuman* IncapacitatedHuman)
				{
					return IncapacitatedHuman == this;
				});
				
				MasterHumanInGroup = nullptr;
			}
		}
	}
	else if (PropertyChangedEvent.GetPropertyName() == "bIsMasterOfGroup")
	{
		if (bIsMasterOfGroup)
		{
			SetupGroup();
		}
	}
	else if (PropertyChangedEvent.GetPropertyName() == "IncapacitatedHumansInGroup")
	{
		switch (PropertyChangedEvent.ChangeType)
		{
			case EPropertyChangeType::ArrayRemove:
			{
				for (TActorIterator<AIncapacitatedHuman> It(GetWorld()); It; ++It)
				{
					AIncapacitatedHuman* IncapacitatedHuman = *It;
					if (IncapacitatedHuman->MasterHumanInGroup == this && !IncapacitatedHumansInGroup.Contains(IncapacitatedHuman))
					{
						IncapacitatedHuman->MasterHumanInGroup = nullptr;
					}
				}
			}
			break;
		
			case EPropertyChangeType::ArrayClear:
			{
				for (TActorIterator<AIncapacitatedHuman> It(GetWorld()); It; ++It)
				{
					AIncapacitatedHuman* IncapacitatedHuman = *It;
					if (IncapacitatedHuman->MasterHumanInGroup == this && !IncapacitatedHumansInGroup.Contains(IncapacitatedHuman))
					{
						IncapacitatedHuman->MasterHumanInGroup = nullptr;
					}
				}
			}
			break;

			default:
			break;
		}
		
		SetupGroup();
	}
}
#endif

void AIncapacitatedHuman::BecomeDead()
{
	if (bIsDead)
		return;

	HumanMesh->SetPhysicsAsset(RagdollPhysicsAsset);

	if (bIsInGroup)
	{
		if (bIsMasterOfGroup)
		{
			// Choose a new master if we become dead
			if (IncapacitatedHumansInGroup.Num() > 0)
			{
				if (AIncapacitatedHuman* RandomHuman = IncapacitatedHumansInGroup[FMath::RandRange(0, IncapacitatedHumansInGroup.Num() - 1)])
				{
					RandomHuman->MakeMasterInGroup();
					RandomHuman->IncapacitatedHumansInGroup.Remove(this);
				}
			}
		}
		else
		{
			// Remove ourselves from the group
			if (MasterHumanInGroup)
			{
				MasterHumanInGroup->IncapacitatedHumansInGroup.Remove(this);
			}
		}

		bIsInGroup = false;
		MasterHumanInGroup = nullptr;
	}
	
	bIsDead = true;
	bCanEverDieByTime = false;
	bHasBeenReported = false;
	
	TimeRemainingUntilDead = 0.0f;

	StopAllAudio();
}

bool AIncapacitatedHuman::CanBeSeenFrom(const FVector& ObserverLocation, FVector& OutSeenLocation, int32& NumberOfLoSChecksPerformed, float& OutSightStrength, const AActor* IgnoreActor, const bool* bWasVisible, int32* UserData) const
{
	if (bHasBeenReported)
		return false;
	
	OutSeenLocation = GetActorLocation() + FVector::UpVector * 50.0f;
	
	if (FVector::Distance(ObserverLocation, OutSeenLocation) > 1100.0f)
		return false;
	
	if (const AReadyOrNotCharacter* OwnerCharacter = Cast<AReadyOrNotCharacter>(IgnoreActor))
	{
		FHitResult HitResult;
		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams = OwnerCharacter->GetCollisionQueryParameters();
		CollisionQueryParams.AddIgnoredActor(this);
		CollisionQueryParams.bTraceComplex = false;

		NumberOfLoSChecksPerformed++;
		//DrawDebugLine(GetWorld(), ObserverLocation, OutSeenLocation, HitResult.bBlockingHit ? FColor::Red : FColor::Green, false, 1.0f);
		return !GetWorld()->LineTraceTestByChannel(ObserverLocation, OutSeenLocation, ECC_Visibility, CollisionQueryParams);
	}

	return false;
}

void AIncapacitatedHuman::OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	const float DistanceToIncapHuman = FVector::Distance(Stimulus.ReceiverLocation, Stimulus.StimulusLocation);
	
	if (DistanceToIncapHuman <= 1000.0f)
	{
		if (!InSenseController->IsSightReactingToActor(this))
		{
			FActorSense FlashlightSightSense;
			FlashlightSightSense.Actor = this;
			FlashlightSightSense.Tag = "IncapHumanSeen";
			FlashlightSightSense.SenseReactionTime = 1.0f;
			FlashlightSightSense.SenseForgetTime = 60.0f;
			
			InSenseController->AddActorSightSense(FlashlightSightSense);
		}
	}
}

float AIncapacitatedHuman::TakeDamage(const float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!HasAuthority())
		return 0.0f;
	
	if (bIsDead)
		return 0.0f;

	if (!EventInstigator)
		return 0.0f;

	if (const TSubclassOf<UDamageType> DamageType = DamageEvent.DamageTypeClass)
	{
		if (DamageEvent.GetTypeID() == FPointDamageEvent::ClassID)
		{
			UBulletDamageType* BulletDamage = Cast<UBulletDamageType>(DamageType->GetDefaultObject());
			UStunDamage* StunDamage = Cast<UStunDamage>(DamageType->GetDefaultObject());

			FPointDamageEvent* PointDamageEvent = (FPointDamageEvent*)&DamageEvent;
			if (PointDamageEvent && (BulletDamage || (StunDamage && StunDamage->bCauseHealthDamage)))
			{
				if (EventInstigator->IsA(APlayerController::StaticClass()))
				{
					ScoringComponent->GivePenalty(AScoringManager::PENALTY_KILLED_INCAP_BODY);

					if (AReadyOrNotCharacter* Player = EventInstigator->GetPawn<AReadyOrNotCharacter>())
					{
						if (ACoopGM * GM = GetWorld()->GetAuthGameMode<ACoopGM>())
							GM->IncapHumanKilled(Player, this);
						
						Player->PlayROEViolateTOCResponse();
					}
				}
				else
				{
					ScoringComponent->TakeAllScores();
				}

				BecomeDead();

				ScoringComponent->ChangeScoreGroup(AScoringManager::SCORE_REPORT_DEAD_BODY);
				
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ShotParticleEffect, PointDamageEvent->HitInfo.Location, PointDamageEvent->HitInfo.Normal.Rotation());
			}
		}
	}

	return DamageAmount;
}

void AIncapacitatedHuman::SelectAllInGroup()
{
	#if WITH_EDITOR
	if (bIsInGroup)
	{
		if (bIsMasterOfGroup)
		{
			for (AIncapacitatedHuman* IncapacitatedHuman : IncapacitatedHumansInGroup)
			{
				GEditor->SelectActor(IncapacitatedHuman, true, true, true);
			}
		}
		else
		{
			if (MasterHumanInGroup)
			{
				GEditor->SelectActor(MasterHumanInGroup, true, true, true);
				
				for (AIncapacitatedHuman* IncapacitatedHuman : MasterHumanInGroup->IncapacitatedHumansInGroup)
				{
					GEditor->SelectActor(IncapacitatedHuman, true, true, true);
				}
			}
		}
	}
	#endif
}

void AIncapacitatedHuman::MakeMasterInGroup()
{
	#if WITH_EDITOR
	if (bIsInGroup)
	{
		if (!bIsMasterOfGroup)
		{
			if (MasterHumanInGroup)
			{
				TArray<AIncapacitatedHuman*> IncapacitatedHumansCopy = MasterHumanInGroup->IncapacitatedHumansInGroup;
				IncapacitatedHumansCopy.Remove(this);
				IncapacitatedHumansCopy.AddUnique(MasterHumanInGroup);
				
				bIsMasterOfGroup = true;

				MasterHumanInGroup->bIsMasterOfGroup = false;
				MasterHumanInGroup->MasterHumanInGroup = this;
				MasterHumanInGroup->IncapacitatedHumansInGroup.Empty();
				MasterHumanInGroup = nullptr;
				
				IncapacitatedHumansInGroup = IncapacitatedHumansCopy;
				
				SetupGroup();
			}
		}
	}
	#endif
}

void AIncapacitatedHuman::Interact_Implementation(AReadyOrNotCharacter* InInteractInstigator, UInteractableComponent* InInteractableComponent)
{
	if (!InInteractInstigator || !InInteractableComponent)
		return;

	InInteractInstigator->Server_ReportTarget(this);
}

FName AIncapacitatedHuman::DetermineAnimatedIcon_Implementation() const
{
	return (bIsDead ? "Report Dead" : "Report Incapacitated");
}

FText AIncapacitatedHuman::DetermineActionText_Implementation() const
{
	if (bIsDead)
	{
		FString ActionTextKey = bIsInGroup && bIsMasterOfGroup && IncapacitatedHumansInGroup.Num() > 0 ? "ReportDeadBodies" : bStartDead ? "ReportDOA" : "ReportDead";
		return FText::FromStringTable("ActionPromptTable", ActionTextKey);
	}

	FString ActionTextKey = bIsInGroup && bIsMasterOfGroup && IncapacitatedHumansInGroup.Num() > 0 ?  "ReportIncapacitatedBodies" : "ReportIncapacitated";
	return FText::FromStringTable("ActionPromptTable", ActionTextKey);
	
	//return FText::FromString(bStartDead ? (bIsGroup ? "Report Dead Bodies" : "Report DOA") : (bIsDead ? "Report Dead" : "Report Incapacitated"));
}

bool AIncapacitatedHuman::CanInteractThroughHitActors_Implementation(const FHitResult& Hit) const
{
	if (const AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(Hit.GetActor()))
	{
		return Character->IsDeadOrUnconscious();
	}

	return false;
}

void AIncapacitatedHuman::ReportToTOC_Implementation(AReadyOrNotCharacter* Reporter, bool bPlayAnimation)
{
	if (!Reporter)
		return;

	if (Reporter->IsDeadOrUnconscious())
		return;

	if (!HasAuthority())
		return;

	if (bHasBeenReported)
		return;

	UFMODEvent* ReportDead, *ReportArrested, *ReportGeneral;
	Reporter->GetReportableFMODEvents(ReportDead, ReportArrested, ReportGeneral);

	Reporter->Client_PlayFMODEvent2D(bIsDead ? ReportDead : ReportGeneral);

	if (Reporter->GetCurrentWeaponAnimData())
	{
		Reporter->Client_Play1PMontage(Reporter->GetCurrentWeaponAnimData()->RadioSelect.Body_FP);
		Reporter->Play3PMontage(Reporter->GetCurrentWeaponAnimData()->RadioSelect.Body_TP);
	}
	
	bHasBeenReported = true;

	// Report all in group, if any
	int32 TotalScoreAmongstGroup = ScoringComponent->GetTotalScore(true);
	if (bIsInGroup && bIsMasterOfGroup)
	{
		IncapacitatedHumansInGroup.Remove(nullptr);
		for (AIncapacitatedHuman* IncapacitatedHuman : IncapacitatedHumansInGroup)
		{
			IncapacitatedHuman->ScoringComponent->GiveAllScores(true);

			if (AReadyOrNotPlayerState* PS = Cast<AReadyOrNotPlayerState>(Reporter->GetPlayerState()))
			{
				PS->Reports++;
			}

			TotalScoreAmongstGroup += IncapacitatedHuman->ScoringComponent->GetTotalScore(true);
		}
	}

	const bool bReportingMultiple = bIsInGroup && bIsMasterOfGroup && IncapacitatedHumansInGroup.Num() > 0;
	const bool bShowScoreOnHUD = !bIsInGroup || (bIsInGroup && bIsMasterOfGroup);
	FString ScoreTextKey = bIsDead ? (bReportingMultiple ? "DeadBodiesReported" : "DeadBodyReported") : (bReportingMultiple ? "IncapacitatedBodiesReported" : "IncapacitatedBodyReported");
	const FText ScoreText = FText::FromStringTable("ScoringTable", ScoreTextKey);

	if (bIsInGroup)
	{
		ScoringComponent->GiveAllScores(true, bShowScoreOnHUD, ScoreText, 0.0f, bIsInGroup ? TotalScoreAmongstGroup : -1);
	}
	else
	{
		ScoringComponent->GiveAllScores(true, false, ScoreText, 0.0f, bIsInGroup ? TotalScoreAmongstGroup : -1);
		
		if (bShowScoreOnHUD)
		{
			//ScoringComponent->DisplayBonusesAndPenalties();
			ScoringComponent->DisplayBonuses();
			ScoringComponent->DisplayPenalties();
		}
	}
}

bool AIncapacitatedHuman::CanReportNow_Implementation()
{
	return !bHasBeenReported;
}

FString AIncapacitatedHuman::GetSpeechTypeForReport_Implementation()
{
	const bool bIsSwat = (Team == ETeamType::TT_SQUAD || Team == ETeamType::TT_SERT_RED || Team == ETeamType::TT_SERT_BLUE);
		
	if (bIsDead)
	{
		if (bIsSwat)
		{
			return VO_SWAT_GENERAL::CALL_REPORT_DEAD_SWAT;
		}
		
		if (Team == ETeamType::TT_SUSPECT)
		{
			return VO_SWAT_GENERAL::CALL_REPORT_DEAD_SUSPECT;
		}

		return VO_SWAT_GENERAL::CALL_REPORT_DEAD_CIVILIAN;
	}

	if (bIsSwat)
	{
		return VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_SWAT;
	}

	if (Team == ETeamType::TT_SUSPECT)
	{
		return VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_SUSPECT;
	}

	return VO_SWAT_GENERAL::CALL_REPORT_INCAPACITATED_CIVILIAN;
}

UScoringComponent* AIncapacitatedHuman::GetScoringComponent_Implementation() const
{
	return ScoringComponent;
}

void AIncapacitatedHuman::SetupGroup(const bool bRemoveNullElements)
{
	if (bIsMasterOfGroup)
	{
		MasterHumanInGroup = nullptr;

		if (bRemoveNullElements)
			IncapacitatedHumansInGroup.Remove(nullptr);
		
		for (AIncapacitatedHuman* IncapacitatedHuman : IncapacitatedHumansInGroup)
		{
			if (IncapacitatedHuman)
			{
				IncapacitatedHuman->bStartDead = bStartDead;
				IncapacitatedHuman->bIsInGroup = true;
				IncapacitatedHuman->bIsMasterOfGroup = false;
				IncapacitatedHuman->MasterHumanInGroup = this;
			}
		}
	}
}

void AIncapacitatedHuman::StopAllAudio()
{
	if (IncapacitatedAudioComponent)
	{
		if (IncapacitatedAudioComponent->StudioInstance)
		{
			IncapacitatedAudioComponent->StudioInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
		}

		IncapacitatedAudioComponent = nullptr;
	}
	
	TInlineComponentArray<UFMODAudioComponent*> FMODAudioComponents(this, true);
	GetComponents(FMODAudioComponents, true);

	for (UFMODAudioComponent* FMODAudioComponent : FMODAudioComponents)
	{
		if (FMODAudioComponent->StudioInstance)
		{
			FMODAudioComponent->StudioInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
		}
	}
}

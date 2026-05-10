// Copyright Void Interactive, 2023

#include "CharacterReactionVolume.h"

#include "Characters/CyberneticController.h"
#include "Components/ReadyOrNotCharMovementComp.h"

ACharacterReactionVolume::FOnVolumeTriggered ACharacterReactionVolume::OnVolumeTriggered;

AReactionInterestPoint::AReactionInterestPoint()
{
#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/CharacterReactionVolume/T_ReactionVolumeInterestPoint.T_ReactionVolumeInterestPoint'"));
	
	BillboardComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>("BillboardComponent");
	if (BillboardComponent)
	{
		BillboardComponent->SetSprite(Icon.Object);
		BillboardComponent->SetRelativeScale3D_Direct(FVector(0.5f, 0.5f, 0.5f));
		BillboardComponent->SetIsVisualizationComponent(true);
		BillboardComponent->bIsScreenSizeScaled = true;
		BillboardComponent->bUseInEditorScaling = true;
		BillboardComponent->SetupAttachment(RootComponent);
	}
#endif
}

void AReactionInterestPoint::Destroyed()
{
	Super::Destroyed();

	ACharacterReactionVolume* ReactionVolume = Cast<ACharacterReactionVolume>(GetAttachParentActor());
	if (IsValid(ReactionVolume))
	{
		ReactionVolume->InterestPoints.Remove(this);
	}
}

ACharacterReactionVolume::ACharacterReactionVolume()
{
#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
#else
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
#endif

	bColored = true;
	BrushColor = FColor::Orange;
	
	GetBrushComponent()->SetCollisionObjectType(ECC_VOLUME);
	GetBrushComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/CharacterReactionVolume/T_ReactionVolumeIcon.T_ReactionVolumeIcon'"));
	
	BillboardComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>("BillboardComponent");
	if (BillboardComponent)
	{
		BillboardComponent->SetSprite(Icon.Object);
		BillboardComponent->SetRelativeScale3D_Direct(FVector(0.5f, 0.5f, 0.5f));
		BillboardComponent->SetIsVisualizationComponent(true);
		BillboardComponent->bIsScreenSizeScaled = true;
		BillboardComponent->bUseInEditorScaling = true;
		BillboardComponent->SetupAttachment(RootComponent);
	}
#endif
	
	OnActorBeginOverlap.AddDynamic(this, &ACharacterReactionVolume::OnOverlapBegin);
	OnActorEndOverlap.AddDynamic(this, &ACharacterReactionVolume::OnOverlapEnd);
}

void ACharacterReactionVolume::BeginPlay()
{
	Super::BeginPlay();

	InterestPoints.Remove(nullptr);

	OnVolumeTriggered.AddUObject(this, &ACharacterReactionVolume::OnTaggedVolumeTriggered);
}

void ACharacterReactionVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (!GetWorld())
		return;
	
	GetWorldTimerManager().ClearTimer(TH_AttemptReaction);
	GetWorldTimerManager().ClearTimer(TH_PlayReaction);

	OnVolumeTriggered.RemoveAll(this);
}

void ACharacterReactionVolume::Destroyed()
{
	Super::Destroyed();

	for (AReactionInterestPoint* InterestPoint : InterestPoints)
	{
		if (IsValid(InterestPoint))
			InterestPoint->Destroy();
	}
}

void ACharacterReactionVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if WITH_EDITOR
	bool bIsAnySelected = IsSelectedInEditor();
	if (!bIsAnySelected)
	{
		for (AReactionInterestPoint* InterestPoint : InterestPoints)
		{
			if (IsValid(InterestPoint) && InterestPoint->IsSelectedInEditor())
			{
				bIsAnySelected = true;
				break;
			}
		}
	}
	
	if (bIsAnySelected)
	{
		for (AReactionInterestPoint* InterestPoint : InterestPoints)
		{
			if (!IsValid(InterestPoint))
				continue;
			
			DrawDebugLine(GetWorld(), GetActorLocation(), InterestPoint->GetActorLocation(), FColor::Red, false, 0.0f);
		}
	}
#else
	ensureMsgf(false, TEXT("CharacterReactionVolume should never tick outside editor builds"));
#endif
}

void ACharacterReactionVolume::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);

	InterestPoints.Empty();
	for (AActor* Actor : AttachedActors)
	{
		AReactionInterestPoint* InterestPoint = Cast<AReactionInterestPoint>(Actor);
		if (!IsValid(InterestPoint))
			continue;

		InterestPoints.Add(InterestPoint);
	}
}

void ACharacterReactionVolume::AttemptReaction()
{
	if (!GetWorld())
		return;
	
	if (OverlappingCharacters.Num() <= 0)
		return;

	if (PossibleVoiceLines.Num() <= 0)
		return;

	bool bAnyPlayerInside = false;
	bool bAnyCharacterInCombat = false;

	for (AReadyOrNotCharacter* Character : OverlappingCharacters)
	{
		if (!IsValid(Character))
			continue;

		if (Character->IsPlayerControlled())
			bAnyPlayerInside = true;

		ACyberneticCharacter* CyberneticCharacter = Cast<ACyberneticCharacter>(Character);
		if (CyberneticCharacter && CyberneticCharacter->bHasEverShot && CyberneticCharacter->TimeSinceLastShot < TimeWithoutCombat)
			bAnyCharacterInCombat = true;
	}

	if (!bAnyPlayerInside || bAnyCharacterInCombat)
		return;
	
	for (AReadyOrNotCharacter* Character : OverlappingCharacters)
	{
		if (!IsValid(Character))
			continue;
		
		if (Character->IsPlayerControlled())
			continue;
		
		FString SpeakerName = Character->GetSpeechCharacterName();
		if (ReactedVoices.Contains(SpeakerName))
			continue;
		
		if (bUseEligibleSpeakersOnly && !EligibleSpeakers.Contains(Character->GetSpeechCharacterName()))
			continue;
		
		if (!Character->CanPlayVO())
			continue;

		// We only want characters that aren't actively moving around (idle)
		UReadyOrNotCharMovementComp* MovementComponent = Cast<UReadyOrNotCharMovementComp>(Character->GetMovementComponent());
		if (MovementComponent && !MovementComponent->Velocity.IsNearlyZero())
			continue;

		GetWorldTimerManager().ClearTimer(TH_AttemptReaction);
		ReactedVoices.Add(SpeakerName);
		
		TryLookRandomPoint(Character);
		if (InspectTimeBeforeReaction > 0.0f)
		{
			GetWorldTimerManager().SetTimer(TH_PlayReaction, FTimerDelegate::CreateUObject(this, &ACharacterReactionVolume::PlayReaction, Character), InspectTimeBeforeReaction, false);
		}
		else
		{
			PlayReaction(Character);
		}

		bHasEverTriggered = true;
		if (!VolumeTag.IsNone())
			OnVolumeTriggered.Broadcast(this, VolumeTag);
		
		break;
	}
}

void ACharacterReactionVolume::PlayReaction(AReadyOrNotCharacter* Character)
{
	if (!HasAuthority())
		return;
	
	FString RandomLine = PossibleVoiceLines[FMath::RandRange(0, PossibleVoiceLines.Num() - 1)];

	// If we want a specific voice line, do some extra steps for that, otherwise just use the regular method
	if (!bUseSpecificVoiceLines)
	{
		Character->PlayRawVO(RandomLine);
	}
	else if (UReadyOrNotVoiceConfig* VoiceConfig = UReadyOrNotVoiceConfig::Get())
	{
		FString FileExtension = VoiceConfig->GetVoiceFileExtension();
		FileExtension.RemoveFromStart("*");
		
		Character->Multicast_PlayRawVO(RandomLine + FileExtension);
	}

	// Don't play any additional reactions if we've surpassed max reactions
	CurrentReactions++;
	if (CurrentReactions >= MaxReactions)
		return;

	// Get the length of the sound so we can play it after this one has finished
	if (Character->VoiceSoundSource)
	{
		Character->VoiceSoundSource->OnProgrammerSoundLengthReady.AddUObject(this, &ACharacterReactionVolume::ReactionLengthReady);
	}
}

void ACharacterReactionVolume::ReactionLengthReady(float Length)
{
	if (!GetWorld())
		return;

	// Only attempt the next reaction some time after the current voiceover is finished
	GetWorldTimerManager().SetTimer(TH_AttemptReaction, FTimerDelegate::CreateUObject(this, &ACharacterReactionVolume::AttemptReaction), Length + 1.5f, false);
}

void ACharacterReactionVolume::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!HasAuthority())
		return;
	
	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(OtherActor);
	if (!Character)
		return;

	if (bSwatOnly && !Character->IsOnSWATTeam())
		return;
	
	OverlappingCharacters.Add(Character);

	if (!TH_AttemptReaction.IsValid() && !bHasEverTriggered)
		GetWorldTimerManager().SetTimer(TH_AttemptReaction, FTimerDelegate::CreateUObject(this, &ACharacterReactionVolume::AttemptReaction), TimeBetweenReactionAttempts, true);
}

void ACharacterReactionVolume::OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!HasAuthority())
		return;
	
	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(OtherActor);
	if (!Character)
		return;
	
	OverlappingCharacters.Remove(Character);

	if (OverlappingCharacters.Num() <= 0)
		GetWorldTimerManager().ClearTimer(TH_AttemptReaction);
}

void ACharacterReactionVolume::OnTaggedVolumeTriggered(ACharacterReactionVolume* Volume, FName Tag)
{
	// Make sure volume is valid, in the same world and not the one triggering the delegate
	if (!IsValid(Volume) || Volume->GetWorld() != GetWorld() || Volume == this)
		return;

	// Only disable volumes with a matching tag
	if (VolumeTag.IsNone() || Tag != VolumeTag)
		return;
	
	bHasEverTriggered = true;
	
	GetWorldTimerManager().ClearTimer(TH_AttemptReaction);
	GetWorldTimerManager().ClearTimer(TH_PlayReaction);
}

void ACharacterReactionVolume::TryLookRandomPoint(AReadyOrNotCharacter* Character)
{
	if (!HasAuthority())
		return;
	
	if (InterestPoints.Num() <= 0)
		return;
	
	ACyberneticController* Controller = Character->GetController<ACyberneticController>();
	if (!Controller)
		return;
	
	AReactionInterestPoint* RandomPoint = InterestPoints[FMath::RandRange(0, InterestPoints.Num() - 1)];
	if (IsValid(RandomPoint))
	{
		Controller->GetTargetingComp()->CustomFocusLocation = RandomPoint->GetActorLocation();
	}
}

#if WITH_EDITOR
void ACharacterReactionVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	InterestPoints.Remove(nullptr);
}

bool ACharacterReactionVolume::ShouldTickIfViewportsOnly() const
{
	return true;
}

void ACharacterReactionVolume::AddInterestPoint()
{
	if (!GetWorld())
		return;
	
	AReactionInterestPoint* Actor = GetWorld()->SpawnActor<AReactionInterestPoint>();
	Actor->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	GEditor->SelectNone(true, true);
	GEditor->SelectActor(Actor, true, true);
	
	InterestPoints.Add(Actor);
}

void ACharacterReactionVolume::RemoveInterestPoint()
{
	if (InterestPoints.Num() <= 0)
		return;

	AReactionInterestPoint* Actor = InterestPoints.Last();
	InterestPoints.RemoveAt(InterestPoints.Num() - 1);
	
	if (Actor)
		Actor->Destroy();
}
#endif

// Copyright Void Interactive, 2021


#include "ASequenceInteraction.h"
#include "LevelSequenceActor.h"
#include "DefaultLevelSequenceInstanceData.h"
#include "Characters/ReadyOrNotPlayerController.h"

// Sets default values
AASequenceInteraction::AASequenceInteraction(const FObjectInitializer& Init)
	: Super(Init)
{
	PrimaryActorTick.bCanEverTick = true;
   RadiusComp = CreateDefaultSubobject<UBoxComponent>(TEXT("RadiusBox"));
   RadiusComp->SetupAttachment(RootComponent);
   RadiusComp->SetCollisionResponseToAllChannels(ECR_Ignore);
   RadiusComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AASequenceInteraction::BeginPlay()
{
	Super::BeginPlay();
		
	RadiusComp->OnComponentBeginOverlap.RemoveDynamic(this, &AASequenceInteraction::OnBoxOverlap);
	RadiusComp->OnComponentBeginOverlap.AddDynamic(this, &AASequenceInteraction::OnBoxOverlap);
	RadiusComp->OnComponentEndOverlap.RemoveDynamic(this, &AASequenceInteraction::OnBoxEndOverlap);
	RadiusComp->OnComponentEndOverlap.AddDynamic(this, &AASequenceInteraction::OnBoxEndOverlap);
}

void AASequenceInteraction::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AASequenceInteraction::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bAutoActivateInRange)
		return;

	APlayerCharacter* pc = Cast<APlayerCharacter>(OtherActor);
	if (!pc)
		return;

	if (!pc->IsPlayerControlled())
		return;

	PlaySequence(pc);
}

void AASequenceInteraction::OnBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	
}

void AASequenceInteraction::PlaySequence(APlayerCharacter* OverlappedCharacter)
{
	if (!OverlappedCharacter)
		return;

	LastPlayedSequencerCharacter = OverlappedCharacter;

	OverlappedCharacter->DisableInput(Cast<APlayerController>(OverlappedCharacter->GetController()));

	FMovieSceneSequencePlaybackSettings MovieSceneSequencePlaybackSettings;
	MovieSceneSequencePlaybackSettings.bAutoPlay = true;
	MovieSceneSequencePlaybackSettings.bHideHud = false;
	MovieSceneSequencePlaybackSettings.bPauseAtEnd = false;
	UDefaultLevelSequenceInstanceData* LevelSequenceData = (UDefaultLevelSequenceInstanceData*)DefaultInstanceData;
	bOverrideInstanceData = true;
	FVector ReferencedCharacterLocation = FVector::ZeroVector;
	if (LevelSequenceData)
	{
		FTransform Transform;
		if (OverlappedCharacter->GetFirstPersonCameraComponent())
		{
			FVector ViewLocation;
			OverlappedCharacter->SetActorLocation(OverlappedCharacter->GetActorLocation() + OverlappedCharacter->GetFirstPersonCameraComponent()->GetForwardVector() * -100.0f);
			ViewLocation = OverlappedCharacter->GetFirstPersonCameraComponent()->GetComponentLocation() + OverlappedCharacter->GetFirstPersonCameraComponent()->GetForwardVector() * 150.0f;
			if (ReferencedCharacterViewTarget)
			{
				// Turn Character To Face Player
				FRotator ViewRotationFacePlayer = UKismetMathLibrary::FindLookAtRotation(ReferencedCharacterViewTarget->GetActorLocation(), ViewLocation);
				ViewRotationFacePlayer.Pitch = 0.0f;
				ViewRotationFacePlayer.Roll = 0.0f;
				ReferencedCharacterViewTarget->SetActorRotation(ViewRotationFacePlayer);
				ReferencedCharacterLocation = ReferencedCharacterViewTarget->GetActorLocation();
				FRotator ViewRotation;
				ViewRotation = UKismetMathLibrary::FindLookAtRotation(ViewLocation, ReferencedCharacterViewTarget->GetActorLocation());
				ViewRotation.Pitch = 0.0f;
				Transform.SetRotation(ViewRotation.Quaternion());
			}
			Transform.SetLocation(ViewLocation);
		}
		LevelSequenceData->TransformOrigin = Transform;
	}
	SequencePlayer->Play();
	SequencePlayer->OnFinished.RemoveDynamic(this, &AASequenceInteraction::OnSequencerFinished);
	SequencePlayer->OnFinished.AddDynamic(this, &AASequenceInteraction::OnSequencerFinished);
	if (ReferencedCharacterViewTarget)
	{
		// Turn Character To Face Player
		FRotator ViewRotationFacePlayer = UKismetMathLibrary::FindLookAtRotation(ReferencedCharacterViewTarget->GetActorLocation(), OverlappedCharacter->GetActorLocation());
		ViewRotationFacePlayer.Pitch = 0.0f;
		ViewRotationFacePlayer.Roll = 0.0f;
		ReferencedCharacterViewTarget->SetActorRotation(ViewRotationFacePlayer);
		ReferencedCharacterViewTarget->SetActorLocation(ReferencedCharacterLocation);	
	}
}

void AASequenceInteraction::OnSequencerFinished()
{
	if (LastPlayedSequencerCharacter)
	{
		LastPlayedSequencerCharacter->EnableInput(Cast<APlayerController>(LastPlayedSequencerCharacter->GetController()));
	}
}

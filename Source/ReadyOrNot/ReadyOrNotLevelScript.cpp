// Copyright Void Interactive, 2022

#include "ReadyOrNotLevelScript.h"

// #include "DLSSLibrary.h"
#include "ReadyOrNotGameMode.h"
#include "ReadyOrNotGameState.h"

#include "Characters/CyberneticCharacter.h"

#include "Info/ConversationManager.h"

#include "Engine/LevelStreaming.h"
#include "Engine/BlockingVolume.h"

#include "LevelSequence/Public/LevelSequenceActor.h"
#include "LevelSequence/Public/DefaultLevelSequenceInstanceData.h"
#include "LevelSequence/Public/LevelSequence.h"

#include "Data/LevelData.h"

#include "Actors/Environment/ReadyOrNotAudioVolume.h"
#include "Actors/Environment/ReadyOrNotTriggerVolume.h"
#include "Actors/Gameplay/VisibilityBlockingVolume.h"
#include "lib/ReadyOrNotFunctionLibrary.h"

#include "FMODStudio/Classes/FMODAmbientSound.h"

TAutoConsoleVariable<int32> CVarFlushAllDebug(TEXT("a.RonFlushAllDebug"), 1, TEXT("Disable all debug from drawing (by flushing at end of frame)"));

float AReadyOrNotLevelScript::GlobalPlayDeadCooldown = 0.0f;
bool AReadyOrNotLevelScript::bAnyAIRequestingCover = false;

AReadyOrNotLevelScript::AReadyOrNotLevelScript(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.033f;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	bReplicates = true;
	
	// static ConstructorHelpers::FObjectFinder<ULevelSequence> MVPLevelSequenceFile(TEXT("LevelSequence'/Game/ReadyOrNot/Animations/TP/mvp/four/LS_PVP_MVP_Four.LS_PVP_MVP_Four'"));
	// static ConstructorHelpers::FObjectFinder<ULevelSequence> TeamLevelSequenceFile(TEXT("LevelSequence'/Game/ReadyOrNot/Animations/TP/afteraction/LS_PVP_Afteraction_Win.LS_PVP_Afteraction_Win'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> TAASharpenFilterObj(TEXT("Material'/Game/ReadyOrNot/Assets/Advanced/Postprocess/M_PP_TAA_Sharpen.M_PP_TAA_Sharpen'"));
	TAASharpenFilter = TAASharpenFilterObj.Object;
	// LevelSequenceMVP = MVPLevelSequenceFile.Object;
	// LevelSequenceTeam = TeamLevelSequenceFile.Object;

	EnableOutOfBounds();

	GlobalPlayDeadCooldown = 0.0f;
	bAnyAIRequestingCover = false;
}

void AReadyOrNotLevelScript::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AReadyOrNotLevelScript, LightingType);
}

void AReadyOrNotLevelScript::VisualizeAllAudioSources()
{
	if (bVisualizeAudioSources)
	{
		for (int32 i = 0; i < AudioSourcesInLevel.Num(); i++)
		{
			UAudioComponent* AudioComponent = AudioSourcesInLevel[i]->GetAudioComponent();
			const FSoundAttenuationSettings* CurrentAttenSettings = AudioComponent->GetAttenuationSettingsToApply();

			if (CurrentAttenSettings)
			{
				if (AudioComponent->Sound)
					DrawDebugString(GetWorld(), AudioComponent->GetComponentLocation(), AudioComponent->Sound->GetName(), nullptr, FColor::Blue, 0.05f);

				DrawDebugSphere(GetWorld(), AudioComponent->GetComponentLocation(), CurrentAttenSettings->AttenuationShapeExtents.X, 10, FColor::Green, false, 0.05f, 0, 0.0f);
				DrawDebugSphere(GetWorld(), AudioComponent->GetComponentLocation(), CurrentAttenSettings->FalloffDistance, 12, FColor::Yellow, false, 0.05f, 0, 0.0f);
			}
			else
			{
				if (AudioComponent->Sound)
					DrawDebugString(GetWorld(), AudioComponent->GetComponentLocation(), AudioComponent->Sound->GetName(), nullptr, FColor::Blue, 0.05f);

				DrawDebugSphere(GetWorld(), AudioComponent->GetComponentLocation(), AudioComponent->AttenuationOverrides.AttenuationShapeExtents.X, 10, FColor::Green, false, 0.05f, 0, 0.0f);
				DrawDebugSphere(GetWorld(), AudioComponent->GetComponentLocation(), AudioComponent->AttenuationOverrides.FalloffDistance, 12, FColor::Yellow, false, 0.05f, 0, 0.0f);
			}
		}
		
		for (int32 i = 0; i < FMODAudioSourcesInLevel.Num(); i++)
		{
			UFMODAudioComponent* FMODAudioComponent =  FMODAudioSourcesInLevel[i]->AudioComponent;

			if (FMODAudioComponent->AttenuationDetails.bOverrideAttenuation)
			{
				DrawDebugString(GetWorld(), FMODAudioComponent->GetComponentLocation(), FMODAudioComponent->Event ? FMODAudioComponent->Event->GetName() : "No FMOD event assigned", nullptr, FColor::Blue, 0.05f);

				DrawDebugSphere(GetWorld(), FMODAudioComponent->GetComponentLocation(), FMODAudioComponent->AttenuationDetails.MinimumDistance, 10, FColor::Yellow, false, 0.05f, 0, 0.0f);
				DrawDebugSphere(GetWorld(), FMODAudioComponent->GetComponentLocation(), FMODAudioComponent->AttenuationDetails.MaximumDistance, 12, FColor::Green, false, 0.05f, 0, 0.0f);
			}
			else
			{
				UFMODEvent* Event = FMODAudioComponent->Event;
				FMOD::Studio::EventDescription* EventDesc = IFMODStudioModule::Get().GetEventDescription(Event, EFMODSystemContext::Max);
				float MinDist;
				float MaxDist;
				if (EventDesc)
				{
					EventDesc->getMinMaxDistance(&MinDist, &MaxDist);
					
					DrawDebugString(GetWorld(), FMODAudioComponent->GetComponentLocation(), Event ? Event->GetName() : "No FMOD event assigned", nullptr, FColor::Blue, 0.05f);

					if (MinDist > 0)
						DrawDebugSphere(GetWorld(), FMODAudioComponent->GetComponentLocation(), MinDist * 100, 10, FColor::Yellow, false, 0.05f, 0, 0.0f);
					
					DrawDebugSphere(GetWorld(), FMODAudioComponent->GetComponentLocation(), MaxDist * 100, 12, FColor::Green, false, 0.05f, 0, 0.0f);
				}

				DrawDebugPoint(GetWorld(), FMODAudioComponent->GetComponentLocation(), 10.0f, FColor::Green, false, 0.05f);
			}
		}
	}
}

void AReadyOrNotLevelScript::RefreshAudioSourcesList()
{
	#if WITH_EDITOR
	AudioSourcesInLevel.Empty(AudioSourcesInLevel.Num() + 1);
	FMODAudioSourcesInLevel.Empty(FMODAudioSourcesInLevel.Num() + 1);

	// Standard ambient audio actors
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(this, AAmbientSound::StaticClass(), FoundActors);
		for (AActor* Actor : FoundActors)
		{
			AudioSourcesInLevel.Add(Cast<AAmbientSound>(Actor));
		}
	}

	// FMOD ambient audio actors
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(this, AFMODAmbientSound::StaticClass(), FoundActors);
		for (AActor* Actor : FoundActors)
		{
			FMODAudioSourcesInLevel.Add(Cast<AFMODAmbientSound>(Actor));
		}
	}
	#endif
}

void AReadyOrNotLevelScript::Start3DAudioVisualizer()
{
	#if WITH_EDITOR
	GetWorldTimerManager().ClearTimer(TH_AudioVisualizer);
	
	RefreshAudioSourcesList();
	UReadyOrNotFunctionLibrary::StartTimerForCallback(TH_AudioVisualizer, this, &AReadyOrNotLevelScript::RefreshAudioSourcesList, 5.0f, true);
	
	bVisualizeAudioSources = true;
	#endif
}

void AReadyOrNotLevelScript::Stop3DAudioVisualizer()
{
	GetWorldTimerManager().ClearTimer(TH_AudioVisualizer);

	bVisualizeAudioSources = false;
}

bool AReadyOrNotLevelScript::AllAudioVolumesTicked() const
{
	for (const AReadyOrNotAudioVolume* AudioVolume : AudioVolumes)
	{
		if (IsValid(AudioVolume))
		{
			if (!AudioVolume->HasRanOnce())
				return false;
		}
	}

	return true;
}

void AReadyOrNotLevelScript::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	FString MapName = GetWorld()->GetMapName();
	MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
	LevelData = UBpGameplayHelperLib::GetMapDetailsFromName(MapName);
}


void AReadyOrNotLevelScript::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() >= ROLE_Authority)
	{
		if (LightingScenarios.Num() > 0)
		{
			TArray<ELightType> OutKeys;
			LightingScenarios.GetKeys(OutKeys);
			LightingType = OutKeys[FMath::RandRange(0, LightingScenarios.Num() - 1)];
			OnRep_LightingType();
		}
	}

	GetWorld()->OnWorldBeginPlay.RemoveAll(this);
	GetWorld()->OnWorldBeginPlay.AddUObject(this, &AReadyOrNotLevelScript::OnWorldBeginPlay);

	// OnWorldBeginPlay is not necessarily broadcast when replicated BeginPlay is received
	if (GetNetMode() <= NM_Client)
		OnWorldBeginPlay();
}

void AReadyOrNotLevelScript::OnWorldBeginPlay()
{
	BlockingVolumesInLevel.Empty(100);
	
	for (TActorIterator<ABlockingVolume> It(GetWorld()); It; ++It)
	{
		BlockingVolumesInLevel.AddUnique(*It);
	}

	VisibilityBlockingVolumesInLevel.Empty(100);
	for (TActorIterator<AVisibilityBlockingVolume>It(GetWorld()); It; ++It)
	{
		VisibilityBlockingVolumesInLevel.Add(*It);
	}

	TriggerVolumesInLevel.Empty(100);
	for (TActorIterator<AReadyOrNotTriggerVolume>It(GetWorld()); It; ++It)
	{
		TriggerVolumesInLevel.Add(*It);
	}

	AudioVolumes.Empty(100);
	for (TActorIterator<AReadyOrNotAudioVolume> It(GetWorld()); It; ++It)
	{
		AudioVolumes.AddUnique(*It);
	}
}

void AReadyOrNotLevelScript::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	GlobalPlayDeadCooldown = FMath::Clamp(GlobalPlayDeadCooldown - DeltaTime, 0.0f, GlobalPlayDeadCooldown);

	AIRequestingCover.RemoveAll([&](const ACyberneticCharacter* Element)
	{
		return !IsValid(Element) || !Element->IsActive();
	});
	
	bAnyAIRequestingCover = AIRequestingCover.Num() > 0;
	
	if (bShouldCountdownForOutOfBounds)
	{
		OutOfBoundsTimeRemaining = FMath::Clamp(OutOfBoundsTimeRemaining - DeltaTime, 0.0f, OutOfBounds_MaxTimeLimit);
		
		if (OutOfBoundsTimeRemaining <= 0.0f)
		{
			bShouldCountdownForOutOfBounds = false;
			
			OnOutOfBoundsTimeLimitEnded();
			Delegate_OnOutOfBoundsTimeLimitEnded.Broadcast();
		}
	}
}

void AReadyOrNotLevelScript::EnableOutOfBounds()
{
	bIsOutOfBoundsEnabled = true;
}

void AReadyOrNotLevelScript::DisableOutOfBounds()
{
	StopOutOfBoundsCountdown();
	bIsOutOfBoundsEnabled = false;
}

void AReadyOrNotLevelScript::StartOutOfBoundsCountdown()
{
	bShouldCountdownForOutOfBounds = true;
	OutOfBoundsTimeRemaining = OutOfBounds_MaxTimeLimit;
}

void AReadyOrNotLevelScript::StopOutOfBoundsCountdown()
{
	bShouldCountdownForOutOfBounds = false;
}

void AReadyOrNotLevelScript::OnOutOfBoundsTimeLimitEnded_Implementation()
{
	StopOutOfBoundsCountdown();
}

void AReadyOrNotLevelScript::OnRep_LightingType()
{
	FName* LightingTypeStr = LightingScenarios.Find(LightingType);

	// load our lighting scenario
	if (LightingTypeStr)
	{
		FLatentActionInfo LatentInfo;
		LatentInfo.UUID = 1;
		UGameplayStatics::LoadStreamLevel(GetWorld(), *LightingTypeStr, true, true, LatentInfo);
	}
}

AConversationManager* AReadyOrNotLevelScript::GetConversationManager()
{
	if (!ConversationManager)
	{
		ConversationManager = GetWorld()->SpawnActor<AConversationManager>(AConversationManager::StaticClass());
	}
	return ConversationManager;
}

void AReadyOrNotLevelScript::PlayMVPSequence()
{
	AReadyOrNotGameState* gs = GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (gs)
	{
		gs->InterpToTimeDialtion = 1.0f;
	}
	FVector Location = LevelData.MVPSequenceLocation;
	Location.Z += 5000.0f;

	for (ULevelStreaming* level : GetWorld()->GetStreamingLevels())
	{
		if (!level->GetWorldAssetPackageName().Contains("DEV_After_action", ESearchCase::IgnoreCase))
		{
			level->SetShouldBeVisible(false);
		}
	}

	const FLatentActionInfo LatentInfo;
	UGameplayStatics::LoadStreamLevel(GetWorld(), "DEV_After_action", true, true, LatentInfo);

	FMovieSceneSequencePlaybackSettings MovieSceneSequencePlaybackSettings;
	MovieSceneSequencePlaybackSettings.bAutoPlay = true;
	MovieSceneSequencePlaybackSettings.bHideHud = false;
	MovieSceneSequencePlaybackSettings.bPauseAtEnd = false;
	ALevelSequenceActor* LevelSequenceActor = nullptr;
	if (LastPlayedSequence)
	{
		LastPlayedSequence->Stop();
	}
	LastPlayedSequence = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), LevelSequenceMVP, MovieSceneSequencePlaybackSettings, LevelSequenceActor);
	if (LevelSequenceActor)
	{
		LastPlayedSequence->OnFinished.AddDynamic(this, &AReadyOrNotLevelScript::OnMVPSequenceFinished);
		LevelSequenceActor->bOverrideInstanceData = true;
		UDefaultLevelSequenceInstanceData* LevelSequenceData = Cast<UDefaultLevelSequenceInstanceData>(LevelSequenceActor->DefaultInstanceData);
		if (LevelSequenceData)
		{
			FTransform Transform;
			Transform.SetLocation(Location);
			LevelSequenceData->TransformOrigin = Transform;
		}
		float TimeTillEndMVP = LevelSequenceMVP->GetMovieScene()->GetPlaybackRange().GetUpperBoundValue() / LevelSequenceMVP->GetMovieScene()->GetTickResolution();
		float TimeTillEndTeam = LevelSequenceTeam->GetMovieScene()->GetPlaybackRange().GetUpperBoundValue() / LevelSequenceTeam->GetMovieScene()->GetTickResolution(); 
		if (gs)
		{
			gs->EndPlayTimer = TimeTillEndMVP;
			FTimerHandle DelayEquipmentHandle;
			GetWorld()->GetTimerManager().SetTimer(DelayEquipmentHandle, FTimerDelegate::CreateUObject(gs, &AReadyOrNotGameState::OnSequenceStartedFunc, LevelSequenceMVP), 0.02f, false);
		}
		AReadyOrNotPlayerController* pc = UBpGameplayHelperLib::GetLocalPlayerController(GetWorld());
		if (pc)
		{
	
			pc->Client_ClearHUDWidgets();
			pc->Client_CreateWidget("MatchEnd_PVP");
			if (pc->PlayerCameraManager != nullptr)
			{
				GetWorld()->GetTimerManager().SetTimer(FadeToBlackAfterMVP, FTimerDelegate::CreateUObject(pc->PlayerCameraManager.Get(), &APlayerCameraManager::StartCameraFade, 0.0f, 1.0f, 0.5f, FLinearColor::Black, false, true), TimeTillEndMVP - 2.0f, false);
				GetWorld()->GetTimerManager().SetTimer(FadeFromBlackAfterMVP, FTimerDelegate::CreateUObject(pc->PlayerCameraManager.Get(), &APlayerCameraManager::StartCameraFade, 1.0f, 0.0f, 5.0f, FLinearColor::Black, false, true), TimeTillEndMVP, false);
			}
		}
	}

}

void AReadyOrNotLevelScript::StopMVPSequence()
{
	if (LastPlayedSequence)
	{
		LastPlayedSequence->Stop();
	}
	FLatentActionInfo LatentInfo;
	UGameplayStatics::UnloadStreamLevel(GetWorld(), "DEV_After_action", LatentInfo, true);
}

void AReadyOrNotLevelScript::OnMVPSequenceFinished()
{
	return;
	// Play Group Sequence
	AReadyOrNotGameState* gs = GetWorld()->GetGameState<AReadyOrNotGameState>();
	if (gs)
	{
		FTimerHandle DelayEquipmentHandle;
		GetWorld()->GetTimerManager().SetTimer(DelayEquipmentHandle, FTimerDelegate::CreateUObject(gs, &AReadyOrNotGameState::OnSequenceStartedFunc, LevelSequenceTeam), 0.02f, false);

	}

	if (LastPlayedSequence)
	{
		LastPlayedSequence->Stop();
	}

	FVector Location = LevelData.MVPSequenceLocation;
	Location.Z += 5000.0f;
	ALevelSequenceActor* LevelSequenceActor = nullptr;
	FMovieSceneSequencePlaybackSettings MovieSceneSequencePlaybackSettings;
	MovieSceneSequencePlaybackSettings.bAutoPlay = true;
	MovieSceneSequencePlaybackSettings.bHideHud = false;
	MovieSceneSequencePlaybackSettings.bPauseAtEnd = false;
	LastPlayedSequence = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), LevelSequenceTeam, MovieSceneSequencePlaybackSettings, LevelSequenceActor);
	if (LevelSequenceActor)
	{
		LastPlayedSequence->OnFinished.AddDynamic(this, &AReadyOrNotLevelScript::OnTeamSequenceFinished);
		LevelSequenceActor->bOverrideInstanceData = true;
		UDefaultLevelSequenceInstanceData* LevelSequenceData =Cast<UDefaultLevelSequenceInstanceData>(LevelSequenceActor->DefaultInstanceData);
		if (LevelSequenceData)
		{
			FTransform Transform;
			Transform.SetLocation(Location);
			LevelSequenceData->TransformOrigin = Transform;
		}
	}
}

void AReadyOrNotLevelScript::OnTeamSequenceFinished()
{
	AReadyOrNotGameMode* gm = GetWorld()->GetAuthGameMode<AReadyOrNotGameMode>();
	if (gm)
	{
		gm->NextGame();
	}
}

void AReadyOrNotLevelScript::OnPiracyCheckUpdate_Private(const bool bIsPirated, const FString& ProgramDetected)
{
	OnPiracyCheckUpdate(bIsPirated, ProgramDetected);
}

void AReadyOrNotLevelScript::OnPiracyCheckUpdate_Implementation(bool bIsPirated, const FString& ProgramDetected)
{
}

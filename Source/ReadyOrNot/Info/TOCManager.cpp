// Void Interactive, 2020

#include "TOCManager.h"
#include "Data/TOCData.h"
#include "Subsystems/SubtitlesSubsystem.h"

ATOCManager::ATOCManager()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;
	
	if (TocEvent == nullptr) 
	{
		ConstructorHelpers::FObjectFinder<UFMODEvent> FMODEvent(TEXT("FMODEvent'/Game/FMOD/Events/Dialogue/Vl_TOC.Vl_TOC'"));
		TocEvent = FMODEvent.Object;
	}

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));
	SetRootComponent(SceneComponent);
}

ATOCManager* ATOCManager::Get()
{
	#if !UE_BUILD_SHIPPING
	UE_CLOG(FUObjectThreadContext::Get().IsInConstructor, LogConfig, Fatal, TEXT("Do not call ATOCManager::Get() in constructors"));
	#endif

	const UWorld* World = UBpGameplayHelperLib::GetWorldStatic();
	
	if (!IsValid(World))
		return nullptr;
	
	const AReadyOrNotGameState* GS = World->GetGameState<AReadyOrNotGameState>();
	
	if (!IsValid(GS))
		return nullptr;

	return GS->TOCManager;
}

void ATOCManager::BeginPlay()
{
	Super::BeginPlay();
	
	SetActorLocation(FVector::ZeroVector);
	
	// VoiceSoundSource = NewObject<USoundSource>();
	// VoiceSoundSource->World = GetWorld();
	// VoiceSoundSource->SoundSourceType = ESoundSourceType::SST_FirstPerson;
	// VoiceSoundSource->Initialize(TocEvent, FTransform::Identity, {}, EOcclusionType::OT_None, EPropagationType::PT_None, false);
	//
	// VoiceSoundSource->OnEventStopped.AddDynamic(this, &ATOCManager::IterateTOCQueue);
}

void ATOCManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ATOCManager, QueuedTOCData);
	DISABLE_REPLICATED_PROPERTY(ATOCManager, QueuedTOCData);
}

void ATOCManager::StartTOCResponse(FString Line, const bool bIsNetworked, const ETOCPriority Priority)
{
	if (UBpGameplayHelperLib::IsVoiceOverSuspended(GetWorld()))
		return;
	
	FTOCData TOCData;
	TOCData.TOCLine = Line;
	TOCData.bIsNetworked = bIsNetworked;
	TOCData.QueuePriority = Priority;

	AddDataToQueue(TOCData);

	if (bCanPlayPrefix)
	{
		bCanPlayPrefix = false;
		PlayTOCLine(VO_TOC::TOC_PREFIX, bIsNetworked);
	}
	else
	{
		IterateTOCQueue();
	}
}

void ATOCManager::StopTOCAudio(bool bClearQueue)
{
	if (bClearQueue)
	{
		QueuedTOCData.Empty();
	}
	
	if (VoiceSoundSource)
	{
		VoiceSoundSource->Stop();
	}
}

bool ATOCManager::IsTOCSpeaking() const
{
	return VoiceSoundSource && VoiceSoundSource->bIsActive;
}

bool ATOCManager::IsTOCSpeakingLine(FString Line) const
{
	return IsTOCSpeaking() && CurrentTOCData.TOCLine == Line;
}

void ATOCManager::IterateTOCQueue()
{
	VoiceSoundSource = nullptr;
	
	if (QueuedTOCData.Num() == 0)
	{
		CurrentTOCData = {};
		bCanPlayPrefix = true;
		return;
	}

	const FTOCData TOCData = QueuedTOCData[0];
	QueuedTOCData.RemoveAt(0);
	
	if (TOCData.TOCLine.IsEmpty())
	{
		IterateTOCQueue();
		return;
	}

	CurrentTOCData = TOCData;
	PlayTOCLine(TOCData.TOCLine, TOCData.bIsNetworked);
}

void ATOCManager::PlayTOCLine(const FString& VoiceLine, bool bIsNetworked)
{
	if (bIsNetworked && !HasAuthority())
		return;
	
	if (UReadyOrNotVoiceConfig* VoiceConfig = UReadyOrNotVoiceConfig::Get())
	{
		FString OutFileName, OutFilePath;
		if (VoiceConfig->GetRandomVoiceLine(VoiceLine, VO_TOC::TOC_CHARACTER_NAME, OutFilePath, OutFileName))
		{
			if (bIsNetworked)
			{
				Multicast_PlayTOCSound2D(OutFileName);
				// Multicast_PlayTOCSound2D_Implementation(OutFileName);
			}
			else
			{
				PlayTOCSound_Internal(OutFilePath, OutFileName);
			}
		}
	}
}

bool ATOCManager::AddDataToQueue(const FTOCData& TOCData)
{
	if (TOCData.TOCLine.IsEmpty())
		return false;

	switch (TOCData.QueuePriority)
	{
		case ETOCPriority::ETP_LowPriority:
			if (QueuedTOCData.Num() > 0)
			{
				return false;
			}

			QueuedTOCData.Add(TOCData);
		break;

		case ETOCPriority::ETP_MediumPriority:
			QueuedTOCData.Add(TOCData);
		break;

		case ETOCPriority::ETP_HighPriority:
			if (QueuedTOCData.Num() > 0)
			{
				int32 Index = 0;
				
				if (QueuedTOCData.Last().QueuePriority == ETOCPriority::ETP_Flush)
					Index = 1;

				QueuedTOCData.Insert(TOCData, Index);
			}
			else
			{
				QueuedTOCData.Add(TOCData);
			}
		break;

		case ETOCPriority::ETP_Flush:
			QueuedTOCData.Empty();
			QueuedTOCData.Add(TOCData);
		break;
	}

	return true;
}

bool ATOCManager::RemoveDataFromQueue(const FTOCData& TOCData)
{
	if (QueuedTOCData.Num() < 1)
		return false;

	QueuedTOCData.Remove(TOCData);
	return true;
}

void ATOCManager::LogSpeechData(const FString& VoiceLine, const FString Filename)
{
	V_LOGM(LogReadyOrNot, "Voice Line Played: %s | FileName: %s | TOC", *VoiceLine, *Filename);
}

#if WITH_EDITOR
void ATOCManager::DebugStartTOCResponse()
{
	StartTOCResponse(DebugTOCLine, true, ETOCPriority::ETP_Flush);
}
#endif

void ATOCManager::Multicast_PlayTOCSound2D_Implementation(const FString& FileName)
{
	UReadyOrNotVoiceConfig* VoiceConfig = UReadyOrNotVoiceConfig::Get();
	if (!VoiceConfig)
		return;
	
	FString OutFileName, OutFilePath;
	if (VoiceConfig->GetSpecificVoiceLine(FileName, VO_TOC::TOC_CHARACTER_NAME, OutFilePath, OutFileName))
	{
		PlayTOCSound_Internal(OutFilePath, OutFileName);
	}
}

void ATOCManager::PlayTOCSound_Internal(const FString& FilePath, const FString& FileName)
{
	VoiceSoundSource = USoundSource::CreateFirstPersonSound(GetWorld(), TocEvent, FTransform::Identity, {});
	if (!VoiceSoundSource)
		return;

	VoiceSoundSource->OnEventStopped.RemoveAll(this);
	VoiceSoundSource->OnEventStopped.AddDynamic(this, &ATOCManager::IterateTOCQueue);
	
	VoiceSoundSource->OnProgrammerSoundLengthReady.Clear();
	VoiceSoundSource->OnProgrammerSoundLengthReady.AddWeakLambda(this, [this](float Length, FString FileName)
	{
		USubtitlesStatics::PlaySubtitles(this, VO_TOC::TOC_CHARACTER_NAME, FileName, Length, FSpeakerTags::SwatTag);
	}, FileName);
	
	VoiceSoundSource->SetProgrammerSoundName(FilePath);
	VoiceSoundSource->bWantsProgrammerSoundLength = true;
	
	VoiceSoundSource->Play();
}

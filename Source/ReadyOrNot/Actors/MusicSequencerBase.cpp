#include "MusicSequencerBase.h"
#include "ReadyOrNot.h"

AMusicSequencerBase::AMusicSequencerBase() : Super()
{
	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	RootComponent = Scene;

	bAlwaysRelevant = true;
	bReplicates = true;
}

void AMusicSequencerBase::BeginPlay()
{
	Super::BeginPlay();

	Multicast_ResetAudio();
	Multicast_ResetAudio_Implementation();
}

UMusicData* AMusicSequencerBase::GetMusicData()
{
	AReadyOrNotLevelScript* ls = Cast<AReadyOrNotLevelScript>(GetWorld()->GetLevelScriptActor());
	if (ls)
	{
		return ls->MusicData;
	}
	return nullptr;
}
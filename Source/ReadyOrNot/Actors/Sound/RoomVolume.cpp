// Copyright Void Interactive, 2023

#include "RoomVolume.h"

#include "Info/SoundManager.h"

ARoomVolume::ARoomVolume()
{
	bGenerateOverlapEventsDuringLevelStreaming = true;

	GetBrushComponent()->SetCollisionResponseToChannel(ECC_COVER, ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_OCCLUSION, ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_SOUND, ECR_Ignore);
	GetBrushComponent()->SetCollisionObjectType(ECC_VOLUME);
}

void ARoomVolume::BeginPlay()
{
	Super::BeginPlay();

	if(USoundManager* SoundManager = GetWorld()->GetSubsystem<USoundManager>())
	{
		SoundManager->ConnectRoom();
	}
	
}

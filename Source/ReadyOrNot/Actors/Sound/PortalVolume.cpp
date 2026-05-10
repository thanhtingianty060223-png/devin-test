// Copyright Void Interactive, 2023

#include "PortalVolume.h"

#include "Actors/Door.h"
#include "Info/SoundManager.h"

APortalVolume::APortalVolume()
{
	bGenerateOverlapEventsDuringLevelStreaming = true;
	
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_COVER, ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_OCCLUSION, ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_SOUND, ECR_Ignore);
	GetBrushComponent()->SetCollisionObjectType(ECC_VOLUME);
}

void APortalVolume::BeginPlay()
{
	Super::BeginPlay();
	for (TSoftObjectPtr<AActor> AttachedActor : AttachedObjects)
	{
		if (ADoor* Door = Cast<ADoor>(AttachedActor.Get()))
		{
			Doors.Add(Door);
		}
	}

	if (BreakableGlass_SoftPointer)
	{
		if (ABreakableGlass* Glass = Cast<ABreakableGlass>(BreakableGlass_SoftPointer.Get()))
		{
			BreakableGlasses.Add(Glass);
		}
		else
		{
			TArray<AActor*> AttachedActors;
			BreakableGlass_SoftPointer.Get()->GetAllChildActors(AttachedActors);
			for (AActor* AttachedActor : AttachedActors)
			{
				if (ABreakableGlass* AttachedGlass = Cast<ABreakableGlass>(AttachedActor))
				{
					BreakableGlasses.Add(AttachedGlass);
				}
			}
		}
	}
	
	if(USoundManager* SoundManager = GetWorld()->GetSubsystem<USoundManager>())
	{
		SoundManager->ConnectPortal();
	}
}

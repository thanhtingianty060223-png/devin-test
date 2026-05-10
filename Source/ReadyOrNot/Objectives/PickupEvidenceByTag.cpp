// Copyright Void Interactive, 2021

#include "PickupEvidenceByTag.h"

#include "Actors/Gameplay/CollectedEvidenceActor.h"
#include "Actors/Gameplay/EvidenceActor.h"

void APickupEvidenceByTag::TickObjective()
{
	if (!HasAuthority())
		return;
	
	if (HasCollectedEvidenceByTag(EvidenceTag))
	{
		ObjectiveCompleted();
	}
}

bool APickupEvidenceByTag::HasCollectedEvidenceByTag(const FName& Tag)
{
	for (TActorIterator<ABaseItem> It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		const ABaseItem* Item = *It;

		if (Item->Tags.Contains(Tag) && Item->IsEvidence())
			return true;
	}
	
	for (TActorIterator<AEvidenceActor> It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		const AEvidenceActor* EvidenceActor = *It;
		
		if (EvidenceActor->Tags.Contains(Tag) && EvidenceActor->IsEvidenceCollected())
			return true;
	}
	
	for (TActorIterator<ACollectedEvidenceActor> It(UBpGameplayHelperLib::GetWorldStatic()); It; ++It)
	{
		const ACollectedEvidenceActor* CollectedEvidenceActor = *It;
		
		if (CollectedEvidenceActor->Tags.Contains(Tag))
			return true;
	}
	
	return false;
}

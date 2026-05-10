// Void Interactive, 2020


#include "Actors/Triggers/LobbyFiringRangeArea.h"

ALobbyFiringRangeArea::ALobbyFiringRangeArea()
{
	OverlappingClasses = {AReadyOrNotCharacter::StaticClass()};
}

bool ALobbyFiringRangeArea::IsInFiringRange(AActor* Actor)
{
	if (!Actor)
		return false;

	for (TActorIterator<ALobbyFiringRangeArea>It(Actor->GetWorld()); It; ++It)
	{
		if (It->IsOverlappingActor(Actor))
			return true;
	}
	return false;
}

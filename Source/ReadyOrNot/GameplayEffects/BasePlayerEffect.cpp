// Copyright Void Interactive, 2021


#include "BasePlayerEffect.h"

void UBasePlayerEffect::Initialize_Implementation(AActor* InActor)
{
	Super::Initialize_Implementation(InActor);

	PlayerCharacter = Cast<APlayerCharacter>(InActor);
}

// Void Interactive, 2020


#include "ReadyOrNotTriggerVolume.h"

AReadyOrNotTriggerVolume::AReadyOrNotTriggerVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UTexture2D> ActorSprite(TEXT("Texture2D'/Engine/EditorResources/S_Trigger.S_Trigger'"));
	
}

void AReadyOrNotTriggerVolume::BeginPlay()
{
	Super::BeginPlay();

	SetActorEnableCollision(true);
	GetCollisionComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetCollisionComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetCollisionComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

#if !WITH_EDITOR
	for (TSubclassOf<AActor> Class : OverlappingClasses)
	{
		ensureMsgf(Class, TEXT("%s in world %s has a null overlapping class"), *GetName(), *GetWorld()->GetName());
	}
	
	ensureMsgf(OverlappingClasses.Num() > 0, TEXT("%s in world %s has no overlapping classes. This will not work."), *GetName(), *GetWorld()->GetName());
#endif
	
	const FOnActorSpawned::FDelegate ActorSpawnedDelegate = FOnActorSpawned::FDelegate::CreateUObject(this, &AReadyOrNotTriggerVolume::OnActorSpawned);
	GetWorld()->AddOnActorSpawnedHandler(ActorSpawnedDelegate);

	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &AReadyOrNotTriggerVolume::GenerateOverlappingActors, 1.0f);
}

void AReadyOrNotTriggerVolume::GenerateOverlappingActors()
{
	for (TSubclassOf<AActor> Class : OverlappingClasses)
	{
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), Class, OutActors);
		for (int32 i = 0; i < OutActors.Num(); i++)
		{
			OnActorSpawned(OutActors[i]);
		}
	
	}
}

void AReadyOrNotTriggerVolume::OnActorSpawned(AActor* Actor)
{
	if (Actor->IsRootComponentMovable())
	{
		for (TSubclassOf<AActor> Class : OverlappingClasses)
		{
			if (!Class)
				continue;
			
			if (Actor->IsA(Class))
			{
				TestActors.AddUnique(Actor);
				break;
			}
		}
	}

	
	if (Actor)
	{
		if (!TestActors.Contains(Actor))
		{
			GetCollisionComponent()->MoveIgnoreActors.Add(Actor);
			UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
			if (PrimitiveComponent)
			{
				PrimitiveComponent->MoveIgnoreActors.Add(this);
			}
		}
	}
	
}

bool AReadyOrNotTriggerVolume::IsOverlappingActor(AActor* Actor)
{
	return GetCollisionComponent()->IsOverlappingActor(Actor);
}

bool AReadyOrNotTriggerVolume::IsOverlappingAllActorsOfType()
{
	for (AActor* a : TestActors)
	{
		if (!GetCollisionComponent()->IsOverlappingActor(a))
			return false;
	}
	return true;
}

int32 AReadyOrNotTriggerVolume::GetOverlappingCount()
{
	int32 i = 0;
	for (AActor* a : TestActors)
	{
		if (GetCollisionComponent()->IsOverlappingActor(a))
		{
			i++;
		}
	}
	return i;
}

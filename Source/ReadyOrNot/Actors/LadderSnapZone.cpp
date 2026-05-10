#include "LadderSnapZone.h"
#include "ReadyOrNot.h"
#include "Actors/Items/TelescopicLadder.h"

ALadderSnapZone::ALadderSnapZone()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;	// turning this off, it doesn't need to tick --eez

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SelectionZone = CreateDefaultSubobject<UBoxComponent>(TEXT("SelectionZone"));
	SelectionZone->SetupAttachment(SceneRoot);

	GhostLadder = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GhostLadder"));
	GhostLadder->SetupAttachment(SceneRoot);

	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	Collision->SetupAttachment(GhostLadder);

}

void ALadderSnapZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALadderSnapZone, AttachedLadder);
}

void ALadderSnapZone::BeginPlay()
{
	Super::BeginPlay();
// 	Multicast_StopShowingGhostMesh();
// 	DisableCollision();
}

void ALadderSnapZone::EnableCollision()
{
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

void ALadderSnapZone::DisableCollision()
{
	Collision->SetGenerateOverlapEvents(false);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	if (!Collision->OnComponentBeginOverlap.IsBound())
	{
		Collision->OnComponentBeginOverlap.AddDynamic(this, &ALadderSnapZone::OnCollisionOverlapBegin);
	}
	if (!Collision->OnComponentEndOverlap.IsBound())
	{
		Collision->OnComponentEndOverlap.AddDynamic(this, &ALadderSnapZone::OnCollisionOverlapEnd);
	}
}

void ALadderSnapZone::OnCollisionOverlapBegin(UPrimitiveComponent* Comp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
	bool bFromSweep, const FHitResult & SweepResult)
{
	APlayerCharacter* OtherCharacter = Cast<APlayerCharacter>(OtherActor);
	if (OtherCharacter && !OtherCharacter->IsDeadOrUnconscious())
	{
		NumberOverlappers++;
		bUnableToPlace = true;
		
		if (GhostLadder)
		{
			GhostLadder->SetMaterial(1, InvalidPlacementMaterial);
		}
	}
}

void ALadderSnapZone::OnCollisionOverlapEnd(UPrimitiveComponent* Comp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacter* OtherCharacter = Cast<APlayerCharacter>(OtherActor);
	if (OtherCharacter && !OtherCharacter->IsDeadOrUnconscious())
	{
		NumberOverlappers--;
		if (NumberOverlappers <= 0)
		{
			bUnableToPlace = false;

			if (GhostLadder)
			{
				GhostLadder->SetMaterial(1, ValidPlacementMaterial);
			}
		}
	}
}

void ALadderSnapZone::Multicast_StartShowingGhostMesh_Implementation(bool bAbleToPlace)
{
	if (AttachedLadder != nullptr)
	{
		GhostLadder->SetVisibility(false);
	}
	else
	{
		GhostLadder->InitAnim(true);
		GhostLadder->SetVisibility(true);
		if (bAbleToPlace)
		{
			GhostLadder->SetMaterial(1, ValidPlacementMaterial);
		}
		else
		{
			GhostLadder->SetMaterial(1, InvalidPlacementMaterial);
		}
	}
}

void ALadderSnapZone::Multicast_StopShowingGhostMesh_Implementation()
{
	GhostLadder->SetVisibility(false);
}

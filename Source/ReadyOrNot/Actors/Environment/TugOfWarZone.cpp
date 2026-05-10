// Copyright Void Interactive, 2021

#include "TugOfWarZone.h"
#include "ReadyOrNot.h"
#include "GameModes/TugOfWarGS.h"
#include "GameModes/KingOfTheHillGS.h"

// Sets default values
ATugOfWarZone::ATugOfWarZone()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetGenerateOverlapEvents(true);
	Bounds->OnComponentBeginOverlap.AddDynamic(this, &ATugOfWarZone::OnOverlapBegin);
	Bounds->OnComponentEndOverlap.AddDynamic(this, &ATugOfWarZone::OnOverlapEnd);
	Bounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	Bounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Bounds->SetCollisionObjectType(ECC_GameTraceChannel9);
	RootComponent = Bounds;
	bReplicates = true;
	bAlwaysRelevant = true;
}

void ATugOfWarZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATugOfWarZone, bZoneDisabled);
}

// Called when the game starts or when spawned
void ATugOfWarZone::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld())
	{	// Tug of War buttons only spawn in Tug of War
		ATugOfWarGS* towgs = GetWorld()->GetGameState<ATugOfWarGS>();
		AKingOfTheHillGS* kothgs = GetWorld()->GetGameState<AKingOfTheHillGS>();
		if (!towgs && !kothgs)
		{
			Destroy();
			return;
		}
	}

	if (GetLocalRole() >= ROLE_Authority)
	{
		// we only need to bind these on the server, they aren't used on the client
		InfluencerKilledDelegate.BindUFunction(this, "OnInfluencerKilled");
		InfluencerArrestedDelegate.BindUFunction(this, "OnInfluencerArrested");
		InfluencerStunnedDelegate.BindUFunction(this, "OnInfluencerStunned");
	}
	
}

// Called every frame
void ATugOfWarZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!Mover)
	{
		for (TActorIterator<ATugOfWarMover>It(GetWorld()); It; ++It)
		{
			Mover = *It;
		}
	}

	SetActorHiddenInGame(bZoneDisabled);
}

void ATugOfWarZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(OtherActor);
	if (!pc)
	{
		return;
	}

	if (!Mover)
	{
		return;
	}

	if (bZoneDisabled)
	{
		return;
	}

	// add events for when specific stuff happens to the influencer here
	pc->OnCharacterKilled.AddUnique(InfluencerKilledDelegate);
	pc->OnPlayerArrestStart.AddUnique(InfluencerArrestedDelegate);
	pc->OnStunnedEvent.AddUnique(InfluencerStunnedDelegate);

	// add this person to the list of influencers
	Mover->Influencers.AddUnique(pc);
	Mover->UpdateMovement();

}

void ATugOfWarZone::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(OtherActor);
	if (!pc)
	{
		return;
	}

	if (!Mover)
	{
		return;
	}

	if (bZoneDisabled)
	{
		return;
	}

	// remove this person from the list of influencers
	if (Mover->Influencers.Contains(pc))
	{
		Mover->Influencers.Remove(pc);
		Mover->UpdateMovement();
	}
}

void ATugOfWarZone::OnInfluencerKilled(AActor* Causer, ACharacter* InstigatorCharacter, ACharacter* KilledCharacter, struct FDamageEvent const& DamageEvent, APlayerState* LastPlayerState)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(KilledCharacter);
	if (!pc)
	{
		return;
	}

	RemoveInfluencer(pc);
}

void ATugOfWarZone::OnInfluencerArrested(APlayerCharacter* ArrestedCharacter, APlayerCharacter* InstigatorCharacter)
{
	RemoveInfluencer(ArrestedCharacter);
}

void ATugOfWarZone::RemoveInfluencer(APlayerCharacter* Influencer)
{
	if (!Mover)
	{
		return;
	}

	if (Mover->Influencers.Contains(Influencer))
	{
		Mover->Influencers.Remove(Influencer);
		Mover->UpdateMovement();
	}
}


#include "TugOfWarMover.h"
#include "ReadyOrNot.h"
#include "GameModes/TugOfWarGM.h"
#include "GameModes/TugOfWarGS.h"
#include "GameModes/KingOfTheHillGS.h"

// Sets default values
ATugOfWarMover::ATugOfWarMover()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bAlwaysRelevant = true;
	bReplicates = true;
	MoverPath = CreateDefaultSubobject<USplineComponent>(TEXT("MoverPath"));
	SetRootComponent(MoverPath);
	SetActorHiddenInGame(true);

	MoverMesh = CreateDefaultSubobject<USkeletalMeshComponent>("Mover");
	MoverMesh->SetupAttachment(RootComponent);
}

void ATugOfWarMover::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATugOfWarMover, bMoverMoving);
	DOREPLIFETIME(ATugOfWarMover, bMoverForward);
	DOREPLIFETIME(ATugOfWarMover, MoverCurrentPosition);
	DOREPLIFETIME(ATugOfWarMover, bContested);
}

void ATugOfWarMover::BeginPlay()
{
	Super::BeginPlay();

	if (!GetWorld())
	{
		return;
	}

	ATugOfWarGS* towgs = GetWorld()->GetGameState<ATugOfWarGS>();
	if (towgs)
	{
		// otherwise, this is a valid mover and we should replicate to all clients!
		towgs->Mover = this;
	}

	AKingOfTheHillGS* kothgs = GetWorld()->GetGameState<AKingOfTheHillGS>();
	if (kothgs)
	{
		kothgs->Mover = this;
	}



	// set the mover's mesh to be at the artist designated point along the spline point
	const FVector StartingPosition = MoverPath->GetLocationAtDistanceAlongSpline(MoverStartingPosition, ESplineCoordinateSpace::Local);
	const FRotator StartingRotation = MoverPath->GetRotationAtDistanceAlongSpline(MoverStartingPosition, ESplineCoordinateSpace::Local);

	MoverMesh->SetRelativeLocationAndRotation(StartingPosition, StartingRotation);

	MoverCurrentPosition = MoverStartingPosition;
}

void ATugOfWarMover::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FVector CurrentPosition = MoverPath->GetWorldLocationAtDistanceAlongSpline(MoverCurrentPosition * MoverPath->GetDistanceAlongSplineAtSplinePoint(MoverPath->GetNumberOfSplinePoints() - 1));
	const FRotator CurrentRotation = MoverPath->GetRotationAtDistanceAlongSpline(MoverCurrentPosition * MoverPath->GetDistanceAlongSplineAtSplinePoint(MoverPath->GetNumberOfSplinePoints() - 1), ESplineCoordinateSpace::Type::World);

	MoverMesh->SetWorldLocationAndRotation(CurrentPosition, CurrentRotation);

	if (!bMoverMoving)
	{
		// don't run the below logic if the mover isn't moving
		return;
	}

	if (GetLocalRole() >= ROLE_Authority)
	{
		AReadyOrNotGameMode_PVP* gm = Cast<AReadyOrNotGameMode_PVP>(GetWorld()->GetAuthGameMode());
		
		if (gm && gm->CurrentMatchState != EMatchState::MS_Playing)
		{
			// not in play mode = don't tick
			return;
		}

		for (APlayerCharacter* pc : Influencers)
		{
			if (pc)
			{
				AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(pc->GetPlayerState());
				if (ps)
				{
					ps->SetScore(ps->GetScore() + 1.0f * DeltaSeconds);
				}
			}
		}

		if (MoverCurrentPosition <= 0.0f)
		{
			gm->RoundWonTeam(bInvertVictoryPositions ? ETeamType::TT_SERT_RED : ETeamType::TT_SERT_BLUE);
			bMoverMoving = false;	// STOP THE TRAIN!!
			// reset the mover
			MoverCurrentPosition = 0.5f;
		}
		else if (MoverCurrentPosition >= 1.0f)
		{
			gm->RoundWonTeam(bInvertVictoryPositions ? ETeamType::TT_SERT_BLUE : ETeamType::TT_SERT_RED);
			bMoverMoving = false;	// STOP THE TRAIN!!
			// reset the mover
			MoverCurrentPosition = 0.5f;
		}		

		// move the mover along the spline
		if (bMoverForward)
		{
			MoverCurrentPosition += DeltaSeconds * MoverSpeed;
		}
		else
		{
			MoverCurrentPosition -= DeltaSeconds * MoverSpeed;
		}

		if (AnnouncerDelay >= 0.0f)
		{
			AnnouncerDelay -= DeltaSeconds;
		}
	}
}

#if WITH_EDITOR
void ATugOfWarMover::PostEditChangeProperty(struct FPropertyChangedEvent & PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.Property)
	{
		return;
	}

	RepositionMesh();
}

void ATugOfWarMover::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bFinished)
	{
		RepositionMesh();
	}
}

void ATugOfWarMover::RepositionMesh()
{
	const FVector StartingPosition = MoverPath->GetWorldLocationAtDistanceAlongSpline(MoverStartingPosition);
	const FRotator StartingRotation = MoverPath->GetRotationAtDistanceAlongSpline(MoverStartingPosition, ESplineCoordinateSpace::Type::World);

	MoverMesh->SetWorldLocationAndRotation(StartingPosition, StartingRotation);
}
#endif

void ATugOfWarMover::UpdateMovement()
{
	TMap<ETeamType, int32> MoverTeamCount;
	MoverTeamCount.Add(ETeamType::TT_SERT_RED, 0);
	MoverTeamCount.Add(ETeamType::TT_SERT_BLUE, 0);

	for (int32 i = 0; i < Influencers.Num(); i++)
	{
		if (!Influencers[i])
		{	// skip this influencer
			continue;
		}

		ETeamType InfluencerTeam = Influencers[i]->GetTeam();
		int32 CountForTeam = MoverTeamCount[InfluencerTeam];
		CountForTeam++;
		MoverTeamCount[InfluencerTeam] = CountForTeam;
	}

	AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	if (MoverTeamCount[ETeamType::TT_SERT_RED] > MoverTeamCount[ETeamType::TT_SERT_BLUE])
	{
		// more red people are moving than blue people
		bMoverMoving = true;
		bMoverForward = !bInvertVictoryPositions;
		bContested = false;
		if (AnnouncerDelay <= 0.0f)
		{
			if (LastAnnouncedTeamType != ETeamType::TT_SERT_RED)
			{
				AnnouncerDelay = 30.0f;
				LastAnnouncedTeamType = ETeamType::TT_SERT_RED;
				gs->PlayAnnouncerForTeam("TOWTeamIsCapturing", ETeamType::TT_SERT_RED);
				gs->PlayAnnouncerForTeam("TOWEnemyIsCapturing", ETeamType::TT_SERT_BLUE);
			}
		}
		
	}
	else if (MoverTeamCount[ETeamType::TT_SERT_BLUE] > MoverTeamCount[ETeamType::TT_SERT_RED])
	{
		// more blue people are moving than red people
		bMoverMoving = true;
		bMoverForward = bInvertVictoryPositions;
		bContested = false;
		if (AnnouncerDelay <= 0.0f)
		{
			if (LastAnnouncedTeamType != ETeamType::TT_SERT_BLUE)
			{
				AnnouncerDelay = 30.0f;
				LastAnnouncedTeamType = ETeamType::TT_SERT_BLUE;
				gs->PlayAnnouncerForTeam("TOWTeamIsCapturing", ETeamType::TT_SERT_BLUE);
				gs->PlayAnnouncerForTeam("TOWEnemyIsCapturing", ETeamType::TT_SERT_RED);
			}
		}
	}
	else
	{

		if (MoverTeamCount[ETeamType::TT_SERT_BLUE] > 0 && MoverTeamCount[ETeamType::TT_SERT_RED] > 0)
		{
			AnnouncerDelay = 30.0f;
			LastAnnouncedTeamType = ETeamType::TT_NONE;
			if (AnnouncerDelay <= 0.0f)
				gs->PlayAnnouncerForTeam("TOWContested", ETeamType::TT_NONE);
			bContested = true;
		}
		else
		{
			bContested = false;
		}

		
		// tied
		bMoverMoving = false;
	}
}


// Void Interactive, 2020

#include "TeamCoverAreaActivity.h"

#include "Characters/CyberneticController.h"

UTeamCoverAreaActivity::UTeamCoverAreaActivity()
{
    ActivityName = 	FText::FromStringTable("SwatCommandTable", "Cover");
    bIsProgressActivity = false;
    bNoMoveActivity = true;
}

void UTeamCoverAreaActivity::PerformActivity(const float DeltaTime)
{
    Super::PerformActivity(DeltaTime);
}

void UTeamCoverAreaActivity::FinishedActivity(const bool bSuccess)
{
    Super::FinishedActivity(bSuccess);
}

bool UTeamCoverAreaActivity::OverrideFocalPoint(FVector& FocalPoint)
{
    FocalPoint = SharedData->CommandLocation;
    return true;
}

bool UTeamCoverAreaActivity::ShouldForceNoStrafe() const
{
    return false;
}

bool UTeamCoverAreaActivity::ShouldForceStrafe() const
{
    return true;
}

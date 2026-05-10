// Void Interactive, 2020

#include "ReportTargetActivity.h"

#include "Actors/Gameplay/IncapacitatedHuman.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

UReportTargetActivity::UReportTargetActivity()
{
	ActivityName = 	FText::FromStringTable("SwatCommandTable", "ReportTarget");
	bAbortIfTrackingEnemy = true;
}

void UReportTargetActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	AActor* ReportActor = Cast<AActor>(ReportTarget.GetObject());
	if (!ReportActor)
		return;

	Location = ReportActor->GetActorLocation();
	
	if (HasReachedLocation(400.0f))
	{
		const bool bCanReport = IReportable::Execute_CanReportNow(ReportActor);
		
		if (bCanReport)
			GetCharacter()->Server_ReportToTOC(ReportActor);
			
		OwningController->FinishActivity(this, true, true);
	}
}

bool UReportTargetActivity::CanFinishActivity() const
{
	return false;
}

bool UReportTargetActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	if (HasReachedLocation(600.0f))
	{
		AActor* ReportActor = Cast<AActor>(ReportTarget.GetObject());
		if (!ReportActor)
			return false;
		
		FocalPoint = ReportActor->GetActorLocation();
		return true;
	}

	return false;
}

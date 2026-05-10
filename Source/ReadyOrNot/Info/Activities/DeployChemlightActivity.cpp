// Void Interactive, 2020

#include "Info/Activities/DeployChemlightActivity.h"

#include "Actors/Items/Chemlight.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

UDeployChemlightActivity::UDeployChemlightActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "DeployChemlight");

	DeployItemClass = AChemlight::StaticClass();
}

void UDeployChemlightActivity::EnterDeployStage()
{
	ProgressState = FText::FromStringTable("SwatCommandTable", "Deploying");

	Blueprint_EnterDeployStage();

	GetCharacter()->PlayMontageFromTable(GetDeployAnimation());
	
	GetCharacter()->OnItemThrown_FromAnimNotify.RemoveAll(this);
	GetCharacter()->OnItemThrown_FromAnimNotify.AddDynamic(this, &UDeployChemlightActivity::OnChemlightThrown);

	GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_DEPLOY_CHEMLIGHT);
}

void UDeployChemlightActivity::FinishedActivity(bool bSuccess)
{
	Super::FinishedActivity(bSuccess);

	GetCharacter()->OnItemThrown_FromAnimNotify.RemoveAll(this);
}

void UDeployChemlightActivity::ActivityOverriden(UBaseActivity* OverridingActivity)
{
	Super::ActivityOverriden(OverridingActivity);

	GetCharacter()->OnItemThrown_FromAnimNotify.RemoveAll(this);
}

bool UDeployChemlightActivity::OverrideFocalPoint(FVector& FocalPoint)
{
	return false;
}

FString UDeployChemlightActivity::GetDeployAnimation() const
{
	return "tp_swt_dropchem";
}

void UDeployChemlightActivity::OnChemlightThrown(ABaseItem* Item)
{
	OwningController->FinishActivity(this, true, true);
}

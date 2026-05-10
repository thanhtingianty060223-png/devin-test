// Void Interactive, 2020

#include "DeployItemAtLocationActivity.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

UDeployItemAtLocationActivity::UDeployItemAtLocationActivity()
{
    ActivityName = FText::FromStringTable("SwatCommandTable", "DeployItem");
    bIsProgressActivity = true;

    ActivityStateMachine->AddState("Move To")
                        .BindEventEnter(MAKE_DELEGATE_BINDING(this, &UDeployItemAtLocationActivity::EnterMoveToStage))
                        .BindEventTick(MAKE_DELEGATE_BINDING(this, &UDeployItemAtLocationActivity::TickMoveToStage))
                        .BindEventExit(MAKE_DELEGATE_BINDING(this, &UDeployItemAtLocationActivity::ExitMoveToStage))
                        .CreateTransition("Deploy", MAKE_DELEGATE_BINDING(this, &UDeployItemAtLocationActivity::CanDeploy));
    
    ActivityStateMachine->AddState("Deploy")
                        .BindEventEnter(MAKE_DELEGATE_BINDING(this, &UDeployItemAtLocationActivity::EnterDeployStage))
                        .BindEventTick(MAKE_DELEGATE_BINDING(this, &UDeployItemAtLocationActivity::TickDeployStage))
                        .BindEventExit(MAKE_DELEGATE_BINDING(this, &UDeployItemAtLocationActivity::ExitDeployStage));
}

bool UDeployItemAtLocationActivity::CanFinishActivity() const
{
    return false;
}

bool UDeployItemAtLocationActivity::CanOverrideActivity() const
{
    return false;
}

bool UDeployItemAtLocationActivity::OverrideFocalPoint(FVector& FocalPoint)
{
    FocalPoint = DeployLocation;
    return true;
}

void UDeployItemAtLocationActivity::StartActivity(AAIController* Owner)
{
    Super::StartActivity(Owner);

    CheckEdgeCases();
}

void UDeployItemAtLocationActivity::FinishedActivity(const bool bSuccess)
{
    Super::FinishedActivity(bSuccess);

    if (!bSuccess)
        StopDeployAnim();

    EquipWeapon();
}

void UDeployItemAtLocationActivity::EnterMoveToStage()
{
    SetLocation(DeployLocation, true);
    
    ProgressState = FText::FromStringTable("SwatCommandTable", "InProgress");
}

void UDeployItemAtLocationActivity::TickMoveToStage(float DeltaTime, float Uptime)
{
}

void UDeployItemAtLocationActivity::ExitMoveToStage()
{
}

bool UDeployItemAtLocationActivity::CanDeploy() const
{
    return HasReachedLocation() && DeployItemClass && DeployLocation != FVector::ZeroVector;
}

void UDeployItemAtLocationActivity::EnterDeployStage()
{
    ProgressState = FText::FromStringTable("SwatCommandTable", "Deploying");

    Blueprint_EnterDeployStage();

    PlayDeployAnim();
}

void UDeployItemAtLocationActivity::TickDeployStage(const float DeltaTime, const float Uptime)
{
    Blueprint_TickDeployStage(DeltaTime, Uptime);

    if (!GetCharacter()->IsTableMontagePlaying(GetDeployAnimation()))
        PlayDeployAnim();
}

void UDeployItemAtLocationActivity::ExitDeployStage()
{
    Blueprint_ExitDeployStage();
}

bool UDeployItemAtLocationActivity::CheckEdgeCases()
{
    if (!DeployItemClass)
    {
        ACTIVITY_FAILED("No valid deploy item specified");
        return false;
    }

    if (DeployLocation == FVector::ZeroVector)
    {
        ACTIVITY_FAILED("Cannot deploy at location (0, 0, 0)");
        return false;
    }

    return true;
}

FString UDeployItemAtLocationActivity::GetDeployAnimation() const
{
    return "";
}

void UDeployItemAtLocationActivity::PlayDeployAnim()
{
    if (!GetDeployAnimation().IsEmpty())
        GetCharacter()->PlayMontageFromTableWithFocalPoint(GetDeployAnimation(), DeployLocation);
}

void UDeployItemAtLocationActivity::StopDeployAnim(const float BlendOutTime)
{
    GetCharacter()->StopTPMontageFromTable(GetDeployAnimation(), BlendOutTime);
}

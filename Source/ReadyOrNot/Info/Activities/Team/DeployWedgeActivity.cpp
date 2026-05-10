// Void Interactive, 2020

#include "DeployWedgeActivity.h"

#include "Characters/CyberneticController.h"
#include "Characters/CyberneticCharacter.h"

#include "Actors/Door.h"
#include "Actors/Items/DoorJam.h"

#include "ReadyOrNotAIConfig.h"

UDeployWedgeActivity::UDeployWedgeActivity()
{
    ActivityName = FText::FromStringTable("SwatCommandTable", "WedgeDoor");
    
    bReturnToPositionAfterInteraction = true;
}

void UDeployWedgeActivity::OnInteractionBegin()
{
    Super::OnInteractionBegin();

    ProgressState = bRemoveWedge ? FText::FromStringTable("SwatCommandTable", "RemovingWedge") : FText::FromStringTable("SwatCommandTable", "WedgingDoor");
}

void UDeployWedgeActivity::OnInteractionEnd()
{
    Super::OnInteractionEnd();

    if (bRemoveWedge)
    {
        Door->RemoveWedges();
    }
    else
    {
        if (ADoorJam* DoorJam = Cast<ADoorJam>(GetCharacter()->GetInventoryComponent()->GetInventoryItemOfClass(ADoorJam::StaticClass())))
        {
            DoorJam->Server_FinishDoorjamPlacement_Implementation(Door);

            GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_DOOR_WEDGE_PLACED);
        }
    }

    ProgressState = bRemoveWedge ? FText::FromStringTable("SwatCommandTable", "WedgeRemoved") : FText::FromStringTable("SwatCommandTable", "WedgeDeployed");
}

bool UDeployWedgeActivity::CheckEdgeCases()
{
    if (!Super::CheckEdgeCases())
        return false;

    if (bRemoveWedge)
    {
        if (!Door->IsJammed())
        {
            ACTIVITY_FAILED("Door is not wedged", true);
            return false;
        }
    }
    else
    {
        if (Door->IsJammed())
        {
            ACTIVITY_FAILED("Door is already wedged", true);
            return false;
        }

        if (Door->IsOpen())
        {
            ACTIVITY_FAILED("Door is opened", true);
            return false;
        }
    }

    return true;
}

float UDeployWedgeActivity::GetInteractionDistance() const
{
    return 100.0f;
}

FString UDeployWedgeActivity::GetInteractionAnimation() const
{
    return "tp_swt_placewedge";
}

FVector UDeployWedgeActivity::GetInteractionLocation() const
{
    return Door ? Door->GetWedgeLocation() : Super::GetInteractionLocation();
}

bool UDeployWedgeActivity::CanInteract() const
{
    if (bRemoveWedge)
        return Super::CanInteract() && Door->IsJammed();

    return Super::CanInteract() && !Door->IsJammed() && Door->IsClosed();
}

void UDeployWedgeActivity::EnterGetInPositionStage()
{
    Super::EnterGetInPositionStage();

    ProgressState = bRemoveWedge ? FText::FromStringTable("SwatCommandTable", "PreparingToRemoveWedge") : FText::FromStringTable("SwatCommandTable", "PreparingToWedge");
}

void UDeployWedgeActivity::BindEvents()
{
    Super::BindEvents();

    if (!GetCharacter())
        return;
    
    GetCharacter()->OnStartDoorWedgePlacement_FromAnimNotify.AddDynamic(this, &UDeployWedgeActivity::OnInteractionBegin);
    GetCharacter()->OnEndDoorWedgePlacement_FromAnimNotify.AddDynamic(this, &UDeployWedgeActivity::OnInteractionEnd);
}

void UDeployWedgeActivity::UnbindEvents()
{
    Super::UnbindEvents();

    if (!GetCharacter())
        return;
    
    GetCharacter()->OnStartDoorWedgePlacement_FromAnimNotify.RemoveAll(this);
    GetCharacter()->OnEndDoorWedgePlacement_FromAnimNotify.RemoveAll(this);
}

bool UDeployWedgeActivity::OverrideFocalPoint(FVector& FocalPoint)
{
    if (Door)
    {
        if (Door->IsPointInFrontOfDoorway(CommandLocation))
        {
            FocalPoint = Door->GetWedgeHighlight()->GetComponentLocation() - Door->GetActorRightVector() * 40.0f;
        }
        else
        {
            FocalPoint = Door->GetWedgeHighlight()->GetComponentLocation() + Door->GetActorRightVector() * 40.0f;
        }
        
        return true;
    }

    return Super::OverrideFocalPoint(FocalPoint);
}

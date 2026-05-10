// Copyright Void Interactive, 2021

#include "MirrorUnderDoorActivity.h"

#include "Actors/Door.h"
#include "Actors/Gameplay/TrapActorAttachedToDoor.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "ReadyOrNotAIConfig.h"

UMirrorUnderDoorActivity::UMirrorUnderDoorActivity()
{
    ActivityName = 	FText::FromStringTable("SwatCommandTable", "MirrorUnderDoor");
    MaxActivityTime = 30.0f;

    bReturnToPositionAfterInteraction = true;
}

void UMirrorUnderDoorActivity::PerformActivity(float DeltaTime)
{
    Super::PerformActivity(DeltaTime);

    if (!Door)
    {
        ACTIVITY_FAILED("MirrorForAI | No valid door specified");
        return;
    }
}

void UMirrorUnderDoorActivity::MirrorForAI()
{
    int32 SpottedEnemyCount = 0;
    int32 SpottedCivilianCount = 0;

    for (const ACyberneticCharacter* AI : SpottedCharacters)
    {
        if (AI)
        {
            if (AI->IsSuspect())
            {
                SpottedEnemyCount++;
            }
            else if (AI->IsCivilian())
            {
                SpottedCivilianCount++;
            }
        }
    }
    
    // Call out the results
    if (SpottedEnemyCount == 0 && SpottedCivilianCount == 0)
    {
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_NONE, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "NoTargetsVisible");
    }
    else if (SpottedEnemyCount == 1 && SpottedCivilianCount == 1)
    {
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_CIVILIAN_AND_SUSPECT, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "OneSuspectAndOneCivilianSpotted");
    }
    else if (SpottedEnemyCount == 1 && SpottedCivilianCount == 0)
    {
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_SUSPECT, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "OneSuspectSpotted");
    }
    else if (SpottedEnemyCount == 0 && SpottedCivilianCount == 1)
    {
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_CIVILIAN, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "OneCivilianSpotted");
    }
    else if (SpottedEnemyCount > 1 && SpottedCivilianCount == 0)
    {    
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_MULTIPLE_SUSPECTS, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "MultipleSuspectsSpotted");
    }
    else if (SpottedEnemyCount == 0 && SpottedCivilianCount > 1)
    {
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_MULTIPLE_CIVILIANS, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "MultipleCiviliansSpotted");
    }
    else
    {
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_MULTIPLE_CIVILIANS_AND_SUSPECTS, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "MultipleSuspectsAndCiviliansSpotted");
    }
}

void UMirrorUnderDoorActivity::MirrorForTrap()
{
    if (!GetCharacter())
        return;
            
    if (!Door)
    {
        ACTIVITY_FAILED("MirrorForTrap | No valid door specified");
        return;
    }

    if (Door->GetAttachedTrap() && Door->GetAttachedTrap()->TrapStatus == ETrapState::TS_Live)
    {
        // TODO (Max): Make trapnames FText
        ProgressState = FText::Format(FText::FromStringTable("SwatCommandTable", "TrapNameTrapSpotted"), FText::FromString(Door->GetAttachedTrap()->TrapName));
        
        switch (Door->GetAttachedTrap()->TrapType)
        {
            case ETrapType::Alarm:
                GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_TRAP_EXPLOSIVE, "", false);
            break;
            
            case ETrapType::Flashbang:
                GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_TRAP_EXPLOSIVE, "", false);
            break;

            case ETrapType::Explosive:
                GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_TRAP_EXPLOSIVE, "", false);
            break;

            default:
                GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_TRAP_NO_MIRRORGUN, "", false);
                ProgressState = FText::FromStringTable("SwatCommandTable", "UnknownTrapSpotted");
            break;
        }
    }
    else
    {
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::CALL_SPOTTED_NO_TRAP, "", false);
        ProgressState = FText::FromStringTable("SwatCommandTable", "NoTrapVisible");
    }

    Door->SetDoorTrapKnowledge(GetCharacter()->IsSuspect(), true);
}

void UMirrorUnderDoorActivity::MirrorForRooms()
{
    if (GetCharacter()->VoiceSoundSource &&
        GetCharacter()->VoiceSoundSource->bIsActive)
        return;

    bAttemptedRoomCallout = true;
    
    FString VO = VO_SWAT_GENERAL::CALL_OPENING_FRONT;
    
    const bool bIsCommandInFrontOfDoor = Door->IsPointInFrontOfDoorway(CommandLocation);
    const FVector MirrorPoint = Door->GetDoorMidLocation() + (bIsCommandInFrontOfDoor ? -Door->GetActorForwardVector() : Door->GetActorForwardVector()) * 100.0f;
    
    if (MirroringRoom)
    {
        uint8 NumDoors = 0;
        bool bAllHallways = true;
        for (const ADoor* D : MirroringRoom->AdditionalRootDoors)
        {
            if (D && D != Door && D != Door->GetSubDoor() && (D->IsDoorwayOnly() || D->IsOpenBeyondIncrementThreshold()))
            {
                NumDoors++;
                
                EDoorRoomPosition RoomPosition;
                if (D->IsPointInFrontOfDoorway(MirrorPoint))
                {
                    RoomPosition = D->FrontRoomPosition;
                }
                else
                {
                    RoomPosition = D->BackRoomPosition;
                }

                if (RoomPosition != EDoorRoomPosition::Hallway && RoomPosition != EDoorRoomPosition::HallwayRight && RoomPosition != EDoorRoomPosition::HallwayLeft)
                {
                    bAllHallways = false;
                }
            }
        }

        if (NumDoors > 1)
        {
            if (GetCharacter()->PlayRawVO(bAllHallways ? VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_MULTIPLE : VO_SWAT_GENERAL::CALL_OPENING_MULTIPLE))
            {
                bCalledOutFrontOpening = true;
                bCalledOutLeftOpening = true;
                bCalledOutRightOpening = true;
            }
        }
        else
        {
            for (const ADoor* D : MirroringRoom->AdditionalRootDoors)
            {
                if (D && D != Door && D != Door->GetSubDoor() && (D->IsDoorwayOnly() || D->IsOpenBeyondIncrementThreshold()))
                {
                    const float RightDot = FVector::DotProduct(D->GetActorRightVector(), (D->GetDoorMidLocation() - D->GetDoorMidLocation()).GetSafeNormal2D());
                    
                    if (FMath::Abs(RightDot) > 0.95f) // directly in front
                    {
                        bool* bCallOutPtr;
                        EDoorRoomPosition RoomPosition, RoomPosition_Opposite;
                        if (D->IsPointInFrontOfDoorway(MirrorPoint))
                        {
                            RoomPosition = D->FrontRoomPosition;
                            RoomPosition_Opposite = D->BackRoomPosition;
                        }
                        else
                        {
                            RoomPosition = D->BackRoomPosition;
                            RoomPosition_Opposite = D->FrontRoomPosition;
                        }

                        bool bInsideHallway = RoomPosition_Opposite == EDoorRoomPosition::Hallway || RoomPosition_Opposite == EDoorRoomPosition::HallwayLeft || RoomPosition_Opposite == EDoorRoomPosition::HallwayRight;
                        bool bIsHallway = RoomPosition == EDoorRoomPosition::Hallway;
                        bool bMovesLeft = false;
                        bool bMovesRight = false;

                        VO = VO_SWAT_GENERAL::CALL_OPENING_FRONT;
                        if (bInsideHallway)
                        {
                            bMovesLeft = RoomPosition == EDoorRoomPosition::CornerRight;
                            bMovesRight = RoomPosition == EDoorRoomPosition::CornerLeft;
                            
                            if (bMovesLeft)
                            {
                                VO = VO_SWAT_GENERAL::CALL_OPENING_LEFT;
                            }
                            else if (bMovesRight)
                            {
                                VO = VO_SWAT_GENERAL::CALL_OPENING_RIGHT;
                            }
                        }
                        else if (bIsHallway)
                        {
                            VO = VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_FRONT;
                        }
                        
                        if (bMovesLeft)
                            bCallOutPtr = &bCalledOutLeftOpening;
                        else if (bMovesRight)
                            bCallOutPtr = &bCalledOutRightOpening;
                        else
                            bCallOutPtr = &bCalledOutFrontOpening;

                        if (!(*bCallOutPtr))
                        {
                            if (GetCharacter()->PlayRawVO(VO))
                            {
                                bCalledOutFrontOpening = true;
                                *bCallOutPtr = true;
                            }
                        }
                    }
                    else
                    {
                        if (RightDot > 0.0f)
                        {
                            bool* bCalloutPtr = &bCalledOutRightOpening;
                            
                            EDoorRoomPosition RoomPosition;
                            if (D->IsPointInFrontOfDoorway(MirrorPoint))
                                RoomPosition = D->FrontRoomPosition;
                            else
                                RoomPosition = D->BackRoomPosition;

                            const bool bIsHallway = RoomPosition == EDoorRoomPosition::Hallway;
                            
                            VO = bIsHallway ? VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_RIGHT : VO_SWAT_GENERAL::CALL_OPENING_RIGHT;
                            
                            const bool bInvert = !Door->IsPointInFrontOfDoorway(MirrorPoint);
                            if (bInvert)
                            {
                                VO = bIsHallway ? VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_LEFT : VO_SWAT_GENERAL::CALL_OPENING_LEFT;
                                bCalloutPtr = &bCalledOutLeftOpening;
                            }
                            
                            if (!(*bCalloutPtr))
                            {
                                if (GetCharacter()->PlayRawVO(VO))
                                {
                                    *bCalloutPtr = true;
                                }
                            }
                        }
                        else
                        {
                            bool* bCalloutPtr = &bCalledOutLeftOpening;
                            
                            EDoorRoomPosition RoomPosition;
                            if (D->IsPointInFrontOfDoorway(MirrorPoint))
                                RoomPosition = D->FrontRoomPosition;
                            else
                                RoomPosition = D->BackRoomPosition;

                            const bool bIsHallway = RoomPosition == EDoorRoomPosition::Hallway;
                            
                            VO = bIsHallway ? VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_LEFT : VO_SWAT_GENERAL::CALL_OPENING_LEFT;
                            
                            const bool bInvert = !Door->IsPointInFrontOfDoorway(MirrorPoint);
                            if (bInvert)
                            {
                                VO = bIsHallway ? VO_SWAT_GENERAL::CALL_OPENING_HALLWAY_RIGHT : VO_SWAT_GENERAL::CALL_OPENING_RIGHT;
                                bCalloutPtr = &bCalledOutRightOpening;
                            }
                            
                            if (!(*bCalloutPtr))
                            {
                                if (GetCharacter()->PlayRawVO(VO))
                                {
                                    *bCalloutPtr = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void UMirrorUnderDoorActivity::OnInteractionBegin()
{
    Super::OnInteractionBegin();

    if (Door->IsMirrorBlocked())
    {
        if (const UAnimMontage* Anim = GetCharacter()->GetMontageFromTable(GetInteractionAnimation()))
        {
            if (GetCharacter()->GetMesh()->GetAnimInstance()->Montage_GetCurrentSection() != "Finished")
            {
                GetCharacter()->GetMesh()->GetAnimInstance()->Montage_JumpToSection("Finished", Anim);
            }
        }
    }

    ProgressState = FText::FromStringTable("SwatCommandTable", "Mirroring");
}

void UMirrorUnderDoorActivity::OnInteractionEnd()
{
    Super::OnInteractionEnd();

    if (Door->IsMirrorBlocked())
    {
        GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_NEGATIVE_GENERIC);
        ProgressState = FText::FromStringTable("SwatCommandTable", "MirrorViewBlocked");
        //ACTIVITY_FAILED("OnInteractionEnd | Cannot mirror under door. View is blocked", true);
        return;
    }
    
    switch (MirrorContactType)
    {
        case EMirrorContactType::AI:        MirrorForAI(); break;
        case EMirrorContactType::Trap:      MirrorForTrap(); break;
        case EMirrorContactType::Custom:    MirrorForCustom(); break;
        case EMirrorContactType::Both:
        {
            MirrorForTrap();
            //MirrorForRooms();
            UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UMirrorUnderDoorActivity::MirrorForAI, 2.0f, false);
            //UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &UMirrorUnderDoorActivity::MirrorForTrap, 4.0f, false);
        }
        break;
        
        default:                            ACTIVITY_FAILED("OnInteractionEnd | No valid MirrorContactType specified"); break;
    }
}

void UMirrorUnderDoorActivity::EnterGetInPositionStage()
{
    Super::EnterGetInPositionStage();
    
    ProgressState = FText::FromStringTable("SwatCommandTable", "PreparingForMirror");
}

void UMirrorUnderDoorActivity::EnterInteractStage()
{
    Super::EnterInteractStage();

    GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_MIRRORGUN);
    
    const bool bIsCommandInFrontOfDoor = Door->IsPointInFrontOfDoorway(CommandLocation);
    const FVector MirrorPoint = Door->GetDoorMidLocation() + (bIsCommandInFrontOfDoor ? -Door->GetActorForwardVector() : Door->GetActorForwardVector()) * 100.0f;
    MirroringRoom = UReadyOrNotFunctionLibrary::GetRoomDataForLocation_Ref(MirrorPoint);
}

void UMirrorUnderDoorActivity::PerformInteractStage(float DeltaTime, float Uptime)
{
    if (!bAttemptedRoomCallout)
    {
        float TimeRemaining = 0.0f;
        if (GetCharacter()->IsTableMontagePlayingWithTimeRemaining(GetInteractionAnimation(), TimeRemaining))
        {
            if (TimeRemaining < 7.0f)
            {
                MirrorForRooms();
            }
        }
    }
    
    if (MirrorContactType == EMirrorContactType::Both || MirrorContactType == EMirrorContactType::AI)
    {
        const bool bIsCommandInFrontOfDoor = Door->IsPointInFrontOfDoorway(CommandLocation);
        
        FVector MirrorPoint = Door->GetDoorMidLocation();
        if (Door->GetSubDoor())
			MirrorPoint = Door->GetDoorMidLocation() + Door->GetActorRightVector() * 65.0f;
        
        MirrorPoint += bIsCommandInFrontOfDoor ? Door->GetActorForwardVector() * -75.0f : Door->GetActorForwardVector() * 75.0f;
        MirrorPoint.Z += 50.0f;

        #if !UE_BUILD_SHIPPING
        DrawDebugBox(GetWorld(), MirrorPoint, FVector(5.0f), FColor::White, false, DeltaTime, 0, 0.5f);
        /*
        FString Message;
        if (Door->GetAttachedTrap())
        {
            Message += Door->GetAttachedTrap()->TrapName + " Trap Visible";
            Message += LINE_TERMINATOR;
        }

        Message += "AI Visible: " + FString::FromInt(SpottedCharacters.Num());
        DrawDebugString(GetWorld(), MirrorPoint, Message, nullptr, FColor::White, DeltaTime, true);
        */
        #endif
        
        // Find all characters in sight
        for (ACyberneticCharacter* AI : GetWorld()->GetGameState<AReadyOrNotGameState>()->AllAICharacters)
        {
            if (IsValid(AI) && !AI->IsOnSWATTeam())
            {
                if (!SpottedCharacters.Contains(AI))
                {
                    FHitResult HitHead, HitBody;
                    
                    FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(AI, GetCharacter());
                    
                    if (!GetWorld()->LineTraceSingleByChannel(HitHead, MirrorPoint, AI->GetMesh()->GetBoneLocation("head"), ECC_Visibility, CollisionQueryParams))
                    {
                        SpottedCharacters.AddUnique(AI);
                    }
                    else if (!GetWorld()->LineTraceSingleByChannel(HitBody, MirrorPoint, AI->GetActorLocation(), ECC_Visibility, CollisionQueryParams))
                    {
                        SpottedCharacters.AddUnique(AI);
                    }
                }
                else
                {
                    #if !UE_BUILD_SHIPPING
                    DrawDebugLine(GetWorld(), MirrorPoint, AI->GetMesh()->GetBoneLocation("head"), FColor::Green, false, DeltaTime, 0, 0.5f);
                    #endif
                }
            }
        }
    }
}

void UMirrorUnderDoorActivity::BindEvents()
{
    Super::BindEvents();

    GetCharacter()->OnMirrorDoorStarted_FromAnimNotify.AddDynamic(this, &UMirrorUnderDoorActivity::OnInteractionBegin);
    GetCharacter()->OnMirrorDoorFinished_FromAnimNotify.AddDynamic(this, &UMirrorUnderDoorActivity::OnInteractionEnd);
}

void UMirrorUnderDoorActivity::UnbindEvents()
{
    Super::UnbindEvents();
    
    GetCharacter()->OnMirrorDoorStarted_FromAnimNotify.RemoveAll(this);
    GetCharacter()->OnMirrorDoorFinished_FromAnimNotify.RemoveAll(this);
}

bool UMirrorUnderDoorActivity::OverrideFocalPoint(FVector& FocalPoint)
{
    if (Door)
    {
		if (Door->GetSubDoor())
		{
			FocalPoint = Door->GetDoorMidLocation() + Door->GetActorRightVector() * 65.0f;
		    FocalPoint.Z = Door->GetActorLocation().Z;
		    return true;
		}
        
        FocalPoint = Door->GetMirrorAimHighlight()->GetComponentLocation();
        return true;
    }

    return Super::OverrideFocalPoint(FocalPoint);
}

bool UMirrorUnderDoorActivity::CanInteract() const
{
    return Super::CanInteract() && !Door->IsOpen();
}

float UMirrorUnderDoorActivity::GetInteractionDistance() const
{
    return 80.0f;
}

FString UMirrorUnderDoorActivity::GetInteractionAnimation() const
{
    return "tp_swt_mirrorgun";
}

bool UMirrorUnderDoorActivity::CheckEdgeCases()
{
    if (!Super::CheckEdgeCases())
        return false;

    if (Door->IsOpen())
    {
        ACTIVITY_FAILED("Door is open", true);
        return false;
    }

    return true;
}

void UMirrorUnderDoorActivity::ResetData()
{
    Super::ResetData();
    
	SpottedCharacters.Empty();

    bAttemptedRoomCallout = false;
    
	bCalledOutLeftOpening = false;
	bCalledOutRightOpening = false;
	bCalledOutFrontOpening = false;

	MirroringRoom = nullptr;
}

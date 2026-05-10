// Copyright Void Interactive, 2017

#include "Zipcuffs.h"

#include "Actors/PairedInteractionDriver.h"

#include "Info/ReadyOrNotSignificanceManager.h"

AZipcuffs::AZipcuffs()
{
	PrimaryActorTick.TickInterval = 0.1f;
	
	ItemCategories.Add(EItemCategory::IC_Zipcuffs);
	
	bDisableTickWhenNotEquipped = false;
	bShouldTickAnimBPWhenNotEquipped = true;
}

void AZipcuffs::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZipcuffs, bArresting);
}

bool AZipcuffs::IsBlockingAnimationPlaying(TArray<EBlockingAnimationExclusion> Exclusions) const
{
	AReadyOrNotCharacter* pc = GetOwnerCharacter();
	if (!pc)
	{
		return false;
	}

	for (int32 i = 0; i < PvPArrestInteraction.Num(); i++)
	{
		if (PvPArrestInteraction[i] && PvPArrestInteraction[i]->IsPairedInteractionPlayingOn(pc))
		{
			return true;
		}
	}

	for (int32 i = 0; i < StandingArrestInteractionCivilians.Num(); i++)
	{
		if (StandingArrestInteractionCivilians[i] && StandingArrestInteractionCivilians[i]->IsPairedInteractionPlayingOn(pc))
		{
			return true;
		}
	}

	for (int32 i = 0; i < StandingArrestInteractionSuspects.Num(); i++)
	{
		if (StandingArrestInteractionSuspects[i] && StandingArrestInteractionSuspects[i]->IsPairedInteractionPlayingOn(pc))
		{
			return true;
		}
	}
	return Super::IsBlockingAnimationPlaying(Exclusions);
}

void AZipcuffs::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	bArresting ? ArrestTimer += DeltaSeconds : ArrestTimer = 0.0f;

	if (bDoingArrest)
	{
		ArrestCurrentTime += DeltaSeconds;
		
		if (ArrestCurrentTime >= DoingArrestTime)
		{
			Server_ArrestComplete();
		}
	}
	else
	{
		ArrestCurrentTime = 0.0f;
	}
}


void AZipcuffs::OnItemSecondaryUsed()
{
	// fixes a bug where instantly canceling arrest puts them in arrest state, unsure of cause but this hotfixes it.
	if (ArrestTimer < 0.25f)
		return;

	if (bArresting)
	{
		Server_ArrestCancelled();
		Server_ArrestCancelled_Implementation();
		Super::OnItemSecondaryUsed();
	}
}

void AZipcuffs::Server_ArrestStart_Implementation(AReadyOrNotCharacter* TargetedChar)
{
	AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
		return;

	if (!TargetedChar)
		return;
	
	if (bArresting)
	{
		if (!UInteractionsData::IsPairedInteractionPlayingOn(TargetedChar))
		{
			Server_ArrestComplete();
		}
		
		return;
	}

	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(GetOwner());
	UReadyOrNotSignificanceManager::UnregisterActorWithSignificanceManager(TargetedChar);

	bArresting = true;
	
	if (!TargetedChar->IsBeingArrested() && !TargetedChar->IsArrested())
	{
		TargetedChar->Arrest(OwnerCharacter);
		
		TargetedCharacter = TargetedChar;
		TargetedCharacter->OnPlayerArrestStart.Broadcast(TargetedCharacter, OwnerCharacter);
		
		OwnerCharacter->LockAllActions();

		// Special case for arresting ragdoll characters
		if (TargetedChar->IsDeadOrUnconscious() || TargetedChar->IsIncapacitated() || TargetedChar->IsInRagdoll())
		{
			UInteractionsData* BestArrestInteraction;

			const float UpDot = FVector::DotProduct(UKismetMathLibrary::GetRightVector(TargetedChar->GetMesh()->GetSocketRotation("pelvis")), FVector::UpVector);

			if (UpDot >= 0.0f)
			{
				BestArrestInteraction = ArrestRagdoll_Up;
				TargetedChar->bArrestedAsRagdoll_Flipped = true;
			}
			else
			{
				BestArrestInteraction = ArrestRagdoll_Down;
				TargetedChar->bArrestedAsRagdoll_Flipped = false;
			}

			TargetedChar->bArrestedAsRagdoll = true;

			if (BestArrestInteraction)
			{
				if (TargetedChar->IsIncapacitated())
				{
					TargetedChar->bBlendInIncapacitation = false;
					TargetedChar->bStartBlendInIncapacitation = false;
					TargetedChar->IncapacitationLoopAnim = nullptr;
				}
				
				TargetedChar->bPlayingDeathMontage = false;
				TargetedChar->bStartedPlayingDeath = false;
				
				TargetedChar->DisablePhysicalAnimation();
	
				const FRotator NewRotation = UKismetMathLibrary::GetForwardVector(TargetedChar->GetMesh()->GetSocketRotation("pelvis")).Rotation();
				TargetedChar->SetActorRotation(FRotator(TargetedChar->GetActorRotation().Pitch, NewRotation.Yaw, TargetedChar->GetActorRotation().Roll), ETeleportType::ResetPhysics);

				TargetedChar->Multicast_SavePoseSnapshot("RagdollPoseEnd");
				TargetedChar->Multicast_SavePoseSnapshot_Implementation("RagdollPoseEnd");
				
				if (APairedInteractionDriver* RagdollArrestInteractionDriver = BestArrestInteraction->PlayPairedInteraction(OwnerCharacter, TargetedChar))
				{
					RagdollArrestInteractionDriver->Event_OnPairedInteractionStarted.AddDynamic(this, &AZipcuffs::OnRagdollArrestInteractionStarted);
					RagdollArrestInteractionDriver->Event_OnDriverInteractionFinished.AddDynamic(this, &AZipcuffs::OnRagdollArrestComplete_Driver);
					RagdollArrestInteractionDriver->Event_OnSlaveInteractionFinished.AddDynamic(this, &AZipcuffs::OnRagdollArrestComplete_Slave);

					if (BestArrestInteraction->DriverMontage_FP)
					{
						DoingArrestTime = BestArrestInteraction->DriverMontage_FP->GetPlayLength() - (BestArrestInteraction->DriverMontage_FP->GetDefaultBlendOutTime() + 0.05f);
						bDoingArrest = true;
					}
				}
			}

			return;
		}

		//const bool bIsObjective = UBpGameplayHelperLib::IsObjectiveTarget(TargetedChar, OwnerCharacter);

		FString VO = VO_SWAT_GENERAL::CALL_ARRESTING_SUSPECT;
		if (TargetedChar->IsCivilian())
			VO = VO_SWAT_GENERAL::CALL_ARRESTING_CIVILIAN;
		
		OwnerCharacter->PlayRawVO(VO);

		// Lookup arresting interaction
		if (PvPArrestInteraction.Num() > 0 &&
			(OwnerCharacter->GetTeam() == ETeamType::TT_SERT_RED || OwnerCharacter->GetTeam() == ETeamType::TT_SERT_BLUE) &&
			(TargetedChar->GetTeam() == ETeamType::TT_SERT_RED || TargetedChar->GetTeam() == ETeamType::TT_SERT_BLUE))
		{
			const int32 ArrestAnimAmount = PvPArrestInteraction.Num();
			const int32 CurArrestAnimIndex = FMath::RandRange(0, ArrestAnimAmount - 1);

			if (PvPArrestInteraction.IsValidIndex(CurArrestAnimIndex))
				PvPArrestInteraction[CurArrestAnimIndex]->PlayPairedInteraction(OwnerCharacter, TargetedChar, this);
		}
		else if (TargetedChar->IsSuspect() && StandingArrestInteractionSuspects.Num() > 0)
		{
			// Randomly choose a arrest entry
			const int32 ArrestAnimAmount = StandingArrestInteractionSuspects.Num();
			int32 CurArrestAnimIndex = FMath::RandRange(0, ArrestAnimAmount - 1);

			if (ForcedInteractionData)
			{
				CurArrestAnimIndex = StandingArrestInteractionSuspects.Find(ForcedInteractionData);
			}

			if (StandingArrestInteractionSuspects.IsValidIndex(CurArrestAnimIndex))
			{
				if (UInteractionsData* SuspectArrestData = StandingArrestInteractionSuspects[CurArrestAnimIndex])
				{
					SuspectArrestData->PlayPairedInteraction(OwnerCharacter, TargetedChar, this);

					#if WITH_EDITOR
					ensure(SuspectArrestData->DriverMontage_FP);
					#endif

					if (SuspectArrestData->DriverMontage_FP)
					{
						DoingArrestTime = SuspectArrestData->DriverMontage_FP->GetPlayLength();
						bDoingArrest = true;
					}
				}
			}
		}
		else if (TargetedChar->IsCivilian() && StandingArrestInteractionCivilians.Num() > 0)
		{
			// Randomly choose a arrest entry
			const int32 ArrestAnimAmount = StandingArrestInteractionCivilians.Num();
			int32 CurArrestAnimIndex = FMath::RandRange(0, ArrestAnimAmount - 1);

			if (ForcedInteractionData)
			{
				CurArrestAnimIndex = StandingArrestInteractionCivilians.Find(ForcedInteractionData);
			}

			if (StandingArrestInteractionSuspects.IsValidIndex(CurArrestAnimIndex))
			{
				if (UInteractionsData* CivilianArrestData = StandingArrestInteractionCivilians[CurArrestAnimIndex])
				{
					CivilianArrestData->PlayPairedInteraction(OwnerCharacter, TargetedChar, this);

					#if WITH_EDITOR
					ensure(CivilianArrestData->DriverMontage_FP);
					#endif

					if (CivilianArrestData->DriverMontage_FP)
					{
						DoingArrestTime = CivilianArrestData->DriverMontage_FP->GetPlayLength();
						bDoingArrest = true;
					}
				}
			}
		}
	}
	else
	{
		OwnerCharacter->UnlockAllActions();
	}
}

void AZipcuffs::Server_ArrestComplete_Implementation()
{
	AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter();
	if (OwnerCharacter)
	{
		OwnerCharacter->UnlockAllActions();
	}

	bArresting = false;
	bDoingArrest = false;
	DoingArrestTime = 0.0f;
	
	if (TargetedCharacter)
	{
		TargetedCharacter->bIsBeingArrested = false;
		TargetedCharacter->ArrestComplete(OwnerCharacter, this);
		
		TargetedCharacter->OnPlayerArrested.Broadcast(TargetedCharacter, OwnerCharacter);
		
		if (TargetedCharacter->IsPlayerControlled())
		{
			TargetedCharacter->UnlockAllActions();			
		}
		
		if (OwnerCharacter)
		{
			OwnerCharacter->PendingAutoReport = TargetedCharacter;
			
			if (TargetedCharacter->IsSuspect())
				OwnerCharacter->PlayRawVO(VO_SWAT_GENERAL::CALL_ARRESTING_SUSPECT_COMPLETE, "", false);
			else
				OwnerCharacter->PlayRawVO(VO_SWAT_GENERAL::CALL_ARRESTING_CIVILIAN_COMPLETE, "", false);
		}
	}

	TargetedCharacter = nullptr;
}

void AZipcuffs::Server_ArrestCancelled_Implementation()
{
	bArresting = false;

	AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
		return;
	
	if (TargetedCharacter)
	{
		TargetedCharacter->CancelArrest(OwnerCharacter);
		TargetedCharacter->OnPlayerArrestedCanceled.Broadcast(TargetedCharacter, Cast<APlayerCharacter>(GetOwner()));

		TargetedCharacter->StopAnimMontage();
	}

	OwnerCharacter->StopAnimMontage();
	OwnerCharacter->StopFPAnimMontage();
	OwnerCharacter->UnlockAllActions();
	bDoingArrest = false;
    DoingArrestTime = 0.0f;

	StopFPMontage();
	StopTPMontage();

	TargetedCharacter = nullptr;
}

void AZipcuffs::Client_ArrestComplete_Implementation()
{
}

void AZipcuffs::Multicast_OnRagdollArrestStart_Implementation(AReadyOrNotCharacter* ArrestTarget)
{
	ArrestTarget->StopDeathAnimation();

	ArrestTarget->DisableRagdoll();
	
	if (ArrestTarget->IsIncapacitated())
	{
		ArrestTarget->bBlendInIncapacitation = false;
		ArrestTarget->bStartBlendInIncapacitation = false;
		ArrestTarget->IncapacitationLoopAnim = nullptr;
		ArrestTarget->bPlayingDeathMontage = false;
		ArrestTarget->bStartedPlayingDeath = false;
	}
	
	ArrestTarget->DisablePhysicalAnimation();
}

void AZipcuffs::StunnedWhileEquipped_Implementation()
{
	if (bArresting && GetLocalRole() >= ROLE_Authority)
	{
		Server_ArrestCancelled_Implementation();
	}
}

bool AZipcuffs::PlayDraw(bool bDrawFirst)
{
	if (PendingArrestCharacter)
	{
		APlayerCharacter* owner = Cast<APlayerCharacter>(GetOwner());
		if (!owner)
			return false;

		if (!owner->bHoldingUse)
			return false;

		APlayerCharacter* testChar = PendingArrestCharacter;
		if (testChar)
		{
			bool bFriendly = UBpGameplayHelperLib::IsFriendly(GetWorld()->GetGameState<AReadyOrNotGameState>(), owner->GetTeam(), testChar->GetTeam());
			if (!testChar->IsBeingArrested() && !testChar->IsArrested() && (!bFriendly || testChar->GetTeam() == ETeamType::TT_CIVILIAN) && (testChar->IsStunned() || testChar->IsSurrendered()))
			{
				owner->LockAllActions();
				if (GetLocalRole() < ROLE_Authority)
					Server_ArrestStart(testChar);
				else
					Server_ArrestStart_Implementation(testChar);
			}
		}

		return false;
	}
	
	return Super::PlayDraw(bDrawFirst);
}

void AZipcuffs::OnRagdollArrestInteractionStarted()
{
	if (TargetedCharacter)
	{
		Multicast_OnRagdollArrestStart(TargetedCharacter);
		Multicast_OnRagdollArrestStart_Implementation(TargetedCharacter);
	}
}

void AZipcuffs::OnRagdollArrestComplete_Driver(AActor* Driver)
{
	if (AReadyOrNotCharacter* DriverCharacter = Cast<AReadyOrNotCharacter>(Driver))
	{
		DriverCharacter->MoveIgnoreActorRemove(TargetedCharacter);
		DriverCharacter->UnlockAllActions();
	}
}

void AZipcuffs::OnRagdollArrestComplete_Slave(AActor* Slave)
{
	Server_ArrestComplete();
}

void AZipcuffs::OnThrownFromInventory(AReadyOrNotCharacter* Thrower, bool bMarkAsEvidence)
{
	if (Thrower->CurrentlyArresting)
	{
		if (bArresting)
		{
			Server_ArrestCancelled_Implementation();
		}
	}

	Super::OnThrownFromInventory(Thrower, bMarkAsEvidence);
}

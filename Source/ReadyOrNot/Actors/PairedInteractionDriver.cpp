// Void Interactive, 2020

#include "PairedInteractionDriver.h"

#include "Door.h"
#include "AMRagdoll/Components/RagdollComponent.h"

APairedInteractionDriver::APairedInteractionDriver()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APairedInteractionDriver::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (bInteractionRunning)
	{
		TickInteraction(DeltaTime);
	}
}

void APairedInteractionDriver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	AReadyOrNotCharacter* DriverCharacter = Cast<AReadyOrNotCharacter>(Driver);
	if (DriverCharacter)
	{
		DriverCharacter->UnlockAllActions();
	}
	
	AReadyOrNotCharacter* SlaveCharacter = Cast<AReadyOrNotCharacter>(Slave);
	if (SlaveCharacter)
	{
		SlaveCharacter->UnlockAllActions();
	}

	GetWorld()->GetGameState<AReadyOrNotGameState>()->AllPairedInteractionActors.Remove(this);
}

void APairedInteractionDriver::BeginInteraction()
{
	if (!InteractionData)
	{
		bInteractionRunning = false;
		GetWorld()->DestroyActor(this);
		return;
	}

	if (!IsValid(Driver) || !IsValid(Slave))
	{
		bInteractionRunning = false;
		GetWorld()->DestroyActor(this);
		return;
	}
	
	V_LOGM(LogReadyOrNot, "Playing Interaction %s!", *GetName());
	
	// disable collision between driver and slave
	AReadyOrNotCharacter* DriverCharacter = Cast<AReadyOrNotCharacter>(Driver);
	if (DriverCharacter)
	{
		DriverCharacter->MoveIgnoreActorAdd(Slave);

		if (ShouldHolsterItem() && DriverCharacter->GetEquippedItem())
		{
			DriverCharacter->GetInventoryComponent()->OnItemHolstered.RemoveAll(this);
			DriverCharacter->GetInventoryComponent()->OnItemHolstered.AddDynamic(this, &APairedInteractionDriver::OnEquippedItemHolstered);
			
			if (!DriverCharacter->GetInventoryComponent()->HolsterEquippedItem(InteractionData->bInstantHolster))
			{
				bCanPlayMontagesNow = true;
			}
		}
		else
		{
			bCanPlayMontagesNow = true;
		}
	}
	else
	{
		bCanPlayMontagesNow = true;
	}
	
	AReadyOrNotCharacter* SlaveCharacter = Cast<AReadyOrNotCharacter>(Slave);
	if (SlaveCharacter)
	{
		SlaveCharacter->MoveIgnoreActorAdd(Driver);
	}

	if (!InteractionData->bUpdateSlaveTransform)
	{
		bInteractionRunning = bCanPlayMontagesNow;
		return;
	}
	
	SlaveOriginalWorldPos = Slave->GetActorLocation();
	SlaveOriginalWorldRot = Slave->GetActorRotation();
	DriverWorldPos = Driver->GetActorLocation();
	DriverWorldRot = Driver->GetActorRotation();

	DriverForward.Normalize();
	DriverRight.Normalize();
	DriverUp.Normalize();

	// we always want it to face the driver
	SlaveFinalRot = DriverWorldRot + InteractionData->RelativeRotOffsetToDriver;

	bInteractionRunning = bCanPlayMontagesNow;
}

void APairedInteractionDriver::TickInteraction(const float DeltaTime)
{
	// One of them is not valid, stop running interaction
	if (!IsValid(Driver) || !IsValid(Slave))
	{
		bInteractionRunning = false;
		GetWorld()->DestroyActor(this);
		return;
	}

	AReadyOrNotCharacter* DriverCharacter = Cast<AReadyOrNotCharacter>(Driver);
	AReadyOrNotCharacter* SlaveCharacter = Cast<AReadyOrNotCharacter>(Slave);

	if ((!InteractionData->bAllowDeadDriverInteraction && (DriverCharacter && DriverCharacter->IsDeadOrUnconscious())) ||
		(!InteractionData->bAllowDeadSlaveInteraction && (SlaveCharacter && SlaveCharacter->IsDeadOrUnconscious())))
	{
		if (DriverCharacter)
		{
			DriverCharacter->UnlockAllActions();
		}
		
		if (SlaveCharacter)
		{
			SlaveCharacter->UnlockAllActions();
		}
		
		bInteractionRunning = false;
		GetWorld()->DestroyActor(this);
		return;
	}

	const bool bHasDriverAnimation = InteractionData->DriverMontage || InteractionData->DriverMontage_FP;
	const bool bHasSlaveAnimation = InteractionData->SlaveMontage || InteractionData->SlaveMontage_FP;

	const bool bHasFinishedDriverAnimation = (bEverRanDriverInteractionFinished && bHasDriverAnimation) || !bHasDriverAnimation;
	const bool bHasFinishedSlaveAnimation = (bEverRanSlaveInteractionFinished && bHasSlaveAnimation) || !bHasSlaveAnimation;

	if (bHasFinishedDriverAnimation && bHasFinishedSlaveAnimation)
	{
		OnInteractionFinished();
		return;
	}

	if (DriverCharacter)
	{
		DriverCharacter->LockAllActions();
	}

	if (SlaveCharacter)
	{
		SlaveCharacter->LockAllActions();
	}

	if (!Cast<ADoor>(Slave) && !Cast<ADoor>(Driver))
	{
		if (InteractionData->bUseSyncBone && !bEverRanSlaveInteractionFinished && !SlaveCharacter->IsDeadOrUnconscious())
		{
			if (const UAnimSequence* AnimSequence = Cast<const UAnimSequence>(InteractionData->DriverMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].AnimReference))
			{
				const float MontagePosition = DriverCharacter->GetMesh()->GetAnimInstance()->Montage_GetPosition(InteractionData->DriverMontage);
				
				if (!FMath::IsNearlyZero(MontagePosition, 0.00001f))
				{
					constexpr int32 SyncBoneIndex = 133;
					
					FTransform PoseTransform;
					AnimSequence->GetBoneTransform(PoseTransform, SyncBoneIndex, MontagePosition, false);

					PoseTransform.SetLocation(PoseTransform.GetLocation().RotateAngleAxis(-90.0f, FVector::UpVector));

					const FVector SyncBoneLocation = DriverCharacter->GetActorTransform().TransformPosition(PoseTransform.GetLocation());

					DrawDebugSphere(GetWorld(), SyncBoneLocation, 5.0f, 16, FColor::Magenta, false, 0.033f);
					
					constexpr float InterpSpeed = 3.0f;
					
					const FVector SmoothedLocation = UKismetMathLibrary::VInterpTo(Slave->GetActorLocation(), SyncBoneLocation, DeltaTime, InterpSpeed);
					const FRotator SmoothedRotation = UKismetMathLibrary::RInterpTo(Slave->GetActorRotation(), DriverCharacter->GetActorRotation(), DeltaTime, InterpSpeed);

					SlaveCharacter->SetActorLocationAndRotation(SmoothedLocation, SmoothedRotation, InteractionData->bSweepEnvironment, nullptr, ETeleportType::TeleportPhysics);
				}
			}
		}
		else
		{
			// set the slave where they need to be
			if (!InteractionData->bDoNotApplyRelativeOffset ||
				(InteractionData->bApplyRelativeOffsetBeforePlaying && !bHasPlayedMontages))
			{
				DriverForward = Driver->GetActorForwardVector();
				DriverRight = Driver->GetActorRightVector();
				DriverUp = Driver->GetActorUpVector();

				// Move the slave into the driver until the interaction offset matches the new offset
				FTransform DriverTransform;
				DriverTransform.SetRotation(DriverWorldRot.Quaternion());
				DriverTransform.SetLocation(DriverWorldPos);
				
				RelativeOffset = InteractionData->RelativePosOffsetToDriver;
				SlaveFinalPos = DriverTransform.TransformPosition(FVector(RelativeOffset.Y, RelativeOffset.X, 0.0f));
				SlaveFinalPos.Z = Slave->GetActorLocation().Z;

				if (UNLIKELY(InteractionData->bUpdateTransformsInstantly))
				{
					Slave->SetActorLocation(SlaveFinalPos);
					Slave->SetActorRotation(SlaveFinalRot);

					if (InteractionData->DriverMontage)
					{
						Driver->SetActorLocation(DriverWorldPos);
						Driver->SetActorRotation(DriverWorldRot);
					}
				}
				else
				{
					constexpr float InterpSpeed = 3.0f;
					
					Slave->SetActorLocation(UKismetMathLibrary::VInterpTo(Slave->GetActorLocation(), SlaveFinalPos, DeltaTime, InterpSpeed));
					Slave->SetActorRotation(UKismetMathLibrary::RInterpTo(Slave->GetActorRotation(), SlaveFinalRot, DeltaTime, InterpSpeed));

					if (InteractionData->DriverMontage)
					{
						Driver->SetActorLocation(UKismetMathLibrary::VInterpTo(Driver->GetActorLocation(), DriverWorldPos, DeltaTime, InterpSpeed));
						Driver->SetActorRotation(UKismetMathLibrary::RInterpTo(Driver->GetActorRotation(), DriverWorldRot, DeltaTime, InterpSpeed));
					}
				}

				//#if !UE_BUILD_SHIPPING
				//DrawDebugLine(GetWorld(), Slave->GetActorLocation(), SlaveFinalPos, FColor::Blue, false, 0.033f, 0, 1.5f);
				//DrawDebugLine(GetWorld(), Driver->GetActorLocation(), DriverWorldPos, FColor::Green, false, 0.033f, 0, 1.5f);

				//DrawDebugLine(GetWorld(), Slave->GetActorLocation(), Slave->GetActorLocation() + Slave->GetActorRotation().Vector() * 100.0f, FColor::Orange, false, 0.033f, 0, 1.5f);
				//DrawDebugLine(GetWorld(), Slave->GetActorLocation(), Slave->GetActorLocation() + SlaveFinalRot.Vector() * 100.0f, FColor::Blue, false, 0.033f, 0, 1.5f);

				//ULog::Info("Driver Loc Real: " + Driver->GetActorLocation().ToString() + " | Driver Loc Final: " + DriverWorldPos.ToString());
				//ULog::Info("Driver Rot Real: " + Driver->GetActorRotation().ToString() + " | Driver Rot Final: " + DriverWorldRot.ToString());
				//ULog::Info("Slave Loc Real: " + Slave->GetActorLocation().ToString() + " | Slave Loc Final: " + SlaveFinalPos.ToString());
				//ULog::Info("Slave Rot Real: " + Slave->GetActorRotation().ToString() + " | Slave Rot Final: " + SlaveFinalRot.ToString());
				//#endif
			}
		}
	}

	if (!bHasPlayedMontages && bCanPlayMontagesNow)
	{
		if (GetLocalRole() == ROLE_Authority)
		{
			if (!InteractionData->bApplyRelativeOffsetBeforePlaying ||
				(InteractionData->bApplyRelativeOffsetBeforePlaying && IsInPositionToPlayInteraction()))
			{
				bHasPlayedMontages = true;

				if (SlaveCharacter->IsInRagdoll())
				{
					SlaveCharacter->DisableRagdoll();
					SlaveCharacter->DisablePhysicalAnimation();
					SlaveCharacter->GetRagdollComponent()->DisableRagdoll();
					SlaveCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				}
				
				Event_OnPairedInteractionStarted.Broadcast();

				// play animation on targeted character and owner	
				/* see if we want to play the variant that provides a special fp tailored interaction */
				if (SlaveCharacter)
				{
					if (InteractionData->bUseSlaveFPMotion)
					{
						SlaveCharacter->Play1PMontage(InteractionData->SlaveMontage_FP);
					}

					SlaveCharacter->Play3PMontage(InteractionData->SlaveMontage);
					ULog::Info("Playing " + GetNameSafe(InteractionData->SlaveMontage) + " on Slave");
				}

				if (DriverCharacter)
				{
					if (InteractionData->bUseDriverFPMotion)
					{
						DriverCharacter->Play1PMontage(InteractionData->DriverMontage_FP);
					}

					DriverCharacter->Play3PMontage(InteractionData->DriverMontage);
					ULog::Info("Playing " + GetNameSafe(InteractionData->DriverMontage) + " on Driver");
				}

				if (InteractionData->bHasSharedItemAnim && OptionalItem)
				{
					OptionalItem->Client_PlayFPMontage(InteractionData->SharedItemMontage);
					OptionalItem->Multicast_PlayTPMontage(InteractionData->SharedItemMontage);
				}

				// Start timers for events
				if (!InteractionData->bLooping)
				{
					if (InteractionData->CancelDurationLength > 0.0f)
					{
						GetWorld()->GetTimerManager().SetTimer(OnAnimationFinished_Handle, this, &APairedInteractionDriver::OnInteractionFinished, InteractionData->CancelDurationLength);
					}
					else
					{
						// Setup driver/slave finished events
						{
							if (InteractionData->DriverMontage_FP && Cast<APlayerCharacter>(Driver))
							{
								GetWorld()->GetTimerManager().SetTimer(OnDriverAnimationFinished_Handle, this, &APairedInteractionDriver::OnDriverInteractionFinished, InteractionData->DriverMontage_FP->GetPlayLength() - (InteractionData->DriverMontage_FP->GetDefaultBlendOutTime() + 0.01f));
							}
							else if (InteractionData->DriverMontage)
							{
								GetWorld()->GetTimerManager().SetTimer(OnDriverAnimationFinished_Handle, this, &APairedInteractionDriver::OnDriverInteractionFinished, InteractionData->DriverMontage->GetPlayLength() - (InteractionData->DriverMontage->GetDefaultBlendOutTime() + 0.01f));
							}
							
							if (InteractionData->SlaveMontage_FP && Cast<APlayerCharacter>(Slave))
							{
								GetWorld()->GetTimerManager().SetTimer(OnSlaveAnimationFinished_Handle, this, &APairedInteractionDriver::OnSlaveInteractionFinished, InteractionData->SlaveMontage_FP->GetPlayLength() - (InteractionData->SlaveMontage_FP->GetDefaultBlendOutTime() + 0.01f));
							}
							else if (InteractionData->SlaveMontage)
							{
								GetWorld()->GetTimerManager().SetTimer(OnSlaveAnimationFinished_Handle, this, &APairedInteractionDriver::OnSlaveInteractionFinished, InteractionData->SlaveMontage->GetPlayLength() - (InteractionData->SlaveMontage->GetDefaultBlendOutTime() + 0.01f));
							}
						}
						
						// Setup interaction finished event, prioritizing FP animations
						if (!InteractionData->bIndependentFinishes)
						{
							if (InteractionData->DriverMontage_FP && Cast<APlayerCharacter>(Driver))
							{
								GetWorld()->GetTimerManager().SetTimer(OnAnimationFinished_Handle, this, &APairedInteractionDriver::OnInteractionFinished, InteractionData->DriverMontage_FP->GetPlayLength() - (InteractionData->DriverMontage_FP->GetDefaultBlendOutTime() + 0.01f));
							}
							else if (InteractionData->SlaveMontage_FP && Cast<APlayerCharacter>(Slave))
							{
								GetWorld()->GetTimerManager().SetTimer(OnAnimationFinished_Handle, this, &APairedInteractionDriver::OnInteractionFinished, InteractionData->SlaveMontage_FP->GetPlayLength() - (InteractionData->SlaveMontage_FP->GetDefaultBlendOutTime() + 0.01f));
							}
							else if (InteractionData->DriverMontage)
							{
								GetWorld()->GetTimerManager().SetTimer(OnAnimationFinished_Handle, this, &APairedInteractionDriver::OnInteractionFinished, InteractionData->DriverMontage->GetPlayLength() - (InteractionData->DriverMontage->GetDefaultBlendOutTime() + 0.01f));
							}
							else if (InteractionData->SlaveMontage)
							{
								GetWorld()->GetTimerManager().SetTimer(OnAnimationFinished_Handle, this, &APairedInteractionDriver::OnInteractionFinished, InteractionData->SlaveMontage->GetPlayLength() - (InteractionData->SlaveMontage->GetDefaultBlendOutTime() + 0.01f));
							}
							else
							{
								OnInteractionFinished();
							}
						}
					}
				}
			}
		}
	}
}

bool APairedInteractionDriver::IsPlayingAnimationForCharacter(AReadyOrNotCharacter* TestCharacter, UAnimMontage* Montage)
{
	if (!TestCharacter)
		return false;

	if (!bInteractionRunning)
		return false;

	//V_LOGM(LogReadyOrNot, "Paired Interaction Driver Running on %s [trying to play animation on %s]", *Driver->GetName(), *TestCharacter->GetName());

	if (TestCharacter == Driver && Montage != InteractionData->DriverMontage && !bEverRanDriverInteractionFinished)
	{
		V_LOGM(LogReadyOrNot, "Aborting animation attempt due to %s being the driver", *TestCharacter->GetName());
		return true;
	}

	if (TestCharacter == Slave && Montage != InteractionData->SlaveMontage && !bEverRanSlaveInteractionFinished)
	{
		V_LOGM(LogReadyOrNot, "Aborting animation attempt due to %s being the slave", *TestCharacter->GetName());
		return true;
	}
	
	return false;
}

bool APairedInteractionDriver::IsDriver(AActor* TestActor)
{
	return Driver == TestActor;
}

bool APairedInteractionDriver::IsSlave(AActor* TestActor)
{
	return Slave == TestActor;
}

FVector APairedInteractionDriver::GetDriverWorldPos()
{
	return DriverWorldPos;
}

bool APairedInteractionDriver::IsInPositionToPlayInteraction() const
{
	if (!Driver || !Slave)
		return false;
	
	if (Driver && !Slave)
	{
		return	Driver->GetActorLocation().Equals(DriverWorldPos, 1.0f) &&
				Driver->GetActorRotation().Equals(DriverWorldRot, 1.0f);
	}
	
	if (!Driver && Slave)
	{
		return	Slave->GetActorLocation().Equals(SlaveFinalPos, 1.0f) &&
				Slave->GetActorRotation().Equals(SlaveFinalRot, 1.0f);
	}
	
	if (Driver && Slave)
	{
		return	FMath::IsNearlyEqual(Driver->GetActorLocation().X, DriverWorldPos.X, 1.0f) &&
				FMath::IsNearlyEqual(Driver->GetActorLocation().Y, DriverWorldPos.Y, 1.0f) &&
				Driver->GetActorRotation().Equals(DriverWorldRot, 1.0f) &&
				FMath::IsNearlyEqual(Slave->GetActorLocation().X, SlaveFinalPos.X, 1.0f) &&
				FMath::IsNearlyEqual(Slave->GetActorLocation().Y, SlaveFinalPos.Y, 1.0f) &&
				Slave->GetActorRotation().Equals(SlaveFinalRot, 1.0f);
		
		/*return	Driver->GetActorLocation().Equals(DriverWorldPos, 10.0f) &&
				Driver->GetActorRotation().Equals(DriverWorldRot, 10.0f) &&
				Slave->GetActorLocation().Equals(SlaveFinalPos, 10.0f) &&
				Slave->GetActorRotation().Equals(SlaveFinalRot, 10.0f);*/
	}

	return false;
}

bool APairedInteractionDriver::ShouldHolsterItem() const
{
	if (InteractionData)
	{
		return InteractionData->bHolsterItemBeforePlaying;
	}

	return false;
}

void APairedInteractionDriver::OnEquippedItemHolstered(ABaseItem* Item)
{
	bInteractionRunning = true;
	bCanPlayMontagesNow = true;
}

void APairedInteractionDriver::OnInteractionFinished()
{
	bInteractionRunning = false;

	OnDriverInteractionFinished();
	OnSlaveInteractionFinished();

	Event_OnPairedInteractionFinished.Broadcast(Driver, Slave);

	GetWorld()->DestroyActor(this);
}

void APairedInteractionDriver::OnDriverInteractionFinished()
{
	if (!bEverRanDriverInteractionFinished)
	{
		bEverRanDriverInteractionFinished = true;

		if (AReadyOrNotCharacter* DriverCharacter = Cast<AReadyOrNotCharacter>(Driver))
		{
			//DriverCharacter->StopTPMontage(InteractionData->DriverMontage);
			//DriverCharacter->StopFPAnimMontage(InteractionData->DriverMontage_FP);

			if (InteractionData->bEquipLastItemAfterPlaying)
			{
				DriverCharacter->GetInventoryComponent()->EquipLastEquippedWeapon(false, true);
			}
			
			APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(DriverCharacter);
			if (PlayerCharacter)
			{
				PlayerCharacter->Client_SetControlRotation(PlayerCharacter->GetFirstPersonCameraComponent()->GetComponentRotation());
			}

			if (!DriverCharacter->IsDeadOrUnconscious())
				DriverCharacter->MoveIgnoreActorRemove(Slave);
			
			DriverCharacter->UnlockAllActions();

			DriverCharacter->StopTPMontage(InteractionData->DriverMontage);
		}
		
		Event_OnDriverInteractionFinished.Broadcast(Driver);
	}
}

void APairedInteractionDriver::OnSlaveInteractionFinished()
{
	if (!bEverRanSlaveInteractionFinished)
	{
		bEverRanSlaveInteractionFinished = true;

		if (AReadyOrNotCharacter* SlaveCharacter = Cast<AReadyOrNotCharacter>(Slave))
		{
			SlaveCharacter->StopTPMontage(InteractionData->SlaveMontage, InteractionData->SlaveMontage->BlendOut.GetBlendTime());
			SlaveCharacter->StopFPAnimMontage(InteractionData->SlaveMontage_FP);

			if (!SlaveCharacter->IsDeadOrUnconscious())
				SlaveCharacter->MoveIgnoreActorRemove(Driver);
				
			SlaveCharacter->UnlockAllActions();
		}

		Event_OnSlaveInteractionFinished.Broadcast(Slave);
	}
}

APairedInteractionDriver* APairedInteractionDriver::CreateAndPlayInteraction(UWorld* World, UInteractionsData* InInteractionsData, AActor* Driver, AActor* Slave, ABaseItem* InOptionalItem)
{
	if (!World)
		return nullptr;

	if (APairedInteractionDriver* PairedInteractionDriver = World->SpawnActor<APairedInteractionDriver>(StaticClass()))
	{
		PairedInteractionDriver->InteractionData = InInteractionsData;
		PairedInteractionDriver->Driver = Driver;
		PairedInteractionDriver->Slave = Slave;
		PairedInteractionDriver->OptionalItem = InOptionalItem;
		PairedInteractionDriver->BeginInteraction();

		World->GetGameState<AReadyOrNotGameState>()->AllPairedInteractionActors.Add(PairedInteractionDriver);

		return PairedInteractionDriver;
	}

	return nullptr;
}

void APairedInteractionDriver::EndInteraction()
{
	OnInteractionFinished();
}

void APairedInteractionDriver::EndSlaveInteraction()
{
	OnSlaveInteractionFinished();
}

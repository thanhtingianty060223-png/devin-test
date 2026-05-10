// Copyright Void Interactive, 2017

#include "Quadrotor.h"
#include "ReadyOrNot.h"




AQuadrotor::AQuadrotor()
{
	ViewfinderMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Viewfinder"));
	ViewfinderMesh->SetupAttachment(ItemMesh);

	SceneCapture2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture2D"));
}

void AQuadrotor::BeginPlay()
{
	Super::BeginPlay();
}

void AQuadrotor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (SpawnedDrone)
	{
		GetItemMesh()->SetVisibility(false);
	}
}

void AQuadrotor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AQuadrotor, SpawnedDrone);
}

void AQuadrotor::OnItemPrimaryUse()
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (pc && !SpawnedDrone)
	{
#ifdef USE_NEW_ANIMSYSTEM

#else
		pc->Play1PMontage(ThrowDrone_1P);
		pc->Play3PMontage(ThrowDrone_3P);
#endif
		Super::OnItemPrimaryUse();
	}
}

void AQuadrotor::OnItemSecondaryUsed()
{
	bToggleDroneControl = !bToggleDroneControl;
	if (bToggleDroneControl)
	{
		APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
		if (pc)
		{
			pc->Crouch();
		}
		Super::OnItemSecondaryUsed();
	}
}

void AQuadrotor::OnItemUseComplete()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		Server_SpawnDrone();
	}
	else
	{
		Server_SpawnDrone_Implementation();
	}
}

bool AQuadrotor::ConsumeLeanInput(float leanAmount)
{
	if (bToggleDroneControl && SpawnedDrone)
	{
		return true;
	}
	return false;
}

bool AQuadrotor::ConsumeMovementForward(float Val)
{
	if (bToggleDroneControl && SpawnedDrone)
	{
		return true;
	}
	return false;
}

bool AQuadrotor::ConsumeMovementRight(float Val)
{
	if (bToggleDroneControl && SpawnedDrone)
	{
		return true;
	}
	return false;
}

bool AQuadrotor::ConsumeMouseMovement(FRotator RotateVector)
{
	if (bToggleDroneControl && SpawnedDrone)
	{
		SpawnedDrone->TargetRotation += RotateVector;
		return true;
	}
	return false;
}

bool AQuadrotor::ConsumeCrouchInput()
{
	if (bToggleDroneControl && SpawnedDrone)
	{
		return true;
	}
	return false;

}

bool AQuadrotor::ConsumeSprintInput()
{
	if (bToggleDroneControl && SpawnedDrone)
	{
		return true;
	}
	return false;
}

bool AQuadrotor::ConsumeJumpInput()
{
	if (bToggleDroneControl && SpawnedDrone)
	{
		return true;
	}
	return false;
}

bool AQuadrotor::PlayDraw(bool bDrawFirst)
{
	if (!SpawnedDrone)
	{
		ToggleDroneCapture(false);
	}
	else
	{
		ToggleDroneCapture(true);
	}

	return Super::PlayDraw(bDrawFirst);
}

void AQuadrotor::Server_SpawnDrone_Implementation()
{
	if (!SpawnedDrone)
	{
		SpawnedDrone = GetWorld()->SpawnActor<class AQuadrotorPawn>(DronePawnClass, GetItemMesh()->GetComponentTransform());
		if (SpawnedDrone)
		{
			SpawnedDrone->SetOwner(GetOwner());
			SpawnedDrone->GetDroneMesh()->SetWorldScale3D(FVector(1));
			SpawnedDrone->SetActorLocation(GetItemMesh()->GetComponentLocation() + GetOwner()->GetActorForwardVector() * 50);
			SpawnedDrone->SetActorRotation(FRotator(0));
			SpawnedDrone->DroneTransform = SpawnedDrone->GetActorTransform();
			SceneCapture2D->AttachToComponent(SpawnedDrone->GetDroneMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "Camera");
			bToggleDroneControl = true;
			ToggleDroneCapture(true);
		}
	}
}

void AQuadrotor::ToggleDroneCapture(bool bCapture)
{
	bScreenOn = bCapture;
	if (bCapture)
	{
		//HideActorsForDrone();
		RenderTarget = nullptr;
		RenderTarget = NewObject<UTextureRenderTarget2D>();
		RenderTarget->AddressX = TA_Wrap;
		RenderTarget->AddressY = TA_Wrap;

		if (!ViewfinderScreenMaterial || !ViewfinderScreenMaterial->IsValidLowLevel())
			ViewfinderScreenMaterial = ViewfinderMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(1, ViewfinderMesh->GetMaterial(1));

		// If we are the local player we will give us a higher res version of the optiwand capture, 
		//if we are viewing it from another perspective give us a low res version to minimize impact but keep the aesthetics..
		if (IsLocallyControlled())
		{
			RenderTarget->InitAutoFormat(LocalPlayerCaptureResolution.X, LocalPlayerCaptureResolution.Y);
		}
		else
		{
			RenderTarget->InitAutoFormat(SimulatedPlayerCaptureResolution.X, SimulatedPlayerCaptureResolution.Y);
		}

		ViewfinderMesh->SetMaterial(1, ViewfinderScreenMaterial);

		if (SceneCapture2D)
			SceneCapture2D->TextureTarget = RenderTarget;

		ViewfinderScreenMaterial->SetTextureParameterValue("ScreenTexture", RenderTarget);
		SceneCapture2D->UpdateContent();

	}
	else
	{
		SceneCapture2D->TextureTarget = nullptr;
		ViewfinderMesh->SetMaterial(1, DefaultViewfinderMaterial);
	}
}

void AQuadrotor::HideActorsForDrone()
{
	TArray<AActor*> HideActors;
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (pc)
	{
		HideActors.Add(pc);
		for (int32 i = 0; i < pc->GetInventoryComponent()->GetInventoryItems().Num(); i++)
		{
			HideActors.Add(pc->GetInventoryComponent()->GetInventoryItems()[i]);
		}
	}
	SceneCapture2D->HiddenActors = HideActors;
}

void AQuadrotor::OnRep_AttachmentRep()
{
	Super::OnRep_AttachmentRep();

	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (pc)
	{
		if (IsLocallyControlled())
		{
			if (pc->GetMesh1P())
			{
				ViewfinderMesh->AttachToComponent(pc->GetMesh1P(), FAttachmentTransformRules::SnapToTargetIncludingScale, ViewfinderSocket_Hands);
			}
		}
		else
		{
			if (pc->GetMesh())
			{
				if (IsEquipped())
				{
					ViewfinderMesh->AttachToComponent(pc->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, ViewfinderSocket_Hands);
				}
				else
				{
					ViewfinderMesh->AttachToComponent(pc->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, ViewfinderSocket_Body);
				}
			}
		}
	}
}


// QUADROTOR PAWN
void AQuadrotorPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AQuadrotorPawn, RPM);
	DOREPLIFETIME(AQuadrotorPawn, bApplyingInput);
	DOREPLIFETIME_CONDITION(AQuadrotorPawn, DroneTransform, COND_SkipOwner);
}

AQuadrotorPawn::AQuadrotorPawn()
{
	// Handle this ourselves.
	SetReplicateMovement(false);

	FlightBox = CreateDefaultSubobject<UBoxComponent>(TEXT("FlightBox"));
	SetRootComponent(FlightBox);

	DroneMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DroneMesh"));
	DroneMesh->SetupAttachment(FlightBox);

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(DroneMesh, "Camera");
}

void AQuadrotorPawn::BeginPlay()
{
	Super::BeginPlay();
}

void AQuadrotorPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!GetOwner())
	{
		bApplyingInput = false;
		RPM = FMath::FInterpTo(RPM, 0.0f, DeltaSeconds, 1.0f);
	}

	RotorRotation.Yaw += RPM * 0.0167f * 360.0f;

	class APlayerCharacter* owner = Cast<APlayerCharacter>(GetOwner());
	if (owner && !owner->IsLocalPlayer())
	{
		FlightBox->SetSimulatePhysics(false);
		FlightBox->SetWorldLocation(FMath::VInterpTo(DroneMesh->GetComponentLocation(), ClientDroneTransform.GetLocation(), DeltaSeconds, 5.0f));
		FlightBox->SetWorldRotation(FMath::RInterpTo(DroneMesh->GetComponentRotation(), ClientDroneTransform.GetRotation().Rotator(), DeltaSeconds, 5.0f));
	}
	else
	{
		if (bSteadyDrone)
		{
			FlightBox->SetWorldRotation(FMath::RInterpTo(FlightBox->GetComponentRotation(), FRotator(0, TargetRotation.Yaw, 0), DeltaSeconds, 10.0f));
		}
		else
		{
			FlightBox->SetWorldRotation(FMath::RInterpTo(FlightBox->GetComponentRotation(), FRotator(TargetRotation.Pitch, TargetRotation.Yaw, 0), DeltaSeconds, 3.0f));
		}
		FlightBox->AddForce(DroneMesh->GetUpVector() * (RPM * RPMForceScale));

		FVector DroneVel = FlightBox->GetComponentVelocity();
		DroneVel.X = FMath::Clamp(DroneVel.X, -MaxVelocity, MaxVelocity);
		DroneVel.Y = FMath::Clamp(DroneVel.Y, -MaxVelocity, MaxVelocity);
		DroneVel.Z = FMath::Clamp(DroneVel.Z, -MaxVelocity, MaxVelocity);
		FlightBox->SetPhysicsLinearVelocity(DroneVel);

		if (GetLocalRole() < ROLE_Authority && GetLocalRole() > ROLE_SimulatedProxy)
		{
			Server_UpdateDroneTransform(DroneMesh->GetComponentTransform());
		}
		else
		{
			Server_UpdateDroneTransform_Implementation(DroneMesh->GetComponentTransform());
		}
	}
}

void AQuadrotorPawn::OnPickedUp(APlayerCharacter* Interactor)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(GetOwner());
	if (pc && pc == Interactor)
	{
		AQuadrotor* Quadrotor = Cast<AQuadrotor>(pc->GetInventoryComponent()->GetInventoryItemOfClass(AQuadrotor::StaticClass()));
		if (Quadrotor)
		{
			Quadrotor->bToggleDroneControl = false;
			Quadrotor->SpawnedDrone = nullptr;
			Destroy();
		}
	}
}

void AQuadrotorPawn::Server_UpdateDroneTransform_Implementation(FTransform newTransform)
{
	DroneTransform = newTransform;
}

void AQuadrotorPawn::OnRep_DroneMovement()
{
	ClientDroneTransform = DroneTransform;
}

void AQuadrotorPawn::DroneThrottle(float Val)
{
	if (Val != 0.0f)
	{
		bApplyingInput = true;
		RPM = FMath::FInterpTo(RPM, RPM + (Val * RPMThrottleMultiplier), GetWorld()->GetDeltaSeconds(), 12.0f);
		RPM = FMath::Clamp(RPM, 0.0f, MaxRPM);
	}
	else
	{
		RPM = FMath::FInterpTo(RPM, IdleRPM, GetWorld()->GetDeltaSeconds(), 12.0f);
		bApplyingInput = false;
	}
}

void AQuadrotorPawn::DroneForward(float Val)
{
	if (Val != 0.0f)
	{
		FRotator TiltForward = FRotator::ZeroRotator;
		TiltForward.Pitch = MaximumTilt * Val * GetWorld()->GetDeltaSeconds();

		AddActorLocalRotation(TiltForward, true, (FHitResult*)nullptr, ETeleportType::TeleportPhysics);
	}
}

void AQuadrotorPawn::DroneRight(float Val)
{
	if (Val != 0.0f)
	{
		Val = Val * -1;

		FRotator TiltRight = FRotator::ZeroRotator;
		TiltRight.Roll = MaximumTilt * Val * GetWorld()->GetDeltaSeconds();

		AddActorLocalRotation(TiltRight, true, (FHitResult*)nullptr, ETeleportType::TeleportPhysics);
	}
}

void AQuadrotorPawn::DroneYaw(float Val)
{
	if (Val != 0.0f)
	{
		FRotator YawRight = FRotator::ZeroRotator;
		YawRight.Yaw = MaximumTilt * Val * GetWorld()->GetDeltaSeconds();

		TargetRotation.Yaw += YawRight.Yaw;
	}
}

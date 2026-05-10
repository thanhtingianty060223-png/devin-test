#include "DroneVehicle.h"
#include "ReadyOrNot.h"

#include "lib/BpGameplayHelperLib.h"

#include "Log.h"

ADroneVehicle::ADroneVehicle() : Super()
{
	// Handle this ourselves.
	AActor::SetReplicateMovement(true);

	FlightBox = CreateDefaultSubobject<UBoxComponent>(TEXT("FlightBox"));
	SetRootComponent(FlightBox);
	FlightBox->SetSimulatePhysics(true);
	FlightBox->OnComponentHit.AddDynamic(this, &ADroneVehicle::OnDroneHit);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DroneMesh"));
	Mesh->SetupAttachment(FlightBox);

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(Mesh, "Camera");

	ThirdPersonSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonArm"));
	ThirdPersonSpringArm->SetupAttachment(RootComponent);
	ThirdPersonSpringArm->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(ThirdPersonSpringArm, USpringArmComponent::SocketName);
	ThirdPersonCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	FloatingMovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	FloatingMovementComponent->SetIsReplicated(true);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(Mesh);
}

void ADroneVehicle::BeginPlay()
{
	Super::BeginPlay();

	World = GetWorld();

	ActorLocationLastFrame = GetActorLocation();
}

void ADroneVehicle::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!DroneOwner)
	{
		bApplyingInput = false;
		//RPM = FMath::FInterpTo(RPM, 0.0f, DeltaSeconds, 1.0f);
	}

	RotorRotation.Yaw += RPM/60 * DeltaSeconds * 360.0f;

	if (DroneOwner && !DroneOwner->IsLocalPlayer())
	{
		FlightBox->SetWorldLocation(FMath::VInterpTo(FlightBox->GetComponentLocation(), ClientDroneTransform.GetLocation(), DeltaSeconds, 5.0f));
		FlightBox->SetWorldRotation(FMath::RInterpTo(FlightBox->GetComponentRotation(), ClientDroneTransform.GetRotation().Rotator(), DeltaSeconds, bSteadyDrone ? RotationInterpSpeedWhenSteady : RotationInterpSpeed));
	}
	else
	{
		FlightBox->SetWorldRotation(FMath::RInterpTo(FlightBox->GetComponentRotation(), FRotator(TargetRotation.Pitch, TargetRotation.Yaw, 0), DeltaSeconds, bSteadyDrone ? RotationInterpSpeedWhenSteady : RotationInterpSpeed));

		if (GetLocalRole() < ROLE_Authority && GetLocalRole() > ROLE_SimulatedProxy)
		{
			Server_UpdateDroneTransform(Mesh->GetComponentTransform());
		}
		else
		{
			Server_UpdateDroneTransform_Implementation(Mesh->GetComponentTransform());
		}
	}

	UpdatePilotingInfo();

	ActorLocationLastFrame = GetActorLocation();
}

void ADroneVehicle::SetupPlayerInputComponent(class UInputComponent* inputComponent)
{
	check(inputComponent);

	inputComponent->BindAxis("LookUp", this, &ADroneVehicle::AddControllerPitchInput);
	inputComponent->BindAxis("LookUpRate", this, &ADroneVehicle::Drone_LookUpAtRate);
	inputComponent->BindAxis("Turn", this, &ADroneVehicle::AddControllerYawInput);
	inputComponent->BindAxis("TurnRate", this, &ADroneVehicle::Drone_TurnAtRate);

	inputComponent->BindAxis("Drone_MoveForward", this, &ADroneVehicle::Drone_MoveForward);
	inputComponent->BindAxis("Drone_Right", this, &ADroneVehicle::Drone_Right);
	inputComponent->BindAxis("Drone_Throttle", this, &ADroneVehicle::Drone_Throttle);
	inputComponent->BindAxis("Drone_Yaw", this, &ADroneVehicle::Drone_Yaw);

	inputComponent->BindAction("Drone_Steady", IE_Pressed, this, &ADroneVehicle::Drone_Steady);
	inputComponent->BindAction("Drone_ThirdPerson", IE_Pressed, this, &ADroneVehicle::Drone_ToggleThirdPerson);
	inputComponent->BindAction("Drone_QuickTurn", IE_DoubleClick, this, &ADroneVehicle::Drone_QuickTurn);
	inputComponent->BindAction("Drone_Exit", IE_Released, this, &ADroneVehicle::Drone_Exit);

	inputComponent->BindAxis("InrementalSystemAxis", this, &ADroneVehicle::IncrementSpeed);
}

void ADroneVehicle::AddControllerYawInput(const float Val)
{
	Super::AddControllerYawInput(Val);

	ThirdPersonSpringArm->AddRelativeRotation(FRotator(0.0f, Val, 0.0f));
}

void ADroneVehicle::AddControllerPitchInput(const float Val)
{
	Super::AddControllerPitchInput(Val);

	ThirdPersonSpringArm->AddRelativeRotation(FRotator(-Val, 0.0f, 0.0f));
}

void ADroneVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADroneVehicle, RPM);
	DOREPLIFETIME(ADroneVehicle, bApplyingInput);
}

void ADroneVehicle::UpdatePilotingInfo()
{
	if (!Pilot)
	{
		return;
	}

	// To get pilot distance: just dist on the two locations!
	CurrentPilotDistance = FVector::Dist(GetActorLocation(), Pilot->GetActorLocation());
	
	// To get altitude: do a trace downwards from the drone on Visibility and return the distance
	FHitResult Hit;
	FVector End = GetActorLocation();
	FCollisionQueryParams Params = FCollisionQueryParams::DefaultQueryParam;
	bool bHit;

	End.Z -= 1000000.0f;
	Params.AddIgnoredActor(this);

	bHit = World->LineTraceSingleByChannel(Hit, GetActorLocation(), End, ECC_Visibility, Params);
	if (bHit)
	{
		CurrentAltitude = Hit.Distance;
	}

	// Speed = Distance/Time
	float Speed = DistanceMovedThisFrame / World->GetDeltaSeconds();
}

void ADroneVehicle::OnDroneHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	//if (OtherActor && OtherActor != this)
	//{
	//	const float CurrentSpeedAsPercentage = FloatingMovementComponent->Velocity.Size()/MaxSpeed * 100.0f;
	//	float SpeedAsPercentage = FloatingMovementComponent->Velocity.Size()/MaxSpeed * 100.0f;

	//	#if !UE_BUILD_SHIPPING
	//	ULog::Percent(CurrentSpeedAsPercentage, "Current Speed As Percentage: ");
	//	#endif

	//	EDroneDamageSpeed DroneDamageSpeed = EDroneDamageSpeed::DDS_10PercentSpeed;
	//	const int32 MaxValues = uint8(EDroneDamageSpeed::DDS_90PercentSpeed);
	//	for (int32 i = 8; i <= MaxValues; i--)
	//	{
	//		if (CurrentSpeedAsPercentage > (i+1) * 10)
	//		{
	//			DroneDamageSpeed = EDroneDamageSpeed(i);
	//			SpeedAsPercentage = (i+1) * 10;
	//			break;
	//		}
	//	}

	//	if (DroneSpeedToDamageValues.Num() > 0)
	//	{
	//		float DamageToApply = 0.0f;

	//		float* ValuePtr = DroneSpeedToDamageValues.Find(DroneDamageSpeed);
	//		if (ValuePtr)
	//			DamageToApply = *ValuePtr;

	//		if (DamageToApply > 0.0f && FloatingMovementComponent->Velocity.Size() >= MaxSpeed * (SpeedAsPercentage/100) && !GetWorldTimerManager().IsTimerActive(TH_InvincibilityPeriod))
	//		{
	//			TakeDamage(DamageToApply, FDamageEvent(), GetController(), OtherActor);

	//			#if !UE_BUILD_SHIPPING
	//			ULog::Number(DamageToApply, "Applied " + FString::SanitizeFloat(DamageToApply) + " damage at over " + DroneDamageSpeedEnumToString(DroneDamageSpeed) + " : ");
	//			#endif

	//			GetWorldTimerManager().SetTimer(TH_InvincibilityPeriod, InvincibilityTimeAfterDamageApplied, false);
	//		}

	//		#if !UE_BUILD_SHIPPING
	//		ULog::Number(Health, "Current Health: ");
	//		ULog::Number(FloatingMovementComponent->MaxSpeed, "Current Speed: ");
	//		ULog::Number(FloatingMovementComponent->Velocity.Size(), "Current Velocity: ");
	//		#endif
	//	}
	//	else
	//	{
	//		#if !UE_BUILD_SHIPPING
	//		ULog::Warning("DroneSpeedToDamageValues map is empty. Add a few entries to make apply damage work properly on collision");
	//		#endif
	//	}
	//}
}

void ADroneVehicle::Drone_MoveForward(const float Val)
{
	ForwardInput = Val;

	if (bSteadyDrone && !bDroneThirdPerson)
	{
		TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch + MaxTilt * Val * World->GetDeltaSeconds(), -MaxTilt, MaxTilt);
	}

	if (Val != 0.0f && !bSteadyDrone && CurrentAltitude > 5.0f)
	{
		bApplyingInput = true;

		const float TiltForward = MaxTilt * Val * World->GetDeltaSeconds();

		AddActorLocalRotation(FRotator(TiltForward, 0.0f, 0.0f), true, static_cast<FHitResult*>(nullptr), ETeleportType::TeleportPhysics);

		CompensatedDirection = GetActorRotation();
		CompensatedDirection.Pitch = 0.0f;

		// AddActorLocalRotation affects the forward direction, thus producing upward/downward diagonal movement. Use the compensated direction
		if (bDroneThirdPerson)
		{
			// find out which way is forward
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);

			ULog::Rotator(Rotation, false, "Control Rotation: ");

			// get forward vector
			const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			AddMovementInput(Direction, Val);

			//CompensatedDirection = ThirdPersonSpringArm->GetComponentRotation();
			//CompensatedDirection.Pitch = 0.0f;
			//
			//TargetRotation = ThirdPersonCamera->GetForwardVector().Rotation();
			//TargetRotation.Pitch = 0.0f;
			//
			//AddMovementInput(CompensatedDirection.Vector(), Val);
		}
		else
			AddMovementInput(CompensatedDirection.Vector(), -Val);

		DistanceMovedThisFrame = FVector::Dist(ActorLocationLastFrame, GetActorLocation());
	}
	else
	{
		CompensatedDirection = FRotator(0.0f);
		DistanceMovedThisFrame = 0.0f;
		bApplyingInput = false;
	}
}

void ADroneVehicle::Drone_Right(const float Val)
{
	RightInput = Val;

	if (Val != 0.0f && CurrentAltitude > 5.0f)
	{
		bApplyingInput = true;

		const float TiltRight = MaxTilt * -Val * World->GetDeltaSeconds();

		if (!bSteadyDrone)
			AddActorLocalRotation(FRotator(0.0f, 0.0f, TiltRight), true, static_cast<FHitResult*>(nullptr), ETeleportType::TeleportPhysics);

		TurnRight(Val);
	}
	else
	{
		bTurningRight = false;
		bApplyingInput = false;
	}
}

void ADroneVehicle::Drone_Throttle(const float Val)
{
	if (Val != 0.0f && !bSteadyDrone)
	{
		bApplyingInput = true;

		RPM = FMath::FInterpTo(RPM, RPM + Val * RPMThrottleMultiplier, World->GetDeltaSeconds(), ThrottleInterpSpeed);
		RPM = FMath::Clamp(RPM, IdleRPM, MaxRPM);

		FVector WorldDirection = GetActorUpVector();
		if (ForwardInput != 0.0f)
			WorldDirection = GetActorUpVector() + CompensatedDirection.Vector() * (Val > 0.0f ? -ForwardInput : ForwardInput);
		
		AddMovementInput(WorldDirection, Val);
	}
	else
	{
		//RPM = FMath::FInterpTo(RPM, IdleRPM, World->GetDeltaSeconds(), ThrottleInterpSpeed);
		bApplyingInput = false;
	}
}

void ADroneVehicle::Drone_Yaw(const float Val)
{
	if (Val != 0.0f && (bSteadyDrone || !bApplyingInput))
	{
		TurnRight(Val);
	}
}

void ADroneVehicle::TurnRight(const float Val)
{
	if (Val != 0.0f)
	{
		bTurningRight = true;
		
		const float YawRight = MaxTilt * Val * (bSteadyDrone ? TurnSpeedWhenSteady : TurnSpeed) * World->GetDeltaSeconds();

		TargetRotation.Yaw += YawRight;
	}
	else
	{
		bTurningRight = false;
	}
}

//FString ADroneVehicle::DroneDamageSpeedEnumToString(const EDroneDamageSpeed& DroneDamageSpeed)
//{
//	switch (DroneDamageSpeed)
//	{
//		case EDroneDamageSpeed::DDS_90PercentSpeed:
//		return "90% Speed";
//		
//		case EDroneDamageSpeed::DDS_80PercentSpeed:
//		return "80% Speed";
//
//		case EDroneDamageSpeed::DDS_70PercentSpeed:
//		return "70% Speed";
//
//		case EDroneDamageSpeed::DDS_60PercentSpeed:
//		return "60% Speed";
//
//		case EDroneDamageSpeed::DDS_50PercentSpeed:
//		return "50% Speed";
//
//		case EDroneDamageSpeed::DDS_40PercentSpeed:
//		return "40% Speed";
//
//		case EDroneDamageSpeed::DDS_30PercentSpeed:
//		return "30% Speed";
//
//		case EDroneDamageSpeed::DDS_20PercentSpeed:
//		return "20% Speed";
//
//		case EDroneDamageSpeed::DDS_10PercentSpeed:
//		return "10% Speed";
//
//		default:
//		return "None";
//	}
//}

void ADroneVehicle::Drone_Turn(const float Rate)
{
	//ULog::Number(Rate, CUR_CLASS_FUNC + ": ");

	float Sensitivity;
	UBpGameplayHelperLib::GetMouseSensitivity(Sensitivity);

	AddControllerYawInput(Rate * Sensitivity * World->GetDeltaSeconds());
}

void ADroneVehicle::Drone_TurnAtRate(const float Rate)
{
	AddControllerYawInput(Rate * World->GetDeltaSeconds());
}

void ADroneVehicle::Drone_LookUp(const float Rate)
{
	//ULog::Number(Rate, CUR_CLASS_FUNC + ": ");

	float Sensitivity;
	UBpGameplayHelperLib::GetMouseSensitivity(Sensitivity);

	AddControllerPitchInput(Rate * Sensitivity * World->GetDeltaSeconds());
}

void ADroneVehicle::Drone_LookUpAtRate(const float Rate)
{
	AddControllerPitchInput(Rate * World->GetDeltaSeconds());
}

void ADroneVehicle::Drone_Steady()
{
	bSteadyDrone = !bSteadyDrone;
	if (!bSteadyDrone)
	{
		TargetRotation.Pitch = 0.0f;
		TargetRotation.Roll = 0.0f;

#if !UE_BUILD_SHIPPING
		ULog::Info("Drone De-stabilized");
#endif
	}
	else
	{
#if !UE_BUILD_SHIPPING
		ULog::Info("Drone stabilized");
#endif
	}
}

void ADroneVehicle::Drone_QuickTurn()
{
	if (!bTurningRight && CurrentAltitude > 5.0f)
	{
		TargetRotation.Yaw += 180.0f;
	}
}

void ADroneVehicle::Drone_ToggleThirdPerson()
{
	bDroneThirdPerson = !bDroneThirdPerson;
	if (bDroneThirdPerson)
	{
		TargetRotation.Pitch = 0.0f;
		TargetRotation.Roll = 0.0f;

		FirstPersonCamera->Deactivate();
		ThirdPersonCamera->Activate();
	}
	else
	{
		FirstPersonCamera->Activate();
		ThirdPersonCamera->Deactivate();
	}
}

void ADroneVehicle::Server_StartPiloting_Implementation(AReadyOrNotPlayerController* NewController)
{
	DroneOwner = Cast<APlayerCharacter>(GetOwner());

	Super::Server_StartPiloting_Implementation(NewController);
}

void ADroneVehicle::Drone_Exit()
{
	DroneOwner = nullptr;
	
	Server_StopPiloting_Implementation(Cast<AReadyOrNotPlayerController>(GetController()));
}

void ADroneVehicle::IncrementSpeed(const float Val)
{
	// Mouse wheel up
	if (Val > 0.0f)
	{
		FloatingMovementComponent->MaxSpeed = FMath::Clamp(FloatingMovementComponent->MaxSpeed + SpeedIncrementRate, MinSpeed, MaxSpeed);
	}
	// Mouse wheel down
	else if (Val < 0.0f)
	{
		FloatingMovementComponent->MaxSpeed = FMath::Clamp(FloatingMovementComponent->MaxSpeed - SpeedIncrementRate, MinSpeed, MaxSpeed);
	}
}

void ADroneVehicle::Server_UpdateDroneTransform_Implementation(FTransform NewTransform)
{
	DroneTransform = NewTransform;
}

void ADroneVehicle::OnRep_DroneMovement()
{
	ClientDroneTransform = DroneTransform;
}

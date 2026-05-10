// Copyright Void Interactive, 2021

#include "Drone.h"
#include "ReadyOrNot.h"
#include "Log.h"
#include "lib/ReadyOrNotFunctionLibrary.h"

ADrone::ADrone()
{
	AActor::SetReplicateMovement(true);

	bReplicates = true;
	SetActorTickEnabled(false);
	SetCanAffectNavigationGeneration(true, true);
	
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Retrieve the neccessary objects to initialize our settings with
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SKM_DroneMesh(TEXT("SkeletalMesh'/Game/ReadyOrNot/Assets/Devices/Quadrotor/Meshes/SK_Device_Drone.SK_Device_Drone'"));
	//static ConstructorHelpers::FObjectFinder<UAnimBlueprintGeneratedClass> AnimBP_Drone(TEXT("AnimBlueprintGeneratedClass'/Game/ReadyOrNot/Assets/Devices/Quadrotor/Shared/ANIMBP_Drone.ANIMBP_Drone_C'"));
	static ConstructorHelpers::FObjectFinder<UMaterialInstanceConstant> M_FPCameraScreenEffect_1(TEXT("MaterialInstanceConstant'/Game/ThirdParty/SciFiGlitchPostPro/PostProcess/Instance/inst_post_tvcrt.inst_post_tvcrt'"));
	static ConstructorHelpers::FObjectFinder<UCurveFloat> C_DroneFPCamera(TEXT("CurveFloat'/Game/Curves/C_DroneFPCameraReset.C_DroneFPCameraReset'"));
	static ConstructorHelpers::FObjectFinder<UCurveVector> C_FPDroneDamage(TEXT("CurveVector'/Game/Curves/C_DroneFPCameraDamage.C_DroneFPCameraDamage'"));

	//static ConstructorHelpers::FClassFinder<AReadyOrNotPlayerController> BP_DroneController(TEXT("Blueprint'/Game/Blueprints/Controllers/BP_DroneController.BP_DroneController_C'"));

	// Setup the flight box as the root component
	FlightBox = CreateDefaultSubobject<UBoxComponent>(TEXT("FlightBox"));
	FlightBox->SetBoxExtent(FVector(30.0f, 30.0f, 5.0f));
	FlightBox->SetSimulatePhysics(false);
	FlightBox->SetEnableGravity(false);
	FlightBox->SetCanEverAffectNavigation(true);
	FlightBox->bDynamicObstacle = true;
	FlightBox->OnComponentHit.AddDynamic(this, &ADrone::OnDroneHit);
	SetRootComponent(FlightBox);

	// Setup the detection sphere
	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->InitSphereRadius(90.0f);
	DetectionSphere->SetGenerateOverlapEvents(true);
	DetectionSphere->SetupAttachment(RootComponent);
	DetectionSphere->SetCanEverAffectNavigation(true);
	DetectionSphere->bDynamicObstacle = true;
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ADrone::OnDetectionSphereOverlapped);

	// Setup the skeletal mesh
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DroneMesh"));
	Mesh->SetSkeletalMesh(SKM_DroneMesh.Object);
	//Mesh->SetAnimClass(AnimBP_Drone.Object);
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, -5.0f));
	Mesh->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	Mesh->SetCanEverAffectNavigation(true);

	// Setup the audio component
	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(Mesh);
	FPCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FPCamera->SetupAttachment(Mesh);
	FPCamera->SetRelativeLocation(FVector(-5.0f, 0.0f, 10.0f));
	FPCamera->SetRelativeRotation(FRotator(0.0f, -180.0f, 0.0f));

	// Default FPCamera post process settings
	// Chromatic Abberation
	FPCamera->PostProcessSettings.bOverride_SceneFringeIntensity = true;
	FPCamera->PostProcessSettings.SceneFringeIntensity = 1.0f;
	// Lens Flare
	FPCamera->PostProcessSettings.bOverride_LensFlareIntensity = true;
	FPCamera->PostProcessSettings.bOverride_LensFlareBokehSize = true;
	FPCamera->PostProcessSettings.bOverride_LensFlareThreshold = true;
	FPCamera->PostProcessSettings.LensFlareIntensity = 2.0f;
	FPCamera->PostProcessSettings.LensFlareBokehSize = 10.0f;
	FPCamera->PostProcessSettings.LensFlareThreshold = 10.0f;
	// Image Effects
	FPCamera->PostProcessSettings.bOverride_VignetteIntensity = true;
	//FPCamera->PostProcessSettings.bOverride_GrainJitter = true;
	//FPCamera->PostProcessSettings.bOverride_GrainIntensity = true;
	FPCamera->PostProcessSettings.VignetteIntensity = 0.8f;
	//FPCamera->PostProcessSettings.GrainJitter = 0.6f;
	//FPCamera->PostProcessSettings.GrainIntensity = 0.2f;
	// Post Process Materials
	FPCamera->AddOrUpdateBlendable(M_FPCameraScreenEffect_1.Object, 0.02f);
	// Film
	FPCamera->PostProcessSettings.bOverride_FilmSlope = true;
	FPCamera->PostProcessSettings.bOverride_FilmToe = true;
	FPCamera->PostProcessSettings.bOverride_FilmShoulder = true;
	FPCamera->PostProcessSettings.bOverride_FilmBlackClip = true;
	FPCamera->PostProcessSettings.bOverride_FilmWhiteClip = true;
	FPCamera->PostProcessSettings.FilmSlope = 0.847619f;
	FPCamera->PostProcessSettings.FilmToe = 0.454762f;
	FPCamera->PostProcessSettings.FilmShoulder = 0.704762f;
	FPCamera->PostProcessSettings.FilmBlackClip = 0.00005f;
	FPCamera->PostProcessSettings.FilmWhiteClip = 0.257143f;
	// Color Grading | Global
	FPCamera->PostProcessSettings.bOverride_ColorSaturation = true;
	FPCamera->PostProcessSettings.bOverride_ColorContrast = true;
	FPCamera->PostProcessSettings.bOverride_ColorGamma = true;
	FPCamera->PostProcessSettings.bOverride_ColorGain = true;
	FPCamera->PostProcessSettings.ColorSaturation = FVector4(0.0f, 0.0f, 0.0f, 1.0f);
	FPCamera->PostProcessSettings.ColorContrast = FVector4(1.44f, 1.44f, 1.44f, 1.0f);
	FPCamera->PostProcessSettings.ColorGamma = FVector4(0.92f, 0.92f, 0.92f, 1.0f);
	FPCamera->PostProcessSettings.ColorGain = FVector4(1.12f, 1.12f, 1.12f, 1.0f);
	// Color Grading | Shadows
	FPCamera->PostProcessSettings.bOverride_ColorContrastShadows = true;
	FPCamera->PostProcessSettings.bOverride_ColorGammaShadows = true;
	FPCamera->PostProcessSettings.ColorContrast = FVector4(1.26f, 1.26f, 1.26f, 1.0f);
	FPCamera->PostProcessSettings.ColorGamma = FVector4(0.7f, 0.7f, 0.7f, 1.0f);

	// Create a camera boom (pulls in towards the player if there is a collision)
	TPCameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonCameraArm"));
	TPCameraArm->SetupAttachment(RootComponent);
	TPCameraArm->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));
	TPCameraArm->TargetArmLength = 300.0f; // The camera follows at this distance behind the drone
	TPCameraArm->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	TPCameraArm->bEnableCameraLag = true;
	TPCameraArm->CameraLagSpeed = 10.0f;
	TPCameraArm->bInheritPitch = false;
	TPCameraArm->bInheritYaw = false;
	TPCameraArm->SocketOffset = FVector(0.0f, 0.0f, 40.0f);

	// Create a follow camera
	TPCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	TPCamera->SetupAttachment(TPCameraArm, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	TPCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create a custom movement component
	FloatingMovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	FloatingMovementComponent->SetIsReplicated(true);
	FloatingMovementComponent->MaxSpeed = 500.0f;
	FloatingMovementComponent->Acceleration = 600.0f;
	FloatingMovementComponent->Deceleration = 700.0f;

	// Initialize our settings to sensible default values
	FPCameraRotationCurve = C_DroneFPCamera.Object;
	FPDamageCurve = C_FPDroneDamage.Object;

	DroneSpeedToDamageValues.Empty();
	DroneSpeedToDamageValues.Add(EDroneDamageSpeed::DDS_10PercentSpeed, 0.0f);
	DroneSpeedToDamageValues.Add(EDroneDamageSpeed::DDS_20PercentSpeed, 0.0f);
	DroneSpeedToDamageValues.Add(EDroneDamageSpeed::DDS_30PercentSpeed, 10.0f);
	DroneSpeedToDamageValues.Add(EDroneDamageSpeed::DDS_40PercentSpeed, 20.0f);
	DroneSpeedToDamageValues.Add(EDroneDamageSpeed::DDS_50PercentSpeed, 30.0f);
	DroneSpeedToDamageValues.Add(EDroneDamageSpeed::DDS_60PercentSpeed, 30.0f);
	DroneSpeedToDamageValues.Add(EDroneDamageSpeed::DDS_70PercentSpeed, 50.0f);
	DroneSpeedToDamageValues.Add(EDroneDamageSpeed::DDS_80PercentSpeed, 65.0f);
	DroneSpeedToDamageValues.Add(EDroneDamageSpeed::DDS_90PercentSpeed, 70.0f);
}

void ADrone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADrone, RPM);
	DOREPLIFETIME(ADrone, DroneTransform);
	DOREPLIFETIME(ADrone, RotorRotation);
	DOREPLIFETIME(ADrone, TargetRotation);
	DOREPLIFETIME(ADrone, TargetSteadyCameraRotation);

	DOREPLIFETIME(ADrone, OriginalController);

	DOREPLIFETIME(ADrone, bApplyingInput);
	DOREPLIFETIME(ADrone, bSteadyDrone);
}

void ADrone::BeginPlay()
{
	Super::BeginPlay();

	World = GetWorld();
	DroneTransform = GetActorTransform();

	//DroneController = World->SpawnActor<AReadyOrNotPlayerController>(DroneControllerClass, FVector(0.0f), FRotator(0.0f));
	//SetOwner(DroneController);

	UReadyOrNotFunctionLibrary::SetupTimeline(this, TL_ResetFPCameraRotation, FPCameraRotationCurve, false, FPCameraRotationResetSpeed, "Tick_CameraReset");
	UReadyOrNotFunctionLibrary::SetupTimeline(this, TL_Damage, FPDamageCurve, false, FPDamageSpeed, "Tick_CameraDamage", "Finished_CameraDamage");
}

void ADrone::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TL_ResetFPCameraRotation.TickTimeline(DeltaSeconds);
	TL_Damage.TickTimeline(DeltaSeconds);

	if (!HasPilot())
		return;

	if (!GetOwner())
	{
		bApplyingInput = false;
	}

	if (GetOwner() && !GetOwner()->HasLocalNetOwner())
	{
		SetActorLocation(FMath::VInterpTo(GetActorLocation(), DroneTransform.GetLocation(), DeltaSeconds, 10.0f));
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), DroneTransform.GetRotation().Rotator(), DeltaSeconds, bSteadyDrone ? RotationInterpSpeedWhenSteady : RotationInterpSpeed));
	}
	else
	{
		if (bDroneThirdPerson)
		{
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), MovementDirection.Rotation(), DeltaSeconds, bSteadyDrone ? RotationInterpSpeedWhenSteady : RotationInterpSpeed));
		}
		else
		{
			TPCameraArm->SetRelativeRotation(TargetRotation);
			
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), FRotator(TargetRotation.Pitch, TargetRotation.Yaw, 0), DeltaSeconds, bSteadyDrone ? RotationInterpSpeedWhenSteady : RotationInterpSpeed));
		}

		if (HasAuthority())
		{
			Server_UpdateDrone(GetActorTransform(), RPM);
		}
		else
		{
			Client_UpdateDrone(GetActorTransform(), RPM);
		}
	}

	UpdatePilotingInfo();
}

void ADrone::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	
	PlayerInputComponent->BindAxis("Drone_MoveForward", this, &ADrone::MoveForward);
	PlayerInputComponent->BindAxis("Drone_Right", this, &ADrone::MoveRight);
	PlayerInputComponent->BindAxis("Drone_Throttle", this, &ADrone::Throttle);
	PlayerInputComponent->BindAxis("Drone_Yaw", this, &ADrone::Rotate);

	PlayerInputComponent->BindAction("Drone_Steady", IE_Pressed, this, &ADrone::SteadyDrone);
	PlayerInputComponent->BindAction("Drone_ThirdPerson", IE_Pressed, this, &ADrone::ToggleThirdPerson);
	PlayerInputComponent->BindAction("Drone_QuickTurn", IE_DoubleClick, this, &ADrone::QuickTurn);
	PlayerInputComponent->BindAction("Drone_Exit", IE_Released, this, &ADrone::ExitDrone);

	PlayerInputComponent->BindAxis("InrementalSystemAxis", this, &ADrone::IncrementSpeed);
	
	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &ADrone::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ADrone::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &ADrone::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ADrone::LookUpAtRate);
}

void ADrone::UpdatePilotingInfo()
{
	if (!Pilot)
	{
		return;
	}

	CurrentPilotDistance = FVector::Dist(GetActorLocation(), Pilot->GetActorLocation());
	
	// To get altitude: do a trace downwards from the drone on Visibility and return the distance
	FHitResult Hit;
	FVector End = GetActorLocation();
	End.Z -= 1000000.0f;
	FCollisionQueryParams Params = FCollisionQueryParams::DefaultQueryParam;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(Pilot);

	TArray<AActor*> AllPlayerCharacters;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerCharacter::StaticClass(), AllPlayerCharacters);
	Params.AddIgnoredActors(AllPlayerCharacters);

	bool bHit = World->LineTraceSingleByChannel(Hit, GetActorLocation(), End, ECC_Visibility, Params);
	if (bHit)
	{
		CurrentAltitude = Hit.Distance;
	}
}

float ADrone::TakeDamage(const float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.0f)
	{
		TL_Damage.PlayFromStart();
	}

	return ActualDamage;
}

void ADrone::OnDroneHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor && OtherActor != this)
	{
		const float CurrentSpeedAsPercentage = FloatingMovementComponent->Velocity.Size()/MaxSpeed * 100.0f;
		float SpeedAsPercentage = FloatingMovementComponent->Velocity.Size()/MaxSpeed * 100.0f;

		#if !UE_BUILD_SHIPPING
		ULog::Percent(CurrentSpeedAsPercentage, "Current Speed As Percentage: ");
		#endif

		EDroneDamageSpeed DroneDamageSpeed = EDroneDamageSpeed::DDS_10PercentSpeed;
		const int32 MaxValues = uint8(EDroneDamageSpeed::DDS_90PercentSpeed);
		for (int32 i = 8; i <= MaxValues; i--)
		{
			if (CurrentSpeedAsPercentage > (i+1) * 10)
			{
				DroneDamageSpeed = EDroneDamageSpeed(i);
				SpeedAsPercentage = (i+1) * 10;
				break;
			}
		}

		if (DroneSpeedToDamageValues.Num() > 0)
		{
			float DamageToApply = 0.0f;

			float* ValuePtr = DroneSpeedToDamageValues.Find(DroneDamageSpeed);
			if (ValuePtr)
				DamageToApply = *ValuePtr;

			// We have met the threshold, apply damage!
			if (DamageToApply > 0.0f && IsSpeedThresholdMet(SpeedAsPercentage) && !IsInvincible())
			{
				LastHitLocation = Hit.Location;
				DotProductOnDroneHit = FVector::DotProduct(GetActorForwardVector(), LastHitLocation - GetActorLocation());
				bNegativeRollValue = FMath::RandBool();
				
				TakeDamage(DamageToApply, FDamageEvent(), GetController(), OtherActor);

				#if !UE_BUILD_SHIPPING
				ULog::Number(DamageToApply, "Applied " + FString::SanitizeFloat(DamageToApply) + " damage at over " + DroneDamageSpeedEnumToString(DroneDamageSpeed) + " : ");
				#endif

				LastDroneDamageSpeed = DroneDamageSpeed;
				LastDroneDamageAmount = DamageToApply;
				
				GetWorldTimerManager().SetTimer(TH_InvincibilityPeriod, InvincibilityTimeAfterDamageApplied, false);
			}

			#if !UE_BUILD_SHIPPING
			ULog::Number(Health, "Current Health: ");
			ULog::Number(FloatingMovementComponent->MaxSpeed, "Current Speed: ");
			ULog::Number(FloatingMovementComponent->Velocity.Size(), "Current Velocity: ");
			#endif
		}
		else
		{
			#if !UE_BUILD_SHIPPING
			ULog::Warning("DroneSpeedToDamageValues map is empty. Add a few entries to make apply damage work properly on collision");
			#endif
		}
	}
}

void ADrone::OnDetectionSphereOverlapped(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	auto* Player = Cast<APlayerCharacter>(OtherActor);
	if (OtherActor && OtherActor != this && Player && Player == UGameplayStatics::GetPlayerCharacter(this, 0) && !Pilot)
	{
		//Player->StopAnimMontage();
		//Player->GetCharacterMovement()->DisableMovement();
		//Player->Server_LockAllActions();
		//Player->Server_UpdateMoveForwardInput(0.0f);
		Player->GetCharacterMovement()->DisableMovement();
		Player->SetActorRotation(FRotator(0.0f, Player->GetActorRotation().Yaw, Player->GetActorRotation().Roll));

		//DroneController->SetPawn(Player);

		SetActorTickEnabled(true);

		DroneWidgetHUD = CreateWidget<UUserWidget>(Player->GetRONPlayerController(), DroneWidgetClass);
		if (DroneWidgetHUD)
		{
			DroneWidgetHUD->AddToViewport();
		}
		else
		{
			#if !UE_BUILD_SHIPPING
			if (!DroneWidgetClass)
				ULog::Error(CUR_CLASS_FUNC + " | Failed to create DroneWidget. DroneWidgetClass is null.");
			else
				ULog::Error(CUR_CLASS_FUNC + " | Failed to create DroneWidget. DroneWidgetClass is null.");
			#endif
		}

		Server_StartPiloting(Player->GetRONPlayerController());
	}
}

void ADrone::MoveForward(const float Value)
{
	ForwardInput = Value;

	if (bSteadyDrone && !bDroneThirdPerson)
	{
		TargetSteadyCameraRotation.Pitch = FMath::Clamp(TargetSteadyCameraRotation.Pitch + MaxPitchTilt * Value * TurnSpeedWhenSteady * World->GetDeltaSeconds(), -MaxPitchTilt, MaxPitchTilt);

		FPCamera->SetRelativeRotation(FMath::RInterpTo(FPCamera->GetRelativeRotation(), FRotator(TargetSteadyCameraRotation.Pitch, FPCamera->GetRelativeRotation().Yaw, 0), World->GetDeltaSeconds(), RotationInterpSpeedWhenSteady));
	}

	if (Controller && !bSteadyDrone && Value != 0.0f && CurrentAltitude > 7.0f)
	{
		bApplyingInput = true;

		const float TiltForward = MaxPitchTilt * Value * World->GetDeltaSeconds();
		AddActorLocalRotation(FRotator(-TiltForward, 0.0f, 0.0f), true, static_cast<FHitResult*>(nullptr), ETeleportType::TeleportPhysics);

		// find out which way is forward
		FRotator Rotation = GetActorRotation();
		FRotator YawRotation(0, Rotation.Yaw, 0);

		if (bDroneThirdPerson)
		{
			Rotation = TPCameraArm->GetComponentRotation();
			YawRotation = FRotator(0.0f, Rotation.Yaw - GetActorRotation().Yaw, 0.0f);

			TargetRotation = YawRotation;
		}

		// get forward vector
		MovementDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(MovementDirection, Value);
	}
	else
	{
		bApplyingInput = false;
	}
}

void ADrone::MoveRight(const float Value)
{
	RightInput = Value;

	if (Controller && Value != 0.0f && CurrentAltitude > 7.0f)
	{
		bApplyingInput = true;

		const float TiltRight = MaxRollTilt * Value * World->GetDeltaSeconds();

		if (!bSteadyDrone)
			AddActorLocalRotation(FRotator(0.0f, 0.0f, TiltRight), true, static_cast<FHitResult*>(nullptr), ETeleportType::TeleportPhysics);

		if (bDroneThirdPerson && !bSteadyDrone)
		{
			// find out which way is right
			const FRotator Rotation = TPCameraArm->GetComponentRotation();
			const FRotator YawRotation(0, Rotation.Yaw - GetActorRotation().Yaw, 0);

			// get right vector
			const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
			AddMovementInput(Direction, Value);
		}

		TurnRight(Value);
	}
	else
	{
		bApplyingInput = false;
	}
}

void ADrone::TurnAtRate(const float Rate)
{
	AddControllerYawInput(Rate * 45.0f * World->GetDeltaSeconds());
}

void ADrone::LookUpAtRate(const float Rate)
{
	AddControllerPitchInput(Rate * 45.0f * World->GetDeltaSeconds());
}

void ADrone::AddControllerYawInput(const float Value)
{
	Super::AddControllerYawInput(Value);

	float Sensitivity;
	UBpGameplayHelperLib::GetMouseSensitivity(Sensitivity);
	
	TPCameraArm->AddRelativeRotation(FRotator(0.0f, Value, 0.0f));
}

void ADrone::AddControllerPitchInput(const float Value)
{
	Super::AddControllerPitchInput(Value);

	float Sensitivity;
	UBpGameplayHelperLib::GetMouseSensitivity(Sensitivity);

	const FRotator NewRotation = TPCameraArm->GetRelativeRotation() + FRotator(-Value, 0.0f, 0.0f);
	// Clamp at 89 degrees to eliminate camera spazzing around
	if (NewRotation.Pitch <= -89)
	{
		TPCameraArm->SetRelativeRotation(FRotator(-89, NewRotation.Yaw, NewRotation.Roll));
	}
	else if (NewRotation.Pitch >= 89)
	{
		TPCameraArm->SetRelativeRotation(FRotator(89, NewRotation.Yaw, NewRotation.Roll));
	}
	else
	{
		TPCameraArm->SetRelativeRotation(NewRotation);
	}
}

void ADrone::Throttle(const float Rate)
{
	if (Rate != 0.0f && !bSteadyDrone)
	{
		bApplyingInput = true;

		RPM = FMath::FInterpTo(RPM, RPM + Rate * RPMThrottleMultiplier, World->GetDeltaSeconds(), ThrottleInterpSpeed);
		RPM = FMath::Clamp(RPM, IdleRPM, MaxRPM);

		FVector WorldDirection = GetActorUpVector();
		if (ForwardInput != 0.0f)
		{
			WorldDirection = GetActorUpVector() + MovementDirection * (Rate > 0.0f ? ForwardInput : -ForwardInput);
		}
		
		AddMovementInput(WorldDirection, Rate);
	}
	else
	{
		bApplyingInput = false;
	}
}

void ADrone::Rotate(const float Value)
{
	if (Value != 0.0f && (bSteadyDrone || !bApplyingInput))
	{
		TurnRight(Value);
	}
}

void ADrone::TurnRight(const float Value)
{
	if (Value != 0.0f)
	{
		const float YawRight = MaxRollTilt * Value * (bSteadyDrone ? TurnSpeedWhenSteady : TurnSpeed) * World->GetDeltaSeconds();

		TargetRotation.Yaw += YawRight;
		TargetSteadyCameraRotation.Yaw = TargetRotation.Yaw;
	}
}

FString ADrone::DroneDamageSpeedEnumToString(const EDroneDamageSpeed& DroneDamageSpeed)
{
	switch (DroneDamageSpeed)
	{
		case EDroneDamageSpeed::DDS_90PercentSpeed:
		return "90% Speed";
		
		case EDroneDamageSpeed::DDS_80PercentSpeed:
		return "80% Speed";

		case EDroneDamageSpeed::DDS_70PercentSpeed:
		return "70% Speed";

		case EDroneDamageSpeed::DDS_60PercentSpeed:
		return "60% Speed";

		case EDroneDamageSpeed::DDS_50PercentSpeed:
		return "50% Speed";

		case EDroneDamageSpeed::DDS_40PercentSpeed:
		return "40% Speed";

		case EDroneDamageSpeed::DDS_30PercentSpeed:
		return "30% Speed";

		case EDroneDamageSpeed::DDS_20PercentSpeed:
		return "20% Speed";

		case EDroneDamageSpeed::DDS_10PercentSpeed:
		return "10% Speed";

		default:
		return "None";
	}
}

void ADrone::SteadyDrone()
{
	bSteadyDrone = !bSteadyDrone;
	if (!bSteadyDrone)
	{
		TargetRotation.Pitch = 0.0f;
		TargetRotation.Roll = 0.0f;

		TargetSteadyCameraRotation.Pitch = 0.0f;
		TargetSteadyCameraRotation.Roll = 0.0f;

		TL_ResetFPCameraRotation.PlayFromStart();
		
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

void ADrone::ToggleThirdPerson()
{
	bDroneThirdPerson = !bDroneThirdPerson;
	if (bDroneThirdPerson)
	{
		MovementDirection = TargetRotation.Vector();

		FPCamera->Deactivate();
		TPCamera->Activate();

		if (DroneWidgetHUD)
			DroneWidgetHUD->RemoveFromParent();
	}
	else
	{
		FPCamera->Activate();
		TPCamera->Deactivate();

		if (DroneWidgetHUD)
			DroneWidgetHUD->AddToViewport();
	}
}

void ADrone::QuickTurn()
{
	if (RightInput == 0.0f && CurrentAltitude > 7.0f)
	{
		TargetRotation.Yaw += 180.0f;
	}
}

void ADrone::IncrementSpeed(const float Value)
{
	// Mouse wheel up
	if (Value > 0.0f)
	{
		FloatingMovementComponent->MaxSpeed = FMath::Clamp(FloatingMovementComponent->MaxSpeed + SpeedIncrementRate, MinSpeed, MaxSpeed);
		TPCameraArm->CameraLagSpeed = FMath::Clamp(TPCameraArm->CameraLagSpeed + Value, 10.0f, 60.0f);
	}
	// Mouse wheel down
	else if (Value < 0.0f)
	{
		FloatingMovementComponent->MaxSpeed = FMath::Clamp(FloatingMovementComponent->MaxSpeed - SpeedIncrementRate, MinSpeed, MaxSpeed);
		TPCameraArm->CameraLagSpeed = FMath::Clamp(TPCameraArm->CameraLagSpeed - Value, 10.0f, 60.0f);
	}
}

void ADrone::Server_UpdateDrone_Implementation(const FTransform NewTransform, const float InRPM)
{
	DroneTransform = NewTransform;

	RotorRotation.Yaw += InRPM/60 * GetWorld()->GetDeltaSeconds() * 360.0f;
}

void ADrone::Client_UpdateDrone_Implementation(const FTransform NewTransform, const float InRPM)
{
	Server_UpdateDrone(NewTransform, InRPM);
}

void ADrone::Tick_CameraReset()
{
	const float PlaybackPosition = TL_ResetFPCameraRotation.GetPlaybackPosition();
	const float CurveValue = FPCameraRotationCurve->GetFloatValue(PlaybackPosition);

	FPCamera->SetRelativeRotation(FMath::RInterpTo(FPCamera->GetRelativeRotation(), FRotator(0.0f, 180.0f, 0.0f), World->GetDeltaSeconds(), CurveValue * 10.0f));
}

void ADrone::Tick_CameraDamage()
{
	const float PlaybackPosition = TL_Damage.GetPlaybackPosition();
	const FVector CurveValue = FPDamageCurve->GetVectorValue(PlaybackPosition);

	const FRotator CurveValueAsRotator = FRotator(CurveValue.Y, GetActorRotation().Yaw, bNegativeRollValue ? CurveValue.X : -CurveValue.X);

//#if WITH_EDITOR
//	ULog::Number(PlaybackPosition, CUR_CLASS_FUNC + " | Time: ");
//	ULog::Number(DotProductOnDroneHit, CUR_CLASS_FUNC + " | Dot Product: ");
//	ULog::Rotator(GetActorRotation(), CUR_CLASS_FUNC + " | Current Rotation: ");
//#endif

	SetActorRotation(CurveValueAsRotator);
}

void ADrone::Finished_CameraDamage()
{
	//TL_ResetFPCameraRotation.PlayFromStart();
}

bool ADrone::IsMoving() const
{
	return FloatingMovementComponent->Velocity.Size() > 0.0f;
}

bool ADrone::IsInvincible() const
{
	return GetWorldTimerManager().IsTimerActive(TH_InvincibilityPeriod);
}

float ADrone::GetCurrentSpeedAsPercentage() const
{
	return FloatingMovementComponent->Velocity.Size()/MaxSpeed * 100.0f;
}

void ADrone::RetrieveLastHitDamageInfo(EDroneDamageSpeed& InDroneDamageSpeed, float& InDamageAmount) const
{
	InDroneDamageSpeed = LastDroneDamageSpeed;
	InDamageAmount = LastDroneDamageAmount;
}

bool ADrone::IsSpeedThresholdMet(const float InSpeedAsPercentage)
{
	return FloatingMovementComponent->Velocity.Size() >= MaxSpeed * (InSpeedAsPercentage / 100);
}

void ADrone::ExitDrone()
{
	Server_StopPiloting(Cast<AReadyOrNotPlayerController>(GetController()));

	SetActorTickEnabled(false);

	if (DroneWidgetHUD)
		DroneWidgetHUD->RemoveFromParent();
}

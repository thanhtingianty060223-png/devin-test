// Copyright Void Interactive, 2017

#include "ReplayCameraPawn.h"

#include "CineCameraComponent.h"
#include "ReadyOrNot.h"
#include "ReplayController.h"
#include "Components/SplineComponent.h"

static const float SPEED_SCALE_ADJUSTMENT = 0.1f;

// Sets default values
AReplayCameraPawn::AReplayCameraPawn()
{
	this->SetActorTickEnabled(true);
	this->SetTickableWhenPaused(true);
	this->bAlwaysRelevant = true;

	bAddDefaultMovementBindings = false;

	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;
	
	PawnCamera = CreateDefaultSubobject<UCineCameraComponent>(TEXT("PawnCamera"));
	PawnCamera->bUsePawnControlRotation = false;
	PawnCamera->SetTickableWhenPaused(true);
	
	SpringArm = CreateDefaultSubobject<UReplaySpringArm>(TEXT("SpringArm"));
	SpringArm->TargetArmLength = 0;
	SpringArm->SetTickableWhenPaused(true);

	FloatingPawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingMovement"));
	FloatingPawnMovement->SetTickableWhenPaused(true);
	FloatingPawnMovement->MaxSpeed = InitialMaxSpeed = 1200.f;
	FloatingPawnMovement->Acceleration = InitialAcceleration = 4000.f;
	FloatingPawnMovement->Deceleration = InitialDeceleration = 12000.f;
	
	PawnCamera->AttachToComponent(SpringArm, FAttachmentTransformRules::KeepRelativeTransform);
	RootComponent = SpringArm;
	SpringArm->bUsePawnControlRotation = false;	
}

UPawnMovementComponent* AReplayCameraPawn::GetMovementComponent() const
{
	return FloatingPawnMovement;
}

// Called when the game starts or when spawned
void AReplayCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	// Ensure we are in a replay session.
	if( !GetWorld()->IsPlayingReplay() && !GetWorld()->IsPlayingClientReplay())
	{
		return;
	}

	
	// Set the default post process settings.
	bool Found = false;
	for (TActorIterator<APostProcessVolume>It(GetWorld()); It; ++It)
	{
		DefaultPostProcessSettings = It->Settings;
		Found = true;
		break;
	}
	if(!Found)
	{
		DefaultPostProcessSettings = PawnCamera->PostProcessSettings;
	}

	DefaultPostProcessSettings.bOverride_AutoExposureBias = false;
	DefaultPostProcessSettings.bOverride_AutoExposureLowPercent = false;
	DefaultPostProcessSettings.bOverride_AutoExposureHighPercent = false;
	DefaultPostProcessSettings.bOverride_AutoExposureBiasCurve = false;
	DefaultPostProcessSettings.bOverride_AutoExposureMethod = false;
	DefaultPostProcessSettings.bOverride_AutoExposureMinBrightness = false;
	DefaultPostProcessSettings.bOverride_AutoExposureMaxBrightness = false;
	
	PawnCamera->PostProcessSettings = DefaultPostProcessSettings;
	
	// Get the sensitivity
	UBpGameplayHelperLib::GetMouseSensitivity(Sensitivity);
	
	// Send the player to the spawn.
	for (TActorIterator<APlayerStart>It(GetWorld()); It; ++It)
	{
		if (It)
		{
			SetActorLocation(It->GetActorLocation() + FVector(0, 150, 0));
			break;
		}
	}

	// Set the view target to this.
	UGameplayStatics::GetPlayerController(GetWorld(), 0)->SetViewTarget(this);
}

bool AReplayCameraPawn::IsExceedingMaxSpeed(float MaxSpeed) const
{
	MaxSpeed = FMath::Max(0.f, MaxSpeed);
	const float MaxSpeedSquared = FMath::Square(MaxSpeed);
	
	// Allow 1% error tolerance, to account for numeric imprecision.
	const float OverVelocityPercent = 1.01f;
	return (Velocity.SizeSquared() > MaxSpeedSquared * OverVelocityPercent);
}

// Called every frame
void AReplayCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AReplayController* ReplayController = Cast<AReplayController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(ReplayController)
	{
		// Following the spline
		if(ReplayController->bIsFollowingSpline)
		{

			ReplayController->DeltaSplineTime += DeltaTime;
			float CurrentTime = ReplayController->DeltaSplineTime/ReplayController->TotalSplineTime;
			float CurrentDistance = ReplayController->ReplaySplineActor->SplineComponent->GetSplineLength()*CurrentTime;

			// Set the location.
			SetActorLocation(ReplayController->ReplaySplineActor->SplineComponent->GetLocationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World));

			// Which rotation type are we using?
			if(ReplayController->SplineRotationType == IntoPath)
			{
				SetActorRotation(ReplayController->ReplaySplineActor->SplineComponent->GetRotationAtDistanceAlongSpline(CurrentDistance, ESplineCoordinateSpace::World));
			}
			else if(ReplayController->SplineRotationType == Default)
			{
				//Find which spline point index is ahead of us.
				uint32 NextSplinePointIndex = 0;
				for(int i = 1; i < ReplayController->ReplaySplineActor->SplineComponent->GetNumberOfSplinePoints(); i++)
				{
					if(ReplayController->ReplaySplineActor->SplineComponent->GetDistanceAlongSplineAtSplineInputKey(i) > CurrentDistance)
					{
						NextSplinePointIndex = i;
						break;
					}
				}

				// Prevent OOB when finishing spline path.
				if(NextSplinePointIndex > 0)
				{
					// (current distance - min) /(max - min)
					float Alpha = (CurrentDistance - ReplayController->ReplaySplineActor->SplineComponent->GetDistanceAlongSplineAtSplineInputKey(NextSplinePointIndex-1))
						/(ReplayController->ReplaySplineActor->SplineComponent->GetDistanceAlongSplineAtSplineInputKey(NextSplinePointIndex) - ReplayController->ReplaySplineActor->SplineComponent->GetDistanceAlongSplineAtSplineInputKey(NextSplinePointIndex-1));

					FRotator MinRotator = ReplayController->ReplaySplineActor->SplinePointRotations[NextSplinePointIndex-1];
					FRotator MaxRotator = ReplayController->ReplaySplineActor->SplinePointRotations[NextSplinePointIndex];

					FRotator NewRotation = UKismetMathLibrary::RLerp(MinRotator, MaxRotator, Alpha, true);
					SetActorRotation(NewRotation);
				}
			}

			V_LOGM(LogReadyOrNot, "Following spline.")

			// Check if we've exceeded the bounds of the spline.
			if(CurrentTime >= 1)
			{
				V_LOGM(LogReadyOrNot, "Ending spline follow.")
				ReplayController->bIsFollowingSpline = false;
				ReplayController->DeltaSplineTime = 0;
			}
		}
		
	}

	UpdateLocation(DeltaTime);
}

void AReplayCameraPawn::UpdateLocation(float DeltaTime)
{
	const FVector ControlAcceleration = InputAcceleration.GetClampedToMaxSize(1.f);

	const float AnalogInputModifier = (ControlAcceleration.SizeSquared() > 0.f ? ControlAcceleration.Size() : 0.f);
	const float MaxPawnSpeed = (InitialMaxSpeed*( 0.009297 * FMath::Pow(3.129, SpeedApplicator))) * AnalogInputModifier;
	const bool bExceedingMaxSpeed = IsExceedingMaxSpeed(MaxPawnSpeed);

	if (AnalogInputModifier > 0.f && !bExceedingMaxSpeed)
	{
		// Apply change in velocity direction
		if (Velocity.SizeSquared() > 0.f)
		{
			// Change direction faster than only using acceleration, but never increase velocity magnitude.
			const float TimeScale = FMath::Clamp(DeltaTime * TurningBoost, 0.f, 1.f);
			Velocity = Velocity + (ControlAcceleration * Velocity.Size() - Velocity) * TimeScale;
		}
	}
	else
	{
		// Dampen velocity magnitude based on deceleration.
		if (Velocity.SizeSquared() > 0.f)
		{
			const FVector OldVelocity = Velocity;
			const float VelSize = FMath::Max(Velocity.Size() - FMath::Abs(InitialDeceleration) * DeltaTime, 0.f);
			Velocity = Velocity.GetSafeNormal() * VelSize;

			// Don't allow braking to lower us below max speed if we started above it.
			if (bExceedingMaxSpeed && Velocity.SizeSquared() < FMath::Square(MaxPawnSpeed))
			{
				Velocity = OldVelocity.GetSafeNormal() * MaxPawnSpeed;
			}
		}
	}

	// Apply acceleration and clamp velocity magnitude.
	const float NewMaxSpeed = (IsExceedingMaxSpeed(MaxPawnSpeed)) ? Velocity.Size() : MaxPawnSpeed;
	Velocity += ControlAcceleration * FMath::Abs(InitialAcceleration) * DeltaTime;
	Velocity = Velocity.GetClampedToMaxSize(NewMaxSpeed);

	FVector Delta = Velocity * DeltaTime;

	SetActorLocation(GetActorLocation() + Delta);

	InputAcceleration = FVector(0, 0, 0);
}

void AReplayCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("ReplayYaw", this, &AReplayCameraPawn::AddYaw).bExecuteWhenPaused = true;
	PlayerInputComponent->BindAxis("ReplayPitch", this, &AReplayCameraPawn::AddPitch).bExecuteWhenPaused = true;

	PlayerInputComponent->BindAxis("ReplayMoveForward", this, &AReplayCameraPawn::MoveForward).bExecuteWhenPaused = true;
	PlayerInputComponent->BindAxis("ReplayMoveRight", this, &AReplayCameraPawn::MoveRight).bExecuteWhenPaused = true;
	
	PlayerInputComponent->BindAxis("ReplayMoveUp", this, &AReplayCameraPawn::MoveUp).bExecuteWhenPaused = true;

	PlayerInputComponent->BindAxis("ReplayChangeSpeed", this, &AReplayCameraPawn::AdjustSpeed).bExecuteWhenPaused = true;
	
	PlayerInputComponent->SetTickableWhenPaused(true);
}


void AReplayCameraPawn::MoveUp(float Val)
{
	AReplayController* ReplayController = Cast<AReplayController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(!ReplayController)
	{
		InputAcceleration += GetActorUpVector() * Val;
		return;
	}

	if(ReplayController->CurrentCameraState == Freecam && !ReplayController->bIsFollowingSpline)
	{
		InputAcceleration += GetActorUpVector() * Val;
	}
	else if(ReplayController->CurrentCameraState == Mounted && !ReplayController->bMountedTransformLock)
	{
		ReplayController->MountedLocationOffset += PawnCamera->GetComponentRotation().Vector()*SpeedMultiplier*Val;
	}
	else if(ReplayController->CurrentCameraState == Orbit)
	{
		ReplayController->AdjustableVerticalOffset += 2.0f*Val;
	}
}

void AReplayCameraPawn::AddYaw(float Val)
{
	AReplayController* ReplayController = Cast<AReplayController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(!ReplayController)
	{
		AddControllerYawInput( Val*Sensitivity);
		return;
	}
	
	if(ReplayController->CurrentCameraState != Mounted)
	{
		AddControllerYawInput( Val*Sensitivity);
	}
	else
	{
		ReplayController->MountedRotationOffset.Yaw += Val*Sensitivity;
	}
}

void AReplayCameraPawn::AddPitch(float Val)
{
	AReplayController* ReplayController = Cast<AReplayController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(!ReplayController)
	{
		AddControllerPitchInput(Val*Sensitivity);
		return;
	}
	
	if(ReplayController->CurrentCameraState != Mounted)
	{
		AddControllerPitchInput(Val*Sensitivity);
	}
	else
	{
		ReplayController->MountedRotationOffset.Pitch += -1*Val*Sensitivity;
	}
}

void AReplayCameraPawn::MoveForward(float Val)
{
	AReplayController* ReplayController = Cast<AReplayController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(!ReplayController)
	{
		InputAcceleration += PawnCamera->GetComponentRotation().Vector() * Val;
		return;
	}
	
	if(ReplayController->CurrentCameraState == Freecam)
	{
		InputAcceleration += PawnCamera->GetComponentRotation().Vector() * Val;
	}
	else if(ReplayController->CurrentCameraState == Mounted)
	{
		ReplayController->MountedLocationOffset += PawnCamera->GetComponentRotation().Vector()*Val*SpeedMultiplier;
	}
	else if(ReplayController->CurrentCameraState == Orbit)
	{
		SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength + Val*-5, 0.0f, 1000.0f);
	}
}

void AReplayCameraPawn::MoveRight(float Val)
{
	AReplayController* ReplayController = Cast<AReplayController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(!ReplayController)
	{
		InputAcceleration += UKismetMathLibrary::GetRightVector(PawnCamera->GetComponentRotation()) * Val;
		return;
	}
	
	if(ReplayController->CurrentCameraState == Freecam)
	{
		InputAcceleration += UKismetMathLibrary::GetRightVector(PawnCamera->GetComponentRotation()) * Val;
	}
	else if(ReplayController->CurrentCameraState == Mounted)
	{
		ReplayController->MountedLocationOffset += UKismetMathLibrary::GetRightVector(PawnCamera->GetComponentRotation())*Val*SpeedMultiplier;
	}
}

void AReplayCameraPawn::AdjustSpeed(float Val)
{
	AReplayController* ReplayController = Cast<AReplayController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if(!ReplayController)
	{
		SpeedApplicator += SPEED_SCALE_ADJUSTMENT * Val; 
		SpeedApplicator = FMath::Clamp(SpeedApplicator,  0.0f, 10000.0f);

		if(SpeedApplicator <= 0)
		{
			FloatingPawnMovement->MaxSpeed = 0.0f;
		}
		else
		{
			FloatingPawnMovement->MaxSpeed = InitialMaxSpeed*( 0.009297 * FMath::Pow(3.129, SpeedApplicator));
		}
		return;
	}
	
	if(ReplayController->CurrentCameraState == Freecam || ReplayController->CurrentCameraState == Mounted)
	{

		SpeedApplicator += SPEED_SCALE_ADJUSTMENT * Val; 
		SpeedApplicator = FMath::Clamp(SpeedApplicator,  0.0f, 10000.0f);

		if(SpeedApplicator <= 0)
		{
			FloatingPawnMovement->MaxSpeed = 0.0f;
		}
		else
		{
			FloatingPawnMovement->MaxSpeed = InitialMaxSpeed*( 0.009297 * FMath::Pow(3.129, SpeedApplicator));
		}
		
	}
}

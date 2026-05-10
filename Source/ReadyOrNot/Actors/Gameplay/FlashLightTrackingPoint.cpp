// Void Interactive, 2020

#include "FlashLightTrackingPoint.h"

#include "ReadyOrNotAIConfig.h"
#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"
#include "Senses/ReadyOrNotAISense_Sight.h"

TAutoConsoleVariable<int32> CVarRonDrawFlashlightSense(TEXT("AI.DrawFlashlightSense"), 0, TEXT("Draw AI flashlight senses"));

AFlashLightTrackingPoint::AFlashLightTrackingPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0167f;
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TrackingMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetStaticMesh(TrackingMeshAsset.Object);
	
	SetRootComponent(MeshComp);

	PerceptionStimuliComp = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionComp"));

	SetupTrackingPoint();
}

AReadyOrNotCharacter* AFlashLightTrackingPoint::GetOwnerCharacter() const
{
	if (const ABaseItem* Item = GetOwner<ABaseItem>())
		return Item->GetOwnerCharacter();

	return nullptr;
}

void AFlashLightTrackingPoint::BeginPlay()
{
	Super::BeginPlay();
	
	SetActorTickEnabled(true);
	SetActorTickInterval(0.0167f);
}

void AFlashLightTrackingPoint::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	#if !UE_BUILD_SHIPPING
	if (CVarRonDrawFlashlightSense.GetValueOnAnyThread() > 0)
	{
		if (bIsPrimary)
		{
			const float FlashlightSeenReactionTime = FMath::Clamp(AI_CONFIG_GET_FLOAT("FlashlightSeenReactionTime", 0.25f), 0.0f, 60.0f);
			const float FlashlightForgetTime = FMath::Clamp(AI_CONFIG_GET_FLOAT("FlashlightForgetTime", 0.5f), 0.0f, 60.0f);

			FString Message = GetName() + LINE_TERMINATOR + LINE_TERMINATOR;

			for (const ACyberneticController* Controller : SensedByControllers)
			{
				if (!Controller)
					continue;
				
				if (!Controller->GetCharacter())
					continue;

				if (!Controller->IsSightReactingToActor(this))
					continue;
				
				const float CurrentSenseTime = Controller->GetSightSenseTimeForActor(this, "FlashlightSeen");

				Message += Controller->GetName() + LINE_TERMINATOR +
							"Flashlight sense time: " + FString::Printf(TEXT("%.2f"), CurrentSenseTime) + LINE_TERMINATOR +
							"Reaction time: " + FString::Printf(TEXT("%.2f"), FlashlightSeenReactionTime) + LINE_TERMINATOR +
							"Forget time: " + FString::Printf(TEXT("%.2f"), FlashlightForgetTime) + LINE_TERMINATOR + LINE_TERMINATOR;

				DrawDebugLine(GetWorld(), Controller->GetCharacter()->GetMesh()->GetSocketLocation("head"), GetActorLocation(), FColor::Black, false, DeltaSeconds, 0, 1.5f);
			}

			DrawDebugString(GetWorld(), GetActorLocation(), Message, nullptr, FColor::Emerald, DeltaSeconds, true);
			DrawDebugSphere(GetWorld(), GetActorLocation(), 10.0f, 4, FColor::Red, false, DeltaSeconds, 0, 1.5f);
		}
	}
	#endif
}

void AFlashLightTrackingPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	PerceptionStimuliComp->UnregisterFromSense(UReadyOrNotAISense_Sight::StaticClass());
	PerceptionStimuliComp->UnregisterFromPerceptionSystem();
}

void AFlashLightTrackingPoint::OnAIPerceptionSense_Implementation(ACyberneticController* InSenseController, FAIStimulus Stimulus, AActor*& OutOverrideSensedActor)
{
	if (!bIsPrimary)
		return;
	
	// Don't sense flashlights that are off
	if (!bIsActive)
		return;

	if (!GetOwner())
		return;

	const float FlashlightPerceptionRange = FMath::Clamp(AI_CONFIG_GET_FLOAT("FlashlightPerceptionRange", 2000.0f), 100.0f, 10000.0f);
	const float FlashlightSeenReactionTime = FMath::Clamp(AI_CONFIG_GET_FLOAT("FlashlightSeenReactionTime", 0.25f), 0.0f, 60.0f);
	const float FlashlightForgetTime = FMath::Clamp(AI_CONFIG_GET_FLOAT("FlashlightForgetTime", 0.5f), 0.0f, 9999.0f);

	const float DistanceToFlashLight = FVector::Distance(Stimulus.ReceiverLocation, Stimulus.StimulusLocation);
	//LOG_NUMBER(DistanceToFlashLight);
	const float DistanceToFlashLightOnOwner = FVector::Distance(Stimulus.ReceiverLocation, GetOwner()->GetActorLocation());
	//LOG_NUMBER(DistanceToFlashLightOnOwner);

	if (DistanceToFlashLight <= FlashlightPerceptionRange && DistanceToFlashLightOnOwner <= FlashlightPerceptionRange)
	{
		if (!InSenseController->IsSightReactingToActor(this))
		{
			FActorSense FlashlightSightSense;
			FlashlightSightSense.Actor = this;
			FlashlightSightSense.Tag = "FlashlightSeen";
			FlashlightSightSense.SenseReactionTime = FlashlightSeenReactionTime;
			FlashlightSightSense.SenseForgetTime = FlashlightForgetTime;
			
			InSenseController->AddActorSightSense(FlashlightSightSense);

			SensedByControllers.AddUnique(InSenseController);
		}
	}
	// Out of range, forget the flashlight
	else
	{
		if (InSenseController->GetSightSenseTimeForActor(this, "FlashlightSeen") > FlashlightForgetTime)
		{
			InSenseController->RemoveActorSightSense(this, "FlashlightSeen");

			SensedByControllers.Remove(InSenseController);
		}
	}
}

void AFlashLightTrackingPoint::SetupTrackingPoint()
{
	MeshComp->SetEnableGravity(false);
	MeshComp->SetSimulatePhysics(false);
	MeshComp->SetHiddenInGame(true);
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetRelativeScale3D(FVector(0.01f, 0.01f, 0.01f));

	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetCollisionObjectType(ECC_WorldDynamic);
	MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	/*MeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	MeshComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	MeshComp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	MeshComp->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Block);
	MeshComp->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);*/

	if (bIsPrimary)
	{
		PerceptionStimuliComp->RegisterWithPerceptionSystem();
		PerceptionStimuliComp->RegisterForSense(UReadyOrNotAISense_Sight::StaticClass());
	}
	else
	{
		PerceptionStimuliComp->UnregisterFromSense(UReadyOrNotAISense_Sight::StaticClass());
		PerceptionStimuliComp->UnregisterFromPerceptionSystem();
	}
}

void AFlashLightTrackingPoint::ToggleTrackingPoint(const bool bOn)
{
	bIsActive = bOn;
}


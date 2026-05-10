// Copyright Void Interactive, 2022


#include "OcclusionTestActor.h"

#include "Components/FMODAudioPropagationComponent.h"


// Sets default values
AOcclusionTestActor::AOcclusionTestActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = TickInterval;
	FMODAudioPropagationComp = CreateDefaultSubobject<UFMODAudioPropagationComponent>(TEXT("FMODAudioPropagationComponent"));
	AudioComponent = CreateDefaultSubobject<UFMODAudioComponent>(TEXT("UFMODAudioComponent"));
}

// Called when the game starts or when spawned
void AOcclusionTestActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AOcclusionTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AudioComponent->Stop();
	AudioComponent->ParameterCache.Empty();
	AudioComponent->bAutoActivate = false;
	AudioComponent->bAutoDestroy = false;
	AudioComponent->bStopWhenOwnerDestroyed = true;
	//AudioComponent->RegisterComponentWithWorld(GetWorld());
	AudioComponent->SetWorldLocation(GetActorLocation());

	// Gunshot
	if(GunshotOrFootstep)
	{
		AudioComponent->Event = GunshotSound;
		
		// Get the local player, the listener.
		APlayerCharacter* LocalPlayer = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());
		float OcclusionAmount = 0.0f;
		if (LocalPlayer)
		{
			// Add Ignored Actors
			TArray<AActor*> ActorsToIgnore = {this, LocalPlayer};
			// Add listener to ignored actors.
			ActorsToIgnore.Append(LocalPlayer->GetCollisionIgnoredActors());
					
			OcclusionAmount = FMODAudioPropagationComp->GetDepthMaterialOcclusionAmount(GetWorld(),ActorsToIgnore, GetActorLocation(), LocalPlayer->GetFirstPersonCameraComponent()->GetComponentLocation(), FullOcclusionDepth, OcclusionMultiplier);

			// Set the occlusion parameter
			AudioComponent->SetParameter("Occlusion",  OcclusionAmount);
			AudioComponent->SetParameter("IsOutside", bIsOutside ? 1.0f : 0.0f);
			V_LOGM(LogReadyOrNot, "Third person gunshot parameters set. Occlusion at: %f", OcclusionAmount);
		}

		if (AudioComponent && !AudioComponent->IsPlaying() && OcclusionAmount != -1)
		{
			AudioComponent->Play();
		}
	}
	//Footstep
	else
	{
		AudioComponent->Event = FootstepSound;
		// Footstep event and audio component
		
		AudioComponent->SetParameter("Armor", bHeavyArmor);


		// Our local listener
		APlayerCharacter* LocalPlayer = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());
		// Our local pawn
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

		// Calculate zoff
		float zDiff = 0.0f;
		if (LocalPlayer)
		{
			zDiff = FVector(GetActorLocation() - LocalPlayer->GetFirstPersonCameraComponent()->GetComponentLocation()).Z;
		}
		else if (Pawn)
		{
			zDiff = FVector(GetActorLocation() - Pawn->GetActorLocation()).Z;
		}

		// Get and set extra parameters
		int32 stance, speed, surface;
		GetFMODFootstepParameters(stance, speed, surface);
		AudioComponent->SetParameter("Stance", stance);
		AudioComponent->SetParameter("Surface", surface);
		AudioComponent->SetParameter("HeightDiff", zDiff);
		AudioComponent->SetParameter("HeightNegative", zDiff < 0.0f ? 1.0f : 0.0f);
		
		// Calculate occlusion
		float OcclusionAmount = 0.0f;
		AActor* LocalOwner = GetOwner();
		TArray<AActor*> ActorsToIgnore = {LocalOwner, this, LocalPlayer, Pawn};
		if(FMODAudioPropagationComp)
		{
			if(LocalPlayer)
			{
				ActorsToIgnore.Append(LocalPlayer->GetCollisionIgnoredActors());
				OcclusionAmount = FMODAudioPropagationComp->GetDepthMaterialOcclusionAmount(GetWorld(),ActorsToIgnore, GetActorLocation(), LocalPlayer->GetFirstPersonCameraComponent()->GetComponentLocation(), FullOcclusionDepth, OcclusionMultiplier);
			}
			else if(Pawn){
				OcclusionAmount = FMODAudioPropagationComp->GetDepthMaterialOcclusionAmount(GetWorld(), ActorsToIgnore, GetActorLocation(), Pawn->GetActorLocation(), FullOcclusionDepth, OcclusionMultiplier);
			}
		}
		AudioComponent->SetParameter("Occlusion", OcclusionAmount);

		// Play the foostep sound
		if(AudioComponent && OcclusionAmount != -1)
		{
			AudioComponent->Play();
		}
	}
}

	void AOcclusionTestActor::GetFMODFootstepParameters(int32& Stance, int32& Speed, int32& Surface)
	{
		Stance = bIsCrouching ? 1 : 2;
		if (bIsCrouching)
		{
			Stance = 1;
		}
	
		Speed = FMath::CeilToInt(UKismetMathLibrary::NormalizeToRange(GetVelocity().Size2D(), 0.0f, 350.0f) * 5.0f);

		static FName NAME_LineOfSight = FName(TEXT("LineOfSight"));
		FCollisionQueryParams CollisionParams(NAME_LineOfSight);
		CollisionParams.bReturnPhysicalMaterial = true;
		/* The visibility trace should not hit the target as this will invalidate results*/
		CollisionParams.AddIgnoredActor(this);

		// This should be where the grenade currently is as we need to trace to the eyes
		FVector StartTrace = GetActorLocation();
		/* End the trace so many units in front of the unit */
		FVector EndTrace = StartTrace + GetActorUpVector() * -300.0f;

		//float Distance = FVector(StartTrace - EndTrace).Size();

		FHitResult HitResult;

		/* Do a line trace from center of screen to where we are looking, if it hits a weapon pick it up */
		//DrawDebugLine(GetWorld(), StartTrace, EndTrace, FColor::White, 1.0f, 0, 0.2f);
		GetWorld()->LineTraceSingleByChannel(HitResult, StartTrace, EndTrace, ECollisionChannel::ECC_Visibility, CollisionParams);

		UPhysicalMaterial* HitPhysMat = HitResult.PhysMaterial.Get();
		EPhysicalSurface HitSurfaceType = UPhysicalMaterial::DetermineSurfaceType(HitPhysMat);

		Surface = (int32)HitSurfaceType;
	}


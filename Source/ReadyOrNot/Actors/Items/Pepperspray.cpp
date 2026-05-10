// Copyright Void Interactive, 2021

#include "Pepperspray.h"

#include "DamageTypes/LessLethal/PepperSprayDamageType.h"

#include "Components/AmmoComponent.h"

#include "FMODAudioComponent.h"
#include "ReadyOrNotAIConfig.h"
#include "Characters/CyberneticCharacter.h"
#include "Components/MoraleComponent.h"

#include "Info/ScoringManager.h"

APepperspray::APepperspray()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.0167f;
	
	SetCanBeDamaged(false);
	bFindCameraComponentWhenViewTarget = false;

	SprayParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Spray Particle Component"));
	SprayParticleComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	SprayParticleComponent->SetupAttachment(ItemMesh, "tag_muzzle");
	SprayParticleComponent->bAutoActivate = false;
	SprayParticleComponent->bOwnerNoSee = false;
	SprayParticleComponent->bOnlyOwnerSee = false;
	
	FMODAudioComp->SetupAttachment(ItemMesh, "tag_muzzle");
	FMODAudioComp->bAutoActivate = false;

	AmmoComponent = CreateDefaultSubobject<UAmmoComponent>(TEXT("Ammo Component"));
	AmmoComponent->OnLowResource.AddDynamic(this, &APepperspray::OnLowPeppersprayAmmo);
	AmmoComponent->OnDepletedResource.AddDynamic(this, &APepperspray::OnDepletedPeppersprayAmmo);
}

void APepperspray::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APepperspray, bSpraying);
}

void APepperspray::BeginPlay()
{
	Super::BeginPlay();

	SetupPepperspray();
}

void APepperspray::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (!GetWorld() || (GetWorld() && GetWorld()->bIsTearingDown))
		return;
	
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		if (GS->bInPlanningMenu)
		{
			return;
		}
	}
		
	AReadyOrNotCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		StopSpraying();
		
		return;
	}

	if (OwnerCharacter->IsDeadOrUnconscious())
	{
		if (bSpraying)
			StopSpraying();
		
		return;
	}
	
	if (bSpraying)
	{
		FHitResult ImpactResultTrace;

		FVector EndPoint = SprayParticleComponent->GetComponentLocation() + (SprayParticleComponent->GetComponentRotation().Vector() * SprayDistance);

		if (HasAuthority() && AmmoComponent->HasResource())
		{
			FCollisionQueryParams CollisionQueryParams = OwnerCharacter->GetCollisionQueryParameters();
			
			//DrawDebugLine(GetWorld(), SprayParticleComponent->GetComponentLocation(), EndPoint, FColor::Red, false, 0.033f, 0, 1.0f);

			//TArray<FHitResult> HitResults;
			GetWorld()->LineTraceSingleByChannel(ImpactResultTrace, SprayParticleComponent->GetComponentLocation(), EndPoint, ECC_PROJECTILE, CollisionQueryParams);
			
			//for (const FHitResult& Hit : HitResults)
			//{
			//ImpactResultTrace = Hit;
			
			if (AReadyOrNotCharacter* HitCharacter = Cast<AReadyOrNotCharacter>(ImpactResultTrace.GetActor()))
			{
				if (HitCharacter->IsAffectedByDamageTypeClass(UPepperSprayDamageType::StaticClass()))
				{
					if (HitCharacter->IsPepperSprayedLocationValid(ImpactResultTrace, this))
					{
						if (!HitCharacter->IsPepperSprayed())
						{
							FDamageEvent DamageEvent;
							DamageEvent.DamageTypeClass = DamageType ? DamageType : (TSubclassOf<UDamageType>) UPepperSprayDamageType::StaticClass();
							HitCharacter->TakeDamage(0.05, DamageEvent, GetOwnerCharacter()->GetController(), this);
							//V_LOGM(LogReadyOrNot, "Applying Pepperspray damage from %s to %s", *GetOwnerCharacter()->GetName(), *HitCharacter->GetName());
						}
						UMoraleComponent::LowerMoraleOnCharacter(Cast<ACyberneticCharacter>(ImpactResultTrace.GetActor()), AI_CONFIG_GET_FLOAT("PeppersprayMorale.Damage") * DeltaSeconds, "Pepperspray");
						HitCharacter->StartPepperSprayed(this);
					}
						
					if (ACyberneticCharacter* AICharacter = Cast<ACyberneticCharacter>(HitCharacter))
					{
						if (AICharacter->IsPepperSprayed() && SprayTimeOnTarget >= PepperSprayAbuseDebounce)
						{
							if (AICharacter->IsUnjustifiedUseOfForce(GetOwnerCharacter(), this, GetMutableDefault<UPepperSprayDamageType>()))
							{
								AICharacter->ScoringComponent->GivePenalty(AScoringManager::PENALTY_UNAUTHORIZED_FORCE, true);
									
								AICharacter->PlayROEViolateTOCResponse();
							}
						}
					}
						
					//if (!bAnnouncedPeppersprayingSomeone)
					//{
					//	bAnnouncedPeppersprayingSomeone = true;
					//	OwnerCharacter->Server_PlayPVPSpeech("UsePeppersprayOnEnemy", ETeamType::TT_NONE);
					//}

					SprayTimeOnTarget += DeltaSeconds;
				}
					
				//break;
				//}
			}
		}

		if (AmmoComponent->HasResource())
		{
			if (SprayTime > 0.2f)
			{
				//EndPoint = SprayParticleComponent->GetComponentLocation() + SprayParticleComponent->GetComponentRotation().Vector() * SprayDistance * FMath::Clamp(SprayTime * 6.5f, 0.0f, 1.0f);
				//GetWorld()->LineTraceSingleByChannel(ImpactResultTrace, SprayParticleComponent->GetComponentLocation(), EndPoint, ECC_Visibility);
				
				if (ImpactResultTrace.bBlockingHit && ImpactResultTrace.Location != FVector::ZeroVector)
				{
					FTransform SpawnTransform;
					SpawnTransform.SetLocation(ImpactResultTrace.Location);
					SpawnTransform.SetRotation(ImpactResultTrace.Normal.ToOrientationQuat());
					
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleImpact, SpawnTransform);
					
					UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), FMODImpactEvent, SpawnTransform, true);
				}
			}
		
			AmmoComponent->DecreaseResourceByRate(AmmoComponent->GetAmmoUsagePerSecond());
		}

		// Alter the spraying sound effect's pitch as we run out of spray. We should implement the ability to shake the can.
		//const float PitchCurrent = (Ammo->GetNormalizedResource() * (PitchFull - PitchEmpty)) + PitchEmpty;
		//SprayAudioComponent->SetPitchMultiplier(PitchCurrent);

		SprayTime += DeltaSeconds;
	}
	else
	{
		SprayTime = 0.0f;
		SprayTimeOnTarget = 0.0f;
	}

	FMODAudioComp->SetVolume(FMath::Clamp((bSpraying ? SprayTime : 0.0f) * 10.0f, 0.0f, 1.0f));

	if (AmmoComponent->HasResource())
	{
		FMODAudioComp->SetParameter("sprayRelease", bSpraying);
	}
}

void APepperspray::OnItemPrimaryUse()
{
	if (GetOwnerCharacter() && GetOwnerCharacter()->IsLowReady())
		return;
	
	Super::OnItemPrimaryUse();

	StartSpraying();
}

void APepperspray::OnItemPrimaryUseEnd()
{
	Super::OnItemPrimaryUseEnd();

	StopSpraying();
}

void APepperspray::OnRep_Spraying()
{
	if (bSpraying)
	{
		PlaySprayAnimation();
		PlaySprayParticleEffect(AmmoComponent->IsDepleted());
		PlaySpraySoundEffect(AmmoComponent->IsDepleted());
	}
	else
	{
		StopSprayAnimation();
		StopSprayParticleEffect();
		StopSpraySoundEffect();
	}
}

void APepperspray::StartSpraying()
{
	StartSpraying_Internal();

	Server_StartSpraying();
}

void APepperspray::StopSpraying()
{
	StopSpraying_Internal();
	
	Server_StopSpraying();
}

void APepperspray::PlaySpraySoundEffect_Implementation(const bool bRunningOutEffect)
{
	UFMODEvent* ChosenEvent = AmmoComponent->IsDepleted() ? FMODSprayEmptyEvent : bRunningOutEffect ? FMODSprayLowAmmoEvent : FMODSprayEvent;
	FMODAudioComp->SetEvent(ChosenEvent);
	
	if (!FMODAudioComp->IsPlaying())
	{
		FMODAudioComp->Activate();
		FMODAudioComp->Play();
	}
}

void APepperspray::StopSpraySoundEffect_Implementation()
{
	if (FMODAudioComp->StudioInstance)
	{
		FMODAudioComp->StudioInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
	}
}

void APepperspray::PlaySprayParticleEffect_Implementation(const bool bRunningOutEffect)
{
	if (bRunningOutEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(ParticleRunningOut, SprayParticleComponent, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTargetIncludingScale, true, EPSCPoolMethod::None, true);

		UFMODBlueprintStatics::PlayEventAttached(FMODSprayEmptyEvent, GetItemMesh(), NAME_None, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, true, true);
	}
	else
	{
		if (ParticleSprayLoopComponent)
		{
			ParticleSprayLoopComponent->Activate(true);
		}
		else
		{
			ParticleSprayLoopComponent = UGameplayStatics::SpawnEmitterAttached(ParticleSprayLoop, SprayParticleComponent, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTargetIncludingScale, true, EPSCPoolMethod::None, true);
		}

		SprayParticleComponent->Activate();
	}
}

void APepperspray::StopSprayParticleEffect_Implementation()
{
	if (ParticleSprayLoopComponent)
		ParticleSprayLoopComponent->Deactivate();
	
	SprayParticleComponent->Deactivate();
}

void APepperspray::Server_StartSpraying_Implementation()
{
	bSpraying = true;

	PlaySprayParticleEffect(AmmoComponent->IsDepleted());
}

void APepperspray::Server_StopSpraying_Implementation()
{
	bSpraying = false;

	StopSprayParticleEffect();
	StopSpraySoundEffect();
}

void APepperspray::OnLowPeppersprayAmmo(const float CurrentResource)
{
	if (CurrentResource >= 0.0f)
	{
		PlaySprayParticleEffect(true);
	}
}

void APepperspray::OnDepletedPeppersprayAmmo()
{
	StopSpraying();
}

void APepperspray::StartSpraying_Internal()
{
	bSpraying = true;

	OnRep_Spraying();
}

void APepperspray::StopSpraying_Internal()
{
	bSpraying = false;

	OnRep_Spraying();
}

void APepperspray::PlaySprayAnimation()
{
	APlayerCharacter* OwnerCharacter = GetOwnerPlayerCharacter();
	if (OwnerCharacter && AnimationData)
	{
		PlayItemAnimation(AnimationData->FireLoop);
		
		/*
		PlayFPMontage(AnimationData->FireLoop.Gun_FP);
		OwnerCharacter->Play1PMontage(AnimationData->FireLoop.Body_FP);
		OwnerCharacter->Play3PMontage(AnimationData->FireLoop.Body_TP);*/
	}
}

void APepperspray::StopSprayAnimation()
{
	APlayerCharacter* OwnerCharacter = GetOwnerPlayerCharacter();
	if (OwnerCharacter && AnimationData)
	{
		StopFPMontage(AnimationData->FireLoop.Gun_FP);
		OwnerCharacter->StopFPAnimMontage(AnimationData->FireLoop.Body_FP);
		OwnerCharacter->StopTPAnimMontage(AnimationData->FireLoop.Body_TP);
	}
}

bool APepperspray::PlayHolster()
{
	StopSpraying();
	
	return Super::PlayHolster();
}

void APepperspray::SetupPepperspray()
{
	bSpraying = false;
	
	SprayParticleComponent->SetTemplate(ParticleStart);

	SetupAudio();
}

void APepperspray::SetupAudio()
{
	FMODAudioComp->SetEvent(FMODSprayEvent);
}

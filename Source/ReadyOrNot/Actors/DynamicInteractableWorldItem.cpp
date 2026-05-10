// Copyright Void Interactive, 2022


#include "Actors/DynamicInteractableWorldItem.h"
#include "Components/InteractableComponent.h"

ADynamicInteractableWorldItem::ADynamicInteractableWorldItem()
{

	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Static Mesh
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>("ItemMesh");
	ItemMesh->SetSimulatePhysics(false);
	ItemMesh->SetMobility(EComponentMobility::Stationary);
	ItemMesh->OnComponentHit.AddDynamic(this, &ADynamicInteractableWorldItem::OnHit);
	RootComponent = ItemMesh;

	// Impact particle
	ImpactParticle = CreateDefaultSubobject<UParticleSystemComponent>("ImpactParticle");
	ImpactParticle->SetupAttachment(RootComponent);
	ImpactParticle->bAutoActivate = false;

	// Interaction component for toggling item on/off
	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>("InteractableComponent");
	InteractableComponent->RequiredLookAtPercentage = 0.95f;
	InteractableComponent->bDistanceFadeIcon = false;
	InteractableComponent->bImprintIconOnHUDUponInteraction = true;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable","TurnOff"));
	InteractableComponent->ActionSlot1.bCondition = true;
	InteractableComponent->SetupAttachment(GetRootComponent());

	// FMOD audio for impact
	ImpactAudioFMOD = CreateDefaultSubobject<UFMODAudioComponent>("ImpactAudioFMOD");
	ImpactAudioFMOD->SetupAttachment(RootComponent);
	ImpactAudioFMOD->bAutoActivate = false;

	// FMOD audio for interaction
	InteractAudioFMOD = CreateDefaultSubobject<UFMODAudioComponent>("InteractAudioFMOD");
	InteractAudioFMOD->SetupAttachment(RootComponent);
	InteractAudioFMOD->bAutoActivate = false;

	// FMOD audio for item on sound when intact.
	IntactRunningAudioFMOD1 = CreateDefaultSubobject<UFMODAudioComponent>("IntactRunningAudioFMOD");
	IntactRunningAudioFMOD1->SetupAttachment(RootComponent);
	IntactRunningAudioFMOD1->bAutoActivate = false;

	// FMOD audio for item on sound when destroyed.
	DestroyedRunningAudioFMOD = CreateDefaultSubobject<UFMODAudioComponent>("DestroyedRunningAudioFMOD");
	DestroyedRunningAudioFMOD->SetupAttachment(RootComponent);
	DestroyedRunningAudioFMOD->bAutoActivate = false;
}


/*
 * On BeginPlay.
 */
void ADynamicInteractableWorldItem::BeginPlay()
{
	Super::BeginPlay();

	if (ItemMesh->IsSimulatingPhysics())
	{
		ItemMesh->SetCanEverAffectNavigation(false);
	}

	// Toggle on the intact running audio
	if(IntactRunningAudioFMOD1 && bItemOn && !bItemDestroyed)
	{
		IntactRunningAudioFMOD1->Play();
	}
	
	// Toggle on the intact running audio
	if(DestroyedRunningAudioFMOD && bItemOn && bItemDestroyed)
	{
		DestroyedRunningAudioFMOD->Play();
	}

	SwitchMaterials();

	// Ensure component exists
	if(!InteractableComponent)
	{
		return;;
	}

	if(!bItemDestroyed || (bItemDestroyed && bCanToggleIfDestroyed))
	{
		FString TextKey = bItemOn ? "TurnOff": "TurnOn";
		InteractableComponent->ActionSlot1.ActionText = FText::FromStringTable("ActionPromptTable", TextKey);
	}
	else
	{
		InteractableComponent->bEnabled = false;
	}
}

/*
 * Properties to replicate.
 */
// void ADynamicInteractableWorldItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
// {
// 	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//
// 	// DOREPLIFETIME(ADynamicInteractableWorldItem, bItemOn);
// 	// DOREPLIFETIME(ADynamicInteractableWorldItem, bItemDestroyed);
// }


/*
 * On TakeDamage.
 */
float ADynamicInteractableWorldItem::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bItemDestroyed)
		return 0.0f;
	
	Multicast_DestroyItem();
	return 0.0f;
}


/*
 * On Hit.
 */
void ADynamicInteractableWorldItem::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (GetLocalRole() < ROLE_Authority)
		return;

	if (bItemDestroyed)
		return;

	if (OtherActor && OtherActor->GetVelocity().Size2D() > 1500.0f)
	{
		bItemDestroyed = true;

		FRotator DecalRandomRotation = Hit.ImpactNormal.Rotation();
		DecalRandomRotation.Roll += FMath::RandRange(0, 360);
		UDecalComponent* DecalComp = UGameplayStatics::SpawnDecalAttached(PhysicsImpactDecal, FVector(5, PhysicsImpactDecalScale, PhysicsImpactDecalScale), Hit.GetComponent(), Hit.BoneName, Hit.ImpactPoint, DecalRandomRotation, EAttachLocation::KeepWorldPosition);
		if (DecalComp)
		{
			DecalComp->FadeScreenSize = 0.0f;
			DecalComp->SetLifeSpan(1500.0f);
		}
	}
}

/*
 * Interact Implementation
 */
void ADynamicInteractableWorldItem::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator,
	UInteractableComponent* InInteractableComponent)
{

	if(bItemDestroyed && !bCanToggleIfDestroyed)
	{
		return;
	}

	Multicast_ItemStateToggle();
}


void ADynamicInteractableWorldItem::Multicast_ItemStateToggle_Implementation()
{

	if(bItemDestroyed)
	{
		if(bCanToggleIfDestroyed)
		{
			if (InteractAudioFMOD)
			{
				InteractAudioFMOD->Play();
			}

			// Switch the state
			bItemOn = !bItemOn;

			// Change interactable text
			FString TextKey = bItemOn ? "TurnOff": "TurnOn";
			InteractableComponent->ActionSlot1.ActionText = FText::FromStringTable("ActionPromptTable", TextKey);
			
			if(bItemOn)
			{
				if(DestroyedRunningAudioFMOD)
				{
					if(bRestartOnToggle)
					{
						DestroyedRunningAudioFMOD->Play();
					}
					else
					{
						DestroyedRunningAudioFMOD->SetPaused(false);
					}
				}
			}
			else
			{
				if(DestroyedRunningAudioFMOD)
				{
					if(bRestartOnToggle)
					{
						DestroyedRunningAudioFMOD->Stop();
					}
					else
					{
						DestroyedRunningAudioFMOD->SetPaused(true);
					}
				}
			}
		}
	}
	// Not destroyed
	else
	{
		if (InteractAudioFMOD)
		{
			InteractAudioFMOD->Play();
		}

		// Switch the state
		bItemOn = !bItemOn;

		// Change interactable text
		FString TextKey = bItemOn ? "TurnOff": "TurnOn";
		InteractableComponent->ActionSlot1.ActionText = FText::FromStringTable("ActionPromptTable", TextKey);

		if(bItemOn)
		{
			if(IntactRunningAudioFMOD1)
			{
				if(bRestartOnToggle)
				{
					IntactRunningAudioFMOD1->Play();
				}
				else
				{
					IntactRunningAudioFMOD1->SetPaused(false);
				}
			}

			TArray<UPointLightComponent*> PointLights;
			GetComponents(PointLights);
			for (UPointLightComponent* PointLight : PointLights)
			{
				PointLight->SetVisibility(true);
			}
            	
			TArray<USpotLightComponent*> SpotLights;
			GetComponents(SpotLights);
			for (USpotLightComponent* SpotLight : SpotLights)
			{
				SpotLight->SetVisibility(true);
			}
		}
		else
		{
			if(IntactRunningAudioFMOD1)
			{
				if(bRestartOnToggle)
				{
					IntactRunningAudioFMOD1->Stop();
				}
				else
				{
					IntactRunningAudioFMOD1->SetPaused(true);
				}
			}

			TArray<UPointLightComponent*> PointLights;
			GetComponents(PointLights);
			for (UPointLightComponent* PointLight : PointLights)
			{
				PointLight->SetVisibility(false);
			}
            	
			TArray<USpotLightComponent*> SpotLights;
			GetComponents(SpotLights);
			for (USpotLightComponent* SpotLight : SpotLights)
			{
				SpotLight->SetVisibility(false);
			}
		}
	}

	SwitchMaterials();
	
	OnItemStateToggled();
}

/*
 * Switched the materials.
 *
 * Called from a Multicast.
 */
void ADynamicInteractableWorldItem::SwitchMaterials()
{
	
	if(!ItemMesh)
	{
		return;
	}

	
	if(bItemOn)
	{
		if(bItemDestroyed)
		{
			if(ItemMesh && DestroyedOnMaterials.Num() > 0)
			{
				for(int i = 0; i < ItemMesh->GetMaterials().Num(); i++)
				{
					ItemMesh->SetMaterial(i, DestroyedOnMaterials[i]); 
				}
			}
		}
		else
		{
			if(ItemMesh && IntactOnMaterials.Num() > 0)
			{
				for(int i = 0; i < ItemMesh->GetMaterials().Num(); i++)
				{
					ItemMesh->SetMaterial(i, IntactOnMaterials[i]); 
				}
			}
		}
	}
	else
	{
		if(bItemDestroyed)
		{
			if(ItemMesh && DestroyedOffMaterials.Num() > 0)
			{
				for(int i = 0; i < ItemMesh->GetMaterials().Num(); i++)
				{
					ItemMesh->SetMaterial(i, DestroyedOffMaterials[i]); 
				}
			}
		}
		else
		{
			if(ItemMesh && IntactOffMaterials.Num() > 0)
			{
				for(int i = 0; i < ItemMesh->GetMaterials().Num(); i++)
				{
					ItemMesh->SetMaterial(i, IntactOffMaterials[i]); 
				}
			}
		}
	}
}



/*
 * Multicast Destroy Item
 *
 * Called by any client/server to destroy the item on all game instances.
 */ 
void ADynamicInteractableWorldItem::Multicast_DestroyItem_Implementation()
{

	if (bItemDestroyedLocally)
		return;

	bItemDestroyed = true;
	bItemDestroyedLocally = true;

	// Turning off the audio if it
	if(IntactRunningAudioFMOD1)
	{
		IntactRunningAudioFMOD1->Deactivate();
	}

	if(bItemOn && DestroyedRunningAudioFMOD)
	{
		DestroyedRunningAudioFMOD->Play();
	}

	if(!bCanToggleIfDestroyed)
	{
		InteractableComponent->bEnabled = false;
		bItemOn = false;
	}
	
	if (ImpactParticle)
	{
		ImpactParticle->Activate(true);
	}

	
	if (ImpactAudioFMOD)
	{
		ImpactAudioFMOD->Activate(true);
	}

	
	if (ItemMesh)
	{

		if(PostDestructionMesh)
		{
			ItemMesh->SetStaticMesh(PostDestructionMesh);
		}

		SwitchMaterials();
	}

	
	
	TArray<UPointLightComponent*> PointLights;
	GetComponents(PointLights);
	for (UPointLightComponent* PointLight : PointLights)
	{
		PointLight->SetIntensity(0.0f);
	}
	
	TArray<USpotLightComponent*> SpotLights;
	GetComponents(SpotLights);
	for (USpotLightComponent* SpotLight : SpotLights)
	{
		SpotLight->SetIntensity(0.0f);
	}

	// Call function for Blueprint usage
	OnItemDestroyed();
}

// Copyright Void Interactive, 2022

#include "DynamicWorldItem.h"
#include "DamageTypes/StunDamage.h"

void ADynamicWorldItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADynamicWorldItem, bItemDestroyed);
}

ADynamicWorldItem::ADynamicWorldItem()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>("ItemMesh");
	ItemMesh->SetSimulatePhysics(true);
	ItemMesh->SetMobility(EComponentMobility::Stationary);
	ItemMesh->OnComponentHit.AddDynamic(this, &ADynamicWorldItem::OnHit);
	RootComponent = ItemMesh;

	ImpactParticle = CreateDefaultSubobject<UParticleSystemComponent>("ImpactParticle");
	ImpactParticle->SetupAttachment(RootComponent);
	ImpactParticle->bAutoActivate = false;

	ImpactAudioFMOD = CreateDefaultSubobject<UFMODAudioComponent>("ImpactAudioFMOD");
	ImpactAudioFMOD->SetupAttachment(RootComponent);
	ImpactAudioFMOD->bAutoActivate = false;
}

void ADynamicWorldItem::BeginPlay()
{
	Super::BeginPlay();

	if (ItemMesh->IsSimulatingPhysics())
	{
		ItemMesh->SetCanEverAffectNavigation(false);
	}
}

float ADynamicWorldItem::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (GetLocalRole() < ROLE_Authority)
		return 0.0f;

	if (bItemDestroyed)
		return 0.0f;

	TSubclassOf<UDamageType> DamageType = DamageEvent.DamageTypeClass;
	TSubclassOf<UStunDamage> StunDamage = TSubclassOf<UStunDamage>(DamageType);

	if (StunDamage && StunDamage.GetDefaultObject() && !StunDamage.GetDefaultObject()->bBreaksDestructibles)
	{
		return 0.0f; // the damage type inflicted doesn't break destructible objects
	}

	Multicast_DestroyItem();
	
	return 0.0f;
}

void ADynamicWorldItem::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (GetLocalRole() < ROLE_Authority)
		return;

	if (bItemDestroyed)
		return;

	if (OtherActor && OtherActor->GetVelocity().Size2D() > 1500.0f)
	{
		Multicast_DestroyItem();

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

void ADynamicWorldItem::OnRep_ItemDestroyed()
{
	Multicast_DestroyItem_Implementation();
}

void ADynamicWorldItem::Multicast_DestroyItem_Implementation()
{
	if (bItemDestroyedLocally)
		return;

	bItemDestroyed = true;
	bItemDestroyedLocally = true;

	if (ImpactParticle)
	{
		ImpactParticle->Activate(true);
	}

	if (ItemMesh)
	{
		ItemMesh->SetStaticMesh(PostImpactMesh);

		if (PostImpactMaterial)
		{
			ItemMesh->SetMaterial(0, PostImpactMaterial);
		}
	}

	if (ImpactAudioFMOD)
	{
		ImpactAudioFMOD->Activate(true);
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

	OnItemDestroyed();
}
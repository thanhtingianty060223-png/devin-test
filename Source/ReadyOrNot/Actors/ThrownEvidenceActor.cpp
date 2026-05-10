// Void Interactive, 2020

#include "Actors/ThrownEvidenceActor.h"

#include "Audio/RoNSoundData.h"

AThrownEvidenceActor::AThrownEvidenceActor()
{
	PrimaryActorTick.bCanEverTick = true;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCoponent0"));
	BoxComponent->SetCanEverAffectNavigation(false);
	BoxComponent->bNavigationRelevant = false;
	BoxComponent->SetBoxExtent(FVector(1.5f, 25.0f, 10.0f));
	BoxComponent->SetCollisionProfileName("PhysicsItem");
	BoxComponent->SetIsReplicated(false);
	BoxComponent->SetUseCCD(true);

	BoxComponent->SetNotifyRigidBodyCollision(true);
	BoxComponent->OnComponentHit.AddDynamic(this, &AThrownEvidenceActor::OnBoxHit);

	NetUpdateFrequency = 10.0f;
	MinNetUpdateFrequency = 1.0f;

	bAlwaysRelevant = true;
	bReplicates = true;
	AActor::SetReplicateMovement(true);
	
	SetRootComponent(BoxComponent);
}

void AThrownEvidenceActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AThrownEvidenceActor, Rep_Location);
	DOREPLIFETIME(AThrownEvidenceActor, Rep_Rotation);
}

void AThrownEvidenceActor::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	BoxComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	BoxComponent->SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Ignore);
    BoxComponent->SetSimulatePhysics(true);
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// Only change if bounds are different, to avoid updating the bounds and marking this component dirty every frame
	if (BoxComponent->GetUnscaledBoxExtent() != FVector(1.5f, 25.0f, 10.0f))
		BoxComponent->SetBoxExtent(FVector(1.5f, 25.0f, 10.0f));
	
	//DrawDebugBox(GetWorld(), BoxComponent->GetComponentLocation(), BoxComponent->GetScaledBoxExtent(), BoxComponent->GetComponentQuat(), FColor::White, false, DeltaTime + 0.05f, 0 , 1);
}

bool AThrownEvidenceActor::CanIssueCommand_Implementation() const
{
	return true;
}

AActor* AThrownEvidenceActor::GetCommandActor_Implementation() const
{
	return const_cast<AThrownEvidenceActor*>(this);
}

void AThrownEvidenceActor::OnBoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OwningItem)
		return;

	// Once collected as evidence, don't try to play a hit sound
	if (OwningItem->IsPendingKillPending())
		return;

	// don't trigger this on invisible components (and therefore hear a double sound sometimes)
	if (!OwningItem->GetItemMesh()->IsVisible())
		return;

	if (UFMODBlueprintStatics::EventInstanceIsValid(HitEventInstance))
		return;

	const float NormalImpulseSize = NormalImpulse.Size();
	
	//LOG_NUMBER(NormalImpulseSize);

	if (NormalImpulseSize < 200.0f)
		return;

	if (OwningItem->SoundData && NormalImpulseSize >= OwningItem->SoundData->PhysicsImpactMinimumVelocity)
	{
		HitEventInstance = UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), OwningItem->SoundData->PhysicsImpact, GetActorTransform(), true);
		if (HitEventInstance.Instance)
		{
			HitEventInstance.Instance->setParameterByName("Material", UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()));
			HitEventInstance.Instance->setParameterByName("Velocity", NormalImpulseSize);
		}
	}
}

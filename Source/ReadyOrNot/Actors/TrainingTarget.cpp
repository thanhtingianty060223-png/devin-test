// Copyright Void Interactive, 2023


#include "TrainingTarget.h"

#include "DamageTypes/BulletDamageType.h"
#include "Kismet/KismetMathLibrary.h"

ATrainingTarget::ATrainingTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	TargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetMesh"));
	SetRootComponent(TargetMesh);

	SuccessBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SuccessBox"));
	SuccessBox->InitBoxExtent(FVector(27.619724f,1.850504f,38.340359f));
	SuccessBox->SetRelativeLocation(FVector(0.0f, -1.496463f, 116.637634f));
	SuccessBox->SetGenerateOverlapEvents(false);
	SuccessBox->CanCharacterStepUpOn = ECB_No;
	SuccessBox->SetCollisionProfileName("NoCollision");
	SuccessBox->SetupAttachment(RootComponent);

	FailureBox = CreateDefaultSubobject<UBoxComponent>(TEXT("FailureBox"));
	FailureBox->InitBoxExtent(FVector(0.0f,0.0f,0.0f));
	FailureBox->SetGenerateOverlapEvents(false);
	FailureBox->CanCharacterStepUpOn = ECB_No;
	FailureBox->SetCollisionProfileName("NoCollision");
	FailureBox->SetupAttachment(RootComponent);
}

void ATrainingTarget::BeginPlay()
{
	Super::BeginPlay();

	//##UE5.3UPGRADE##
	// Setup delegate for point damage events, such as bullet hits
	//OnTakePointDamage.AddDynamic(this, &ATrainingTarget::OnPointDamage);

	// Setup delegate for radial damage events, such as grenade hits
	//OnTakeRadialDamage.AddDynamic(this, &ATrainingTarget::OnRadialDamage);
	//##UE5.3UPGRADE##
}

void ATrainingTarget::OnPointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation, UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
	// If DamageType is a not bullet, damage doesn't count
	if (!Cast<UBulletDamageType>(DamageType))
		return;

	// If a failure box is shot, doesn't count
	if (HitAnyBox(*FailureBox, HitLocation))
		return;

	// If a success box is shot, count it
	if (HitAnyBox(*SuccessBox, HitLocation))
		OnSuccessfulShot.Broadcast(this);
}

void ATrainingTarget::OnRadialDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, FVector Origin,	FHitResult HitInfo, AController* InstigatedBy, AActor* DamageCauser)
{
	// If DamageType is a not stun, damage doesn't count
	if (!Cast<UStunDamage>(DamageType))
		return;

	OnGrenadeHit.Broadcast(this);
}

bool ATrainingTarget::HitAnyBox(const UBoxComponent& BaseBox, const FVector HitLocation)
{
	// Check if the hit location is inside the base box
	if (UKismetMathLibrary::IsPointInBoxWithTransform(HitLocation, BaseBox.GetComponentTransform(), BaseBox.GetScaledBoxExtent()))
		return true;

	// Check if the hit location is inside any of the box's children
	TArray<USceneComponent*> ChildrenComponents;
	BaseBox.GetChildrenComponents(true, ChildrenComponents);

	for (USceneComponent* ChildComponent : ChildrenComponents)
	{
		const UBoxComponent* ChildBox = Cast<UBoxComponent>(ChildComponent);
		if (!ChildBox)
			continue;

		if (UKismetMathLibrary::IsPointInBoxWithTransform(HitLocation, ChildBox->GetComponentTransform(), ChildBox->GetScaledBoxExtent()))
			return true;
	}

	return false;
}

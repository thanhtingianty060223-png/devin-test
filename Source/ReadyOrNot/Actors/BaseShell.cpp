// Copyright Void Interactive, 2022

#include "BaseShell.h"

ABaseShell::ABaseShell()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 9999.0f;

	bEverAllowTick = false;

	ShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shell"));
	ShellMesh->SetSimulatePhysics(true);
	ShellMesh->SetCollisionProfileName("Item");
	ShellMesh->SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Ignore);
	ShellMesh->SetRelativeRotation(FRotator(90.0f, 90.0f, 0.0f));
	ShellMesh->SetRelativeLocation(FVector(0, 0, 3.7));
	ShellMesh->SetNotifyRigidBodyCollision(true);
	ShellMesh->SetCanEverAffectNavigation(false);
	ShellMesh->SetGenerateOverlapEvents(false);
	ShellMesh->bNavigationRelevant = false;
	SetRootComponent(ShellMesh);

	bReplicates = false;
	
	AActor::SetReplicateMovement(false);
}

void ABaseShell::BeginPlay()
{
	Super::BeginPlay();
	
	ShellMesh->OnComponentHit.AddDynamic(this, &ABaseShell::OnHit);
	
	for (int32 i = 0; i < ShellMesh->GetMaterials().Num(); i++)
	{
		MID_ShellMesh.Add(ShellMesh->CreateAndSetMaterialInstanceDynamicFromMaterial(i, ShellMesh->GetMaterial(i)));
	}
}

void ABaseShell::StopPhysics()
{
	ShellMesh->SetSimulatePhysics(false);
}

void ABaseShell::DisableWeaponFOV()
{
	for (int32 i = 0; i < ShellMesh->GetMaterials().Num(); i++)
	{
		if (UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(ShellMesh->GetMaterial(i)))
		{
			DynMat->SetScalarParameterValue("DisableWeaponFOV", 1);
		}
	}
}

void ABaseShell::PropelFromGun(ABaseMagazineWeapon* Weapon)
{
	ShellMesh->SetSimulatePhysics(false);
	
	ShellMesh->SetStaticMesh(Weapon->ShellMesh);
	ShellBounceFMODAudio = Weapon->ShellBounceFMODAudio;
	
	CreationTime = GetWorld()->GetTimeSeconds();
	ShellMesh->SetSimulatePhysics(true);

	float RightVel = 0.0f;

	// positive values to the right, negative values to the left (-1.0f, 1.0f)
	if (Weapon->GetOwner())
		RightVel = FVector::DotProduct(Weapon->GetOwner()->GetActorRightVector(), Weapon->GetOwner()->GetVelocity());

	SetActorLocation(Weapon->ShellSpawn->GetComponentLocation());
	AddActorWorldRotation(FRotator(FMath::RandRange(-10, 10), FMath::RandRange(-10, 10), 0));
	//SetActorRotation(FRotator(90.0f, -180.0f, 0));
	
	ShellMesh->SetWorldScale3D(Weapon->GetActorScale3D());
	
	if (RightVel < 200.0f)
		RightVel = 200.0f;
	
	ShellMesh->AddImpulse(Weapon->ShellSpawn->GetComponentRotation().Vector() * RightVel * FMath::RandRange(1.0f, 3.0f), NAME_None, true);
	ShellMesh->AddAngularImpulseInRadians(FVector(FMath::RandRange(0, 360)), NAME_None, true);

	// activate the particle effect here
	Weapon->ShellParticle->Activate(true);

	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ABaseShell::StopPhysics, 5.0f);
	UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &ABaseShell::DisableWeaponFOV, 0.3f);
}

void ABaseShell::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const float NormalImpulseSize = NormalImpulse.Size();
	if (NormalImpulseSize < 1.0f)
		return;
	
	//LOG_NUMBER(NormalImpulseSize);

	const AReadyOrNotCharacter* OwnerChar = Cast<AReadyOrNotCharacter>(GetOwner());
	if (OwnerChar && !OwnerChar->IsLocalPlayer())
		return;
	
	if (ShellBounceFMODAudio)
	{
		if (!UFMODBlueprintStatics::EventInstanceIsValid(ShellHitEventInst))
		{
			ShellHitEventInst = UFMODBlueprintStatics::PlayEventAtLocation(this, ShellBounceFMODAudio, ShellMesh->GetComponentTransform(), false);
			
			const UPhysicalMaterial* HitPhysMat = Hit.PhysMaterial.Get();
			const EPhysicalSurface HitSurfaceType = UPhysicalMaterial::DetermineSurfaceType(HitPhysMat);

			const int32 Surface = HitSurfaceType;
			
			UFMODBlueprintStatics::EventInstanceSetParameter(ShellHitEventInst, "Surface", Surface);
			UFMODBlueprintStatics::EventInstancePlay(ShellHitEventInst);
		}
	}
}


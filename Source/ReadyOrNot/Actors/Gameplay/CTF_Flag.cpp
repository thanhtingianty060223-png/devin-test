// Void Interactive, 2020

#include "CTF_Flag.h"

#include "GameModes/CaptureTheFlagGM.h"
#include "GameModes/CaptureTheFlagGS.h"

#include "Components/ObjectiveMarkerComponent.h"
#include "lib/ReadyOrNotFunctionLibrary.h"

ACTF_Flag::ACTF_Flag()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bFindCameraComponentWhenViewTarget = false;

	SetCanBeDamaged(false);

	bReplicates = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(SceneComponent);
	
	FlagMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlagMeshComponent"));
	#if WITH_EDITOR
	FlagMeshComponent->SetStaticMesh(Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(),nullptr, TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"))));
	FlagMeshComponent->SetMaterial(0, Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(),nullptr, TEXT("Material'/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial'"))));
	FlagMeshComponent->SetRelativeScale3D(FVector(0.25f, 0.25f, 2.5f));
	#endif
	FlagMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlagMeshComponent->SetRelativeLocation(FVector::ZeroVector);
	FlagMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
	FlagMeshComponent->SetupAttachment(RootComponent);
	
	CaptureBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CaptureBoxComponent"));
	CaptureBoxComponent->InitBoxExtent(FVector(30.0f, 30.0f, 150.0f));
	CaptureBoxComponent->SetRelativeLocation(FVector::ZeroVector);
	CaptureBoxComponent->SetRelativeRotation(FRotator::ZeroRotator);
	CaptureBoxComponent->SetCollisionProfileName("Trigger");
	CaptureBoxComponent->SetGenerateOverlapEvents(true);
	CaptureBoxComponent->SetSimulatePhysics(false);
	CaptureBoxComponent->SetEnableGravity(false);
	CaptureBoxComponent->bIgnoreRadialImpulse = true;
	CaptureBoxComponent->bApplyImpulseOnDamage = false;
	CaptureBoxComponent->bReplicatePhysicsToAutonomousProxy = false;
	CaptureBoxComponent->OnComponentBeginOverlap.AddDynamic(this, &ACTF_Flag::OnFlagBeginOverlap);
	CaptureBoxComponent->SetupAttachment(RootComponent);
	
	ObjectiveMarkerComponent = CreateDefaultSubobject<UObjectiveMarkerComponent>(TEXT("ObjectiveMarkerComponent"));
}

void ACTF_Flag::BeginPlay()
{
	Super::BeginPlay();

	ACaptureTheFlagGM* CTFGM = Cast<ACaptureTheFlagGM>(UGameplayStatics::GetGameMode(this));
	ACaptureTheFlagGS* CTFGS = Cast<ACaptureTheFlagGS>(UGameplayStatics::GetGameState(this));
	
	if (!CTFGM || !CTFGS)
	{
		Destroy();

		return;
	}

	ResetFlagTransforms();
}

void ACTF_Flag::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		if (ACaptureTheFlagGS* CTFGS = Cast<ACaptureTheFlagGS>(UGameplayStatics::GetGameState(this)))
		{
			if (CTFGS->bFlagCaptured)
			{
				if (CTFGS->FlagBearer && !CTFGS->FlagBearer->IsDeadNotUnconscious() && !IsAttachedTo(CTFGS->FlagBearer))
				{
					AttachToActor(CTFGS->FlagBearer, FAttachmentTransformRules::SnapToTargetIncludingScale, BoneToAttach);

					#if WITH_EDITOR
					ULog::Warning(GetName() + " was detached for some reason. Forcibly attaching " + GetName() + " back to the flag bearer.");
					#endif
				}
			}
		}
	}
}

void ACTF_Flag::OnFlagBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ACaptureTheFlagGS* CTFGS = Cast<ACaptureTheFlagGS>(UGameplayStatics::GetGameState(this)))
	{
		if (!CTFGS->bFlagCaptured && OtherActor->IsValidLowLevel() && OtherActor != this)
		{
			if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(OtherActor))
			{
				if (!PlayerCharacter->IsDeadNotUnconscious())
				{					
					if (ACaptureTheFlagGM* CTFGM = Cast<ACaptureTheFlagGM>(UGameplayStatics::GetGameMode(this)))
					{
						if (GetNetMode() == NM_ListenServer)
						{
							if (OtherActor == UGameplayStatics::GetPlayerCharacter(this, 0))
							{
								SetActorHiddenInGame(true);
							}
						}
						
						CTFGM->CaptureFlag(this, PlayerCharacter);
					}
				}
			}
		}
	}
}

void ACTF_Flag::ResetFlagTransforms()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	AActor* FlagBearer = nullptr;
	if (ACaptureTheFlagGS* CTFGS = Cast<ACaptureTheFlagGS>(UGameplayStatics::GetGameState(this)))
		FlagBearer = CTFGS->FlagBearer;
	
	SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, UReadyOrNotFunctionLibrary::FindNearestFloor(this, {FlagBearer})));
	SetActorRotation(FRotator::ZeroRotator);
	
	SetActorHiddenInGame(false);
}

float ACTF_Flag::FindNearestFloor()
{
	FHitResult HitResult;
	FVector Start = GetActorLocation();
	FVector End = GetActorLocation() - FVector(0.0f, 0.0f, 1000000.0f);
	
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(this);
	
	if (ACaptureTheFlagGS* CTFGS = Cast<ACaptureTheFlagGS>(UGameplayStatics::GetGameState(this)))
	{
		CollisionQueryParams.AddIgnoredActor(CTFGS->FlagBearer);
	}

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, CollisionQueryParams))
	{
		return HitResult.Location.Z;
	}

	return GetActorLocation().Z;
}

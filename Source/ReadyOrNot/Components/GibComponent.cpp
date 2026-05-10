// Copyright Void Interactive, 2022

#include "Components/GibComponent.h"
#include "SkeletalRenderPublic.h"

UGibComponent::UGibComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UGibComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGibComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (BloodData)
	{
		GibHead = BloodData->GibData.GibHead;
		GibArms = BloodData->GibData.GibArms;
		GibLegs = BloodData->GibData.GibLegs;
		BoneHead = BloodData->GibData.BoneHead;
		BoneArms = BloodData->GibData.BoneArms;
		BoneLegs = BloodData->GibData.BoneLegs;
	}
}

void UGibComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UGibComponent::Gib(EGibAreas GibArea, FVector Impulse)
{
	if (GibArea == EGibAreas::GA_None || GibbedAreas.Contains(GibArea))
		return;
	
	if (!BodyMesh)
		return;

	if (GetGibMesh(GibArea))
	{
		if (AStaticMeshActor* GibActor = GetWorld()->SpawnActor<AStaticMeshActor>())
		{
			GibActor->SetMobility(EComponentMobility::Movable);
			GibActor->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, GetGibAttachSocket(GibArea));
			GibActor->GetStaticMeshComponent()->SetStaticMesh(GetGibMesh(GibArea));
			GibActor->GetStaticMeshComponent()->SetCollisionObjectType(ECC_PhysicsBody);
			GibActor->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
			GibActor->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
			GibActor->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_PROJECTILE, ECR_Ignore);
			GibActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			GibActor->GetStaticMeshComponent()->SetSimulatePhysics(true);
			GibActor->GetStaticMeshComponent()->AddImpulse(Impulse);
			GibActor->GetStaticMeshComponent()->SetCanEverAffectNavigation(false);
			GibActor->GetStaticMeshComponent()->SetReceivesDecals(false);
		}
	}

	if (GetBoneMesh(GibArea))
	{
		if (AStaticMeshActor* BoneActor = GetWorld()->SpawnActor<AStaticMeshActor>())
		{
			BoneActor->SetMobility(EComponentMobility::Movable);
			BoneActor->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, GetBoneAttachSocket(GibArea));
			BoneActor->GetStaticMeshComponent()->SetStaticMesh(GetBoneMesh(GibArea));
			BoneActor->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
			BoneActor->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
			BoneActor->GetStaticMeshComponent()->SetCanEverAffectNavigation(false);
			BoneActor->GetStaticMeshComponent()->SetReceivesDecals(false);
		}
	}

	FName GibBone = FName(GetGibBone(GibArea));
	BodyMesh->SetAllBodiesBelowSimulatePhysics(GibBone, true);
	BodyMesh->HideBoneByName(GibBone, EPhysBodyOp::PBO_None);
	if (FaceMesh)
	{
		FaceMesh->HideBoneByName(GibBone, EPhysBodyOp::PBO_None);
	}

	BodyMesh->BreakConstraint(FVector::ZeroVector, FVector::ZeroVector, GetConstraintName(GibArea));

	GibbedAreas.AddUnique(GibArea);
	OnGib.Broadcast(GibBone);
}

bool UGibComponent::IsBoneInGibArea(FName Bone, EGibAreas GibArea)
{
	if (!BodyMesh)
		return false;
	
	FName GibAreaBone = FName(GetGibBone(GibArea));
	return Bone == GibAreaBone || BodyMesh->BoneIsChildOf(Bone, GibAreaBone);
}

UStaticMesh* UGibComponent::GetGibMesh(EGibAreas GibAreas)
{
	switch (GibAreas)
	{
	case EGibAreas::GA_None: break;
	case EGibAreas::GA_Head: return GibHead;
	case EGibAreas::GA_LeftArm: return GibArms;
	case EGibAreas::GA_RightArm: return GibArms;
	case EGibAreas::GA_LeftLeg: return GibLegs;
	case EGibAreas::GA_RightLeg: return GibLegs;
	}
	return nullptr;
}

UStaticMesh* UGibComponent::GetBoneMesh(EGibAreas GibAreas)
{
	switch (GibAreas)
	{
	case EGibAreas::GA_None: break;
	case EGibAreas::GA_Head: return BoneHead;
	case EGibAreas::GA_LeftArm: return BoneArms;
	case EGibAreas::GA_RightArm: return BoneArms;
	case EGibAreas::GA_LeftLeg: return BoneLegs;
	case EGibAreas::GA_RightLeg: return BoneLegs;
	}
	return nullptr;
}

FName UGibComponent::GetGibAttachSocket(EGibAreas GibAreas)
{
	switch (GibAreas)
	{
	case EGibAreas::GA_None: break;
	case EGibAreas::GA_Head: return "gib_head";
	case EGibAreas::GA_LeftArm: return "gib_lowerarm_LE";
	case EGibAreas::GA_RightArm: return "gib_lowerarm_RI";
	case EGibAreas::GA_LeftLeg: return "gib_calf_LE";
	case EGibAreas::GA_RightLeg: return "gib_calf_RI";
	}
	return NAME_None;
}

FName UGibComponent::GetBoneAttachSocket(EGibAreas GibAreas)
{
	switch (GibAreas)
	{
	case EGibAreas::GA_None: break;
	case EGibAreas::GA_Head: return "gib_neck";
	case EGibAreas::GA_LeftArm: return "gib_upperarm_LE";
	case EGibAreas::GA_RightArm: return "gib_upperarm_RI";
	case EGibAreas::GA_LeftLeg: return "gib_thigh_LE";
	case EGibAreas::GA_RightLeg: return "gib_thigh_RI";
	}
	return NAME_None;
}

FName UGibComponent::GetConstraintName(EGibAreas GibAreas)
{
	if (!BodyMesh)
		return NAME_None;

	for (FConstraintInstance* Constraint : BodyMesh->Constraints)
	{
		switch (GibAreas)
		{
		case EGibAreas::GA_None: break;
		case EGibAreas::GA_Head:
			if (Constraint->ConstraintBone1 == "head" && Constraint->ConstraintBone2 == "neck_1")
				return Constraint->JointName;
			break;
		case EGibAreas::GA_LeftArm:
			if (Constraint->ConstraintBone1 == "lowerarm_LE" && Constraint->ConstraintBone2 == "upperarm_LE")
				return Constraint->JointName;
			break;
		case EGibAreas::GA_RightArm:
			if (Constraint->ConstraintBone1 == "lowerarm_RI" && Constraint->ConstraintBone2 == "upperarm_RI")
				return Constraint->JointName;
			break;
		case EGibAreas::GA_LeftLeg:
			if (Constraint->ConstraintBone1 == "calf_LE" && Constraint->ConstraintBone2 == "thigh_LE")
				return Constraint->JointName;
			break;
		case EGibAreas::GA_RightLeg:
			if (Constraint->ConstraintBone1 == "calf_RI" && Constraint->ConstraintBone2 == "thigh_RI")
				return Constraint->JointName;
			break;
		}
	}
	return NAME_None;
}

FString UGibComponent::GetHideBone(EGibAreas GibAreas)
{
	switch (GibAreas)
	{
		case EGibAreas::GA_None: return "";
		case EGibAreas::GA_Head: return "head";
		case EGibAreas::GA_LeftArm: return "upperarm_LE";
		case EGibAreas::GA_RightArm: return "upperarm_RI";
		case EGibAreas::GA_LeftLeg: return "thigh_LE";
		case EGibAreas::GA_RightLeg: return "thigh_RI";
	}
	
	return "";
}

FString UGibComponent::GetGibBone(EGibAreas GibAreas)
{
	switch (GibAreas)
	{
		case EGibAreas::GA_None: return "";
		case EGibAreas::GA_Head: return "neck_1";
		case EGibAreas::GA_LeftArm: return "lowerarm_LE";
		case EGibAreas::GA_RightArm: return "lowerarm_RI";
		case EGibAreas::GA_LeftLeg: return "calf_LE";
		case EGibAreas::GA_RightLeg: return "calf_RI";
	}
	
	return "";
}


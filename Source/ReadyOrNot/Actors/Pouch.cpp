// Copyright Void Interactive, 2017

#include "Pouch.h"
#include "ReadyOrNot.h"
#include "Net/UnrealNetwork.h"




APouch::APouch()
{
	MagazineComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Magazine"));
	MagazineComponent->SetOwnerNoSee(true);
	MagazineComponent->SetupAttachment(GetSkeletalMeshComponent(), MagazineSocket);

	GetSkeletalMeshComponent()->SetOwnerNoSee(true);
	bReplicates = true;
}

void APouch::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APouch, bShowMagazine);
	DOREPLIFETIME(APouch, AttachToComp);
	DOREPLIFETIME(APouch, AttachToSocket);
}

void APouch::AttachPouch(USceneComponent* SceneComp /*= nullptr*/, FName SocketName /*= NAME_None*/)
{
	AttachToComp = SceneComp;
	AttachToSocket = SocketName;
	GetSkeletalMeshComponent()->AttachToComponent(AttachToComp, FAttachmentTransformRules::SnapToTargetIncludingScale, AttachToSocket);
}

void APouch::ShowMagazine()
{
	MagazineComponent->SetVisibility(true);
	bShowMagazine = true;
}

void APouch::HideMagazine()
{
	MagazineComponent->SetVisibility(false);
	bShowMagazine = false;
}

void APouch::SetMesh(USkeletalMesh* NewMesh)
{
	MagazineComponent->SetSkeletalMesh(NewMesh);
}

void APouch::OnRep_UpdateVisibility()
{
	MagazineComponent->SetVisibility(bShowMagazine);
}

void APouch::OnRep_Attach()
{
	if (AttachToComp)
	{
		GetSkeletalMeshComponent()->AttachToComponent(AttachToComp, FAttachmentTransformRules::SnapToTargetIncludingScale, AttachToSocket);
	}
}

// Copyright Void Interactive, 2017

#include "BaseMagPouch.h"
#include "ReadyOrNot.h"




ABaseMagPouch::ABaseMagPouch()
{
	MagazineMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MagazineMesh"));
	MagazineMesh->SetupAttachment(ItemMesh, MagSocket);
	MagazineMesh->SetOwnerNoSee(true);
}

void ABaseMagPouch::BeginPlay()
{
	Super::BeginPlay();

	MagazineMesh->AttachToComponent(ItemMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, MagSocket);
}

void ABaseMagPouch::OpenPouch()
{
	ItemMesh->PlayAnimation(OpenPouchAnim, false);
}

void ABaseMagPouch::ClosePouch()
{
	ItemMesh->PlayAnimation(ClosedPouchAnim, false);
}

void ABaseMagPouch::ShowMagazine()
{
	MagazineMesh->SetVisibility(true);
}

void ABaseMagPouch::HideMagazine()
{
	MagazineMesh->SetVisibility(false);
}

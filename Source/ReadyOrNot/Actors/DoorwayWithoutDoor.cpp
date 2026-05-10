// Void Interactive, 2020

#include "DoorwayWithoutDoor.h"
#include "Components/DoorwayComponent.h"

ADoorwayWithoutDoor::ADoorwayWithoutDoor()
{
	Doorway = CreateDefaultSubobject<UDoorwayComponent>(TEXT("Doorway"));
	SetRootComponent(Doorway);
	Doorway->SetWorldScale3D(FVector(0.1f, 1.0f, 1.0f));
	Doorway->ComponentTags.AddUnique("NoCover");
	Doorway->SetMobility(EComponentMobility::Static);
	Doorway->AreaClass = nullptr;
	Doorway->CanCharacterStepUpOn = ECB_No;
	Doorway->SetCollisionObjectType(ECC_DOORWAY);
	Doorway->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Doorway->SetCollisionResponseToAllChannels(ECR_Ignore);
	Doorway->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
	Doorway->SetCollisionResponseToChannel(ECC_DOORWAY, ECR_Block);
	Doorway->SetCollisionResponseToChannel(ECC_VOLUME, ECR_Overlap);
	Doorway->SetCanEverAffectNavigation(false);
	Doorway->SetGenerateOverlapEvents(false);
	Doorway->bNavigationRelevant = false;
	Doorway->SetBoxExtent(FVector(10.0f, 65.0f, 114.0f));

	bFindCameraComponentWhenViewTarget = false;
}

FVector ADoorwayWithoutDoor::GetDoorSize() const
{
	return IsValid(Doorway) ? Doorway->GetUnscaledBoxExtent() : FVector::ZeroVector;
}

bool ADoorwayWithoutDoor::IsPointInFrontOfDoorway(const FVector Point) const
{
	if (!IsValid(Doorway))
		return false;

	return FVector::DotProduct(Doorway->GetForwardVector(), (Point - Doorway->GetComponentLocation()).GetSafeNormal2D()) > 0.0f;
}

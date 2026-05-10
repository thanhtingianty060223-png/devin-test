// Copyright Void Interactive, 2017

#include "PickupMagazineActor.h"
#include "ReadyOrNot.h"

#include "BaseMagazineWeapon.h"

#include "Characters/PlayerCharacter.h"

#include "Components/ObjectiveMarkerComponent.h"
#include "Components/InteractableComponent.h"
#include "Components/InventoryComponent.h"

APickupMagazineActor::APickupMagazineActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0167f;

	bReplicates = true;
	
	SetRootComponent(StaticMesh);
	StaticMesh->SetVisibility(true);
	StaticMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	StaticMesh->OnComponentHit.AddDynamic(this, &APickupMagazineActor::OnComponentHit);
	StaticMesh->SetNotifyRigidBodyCollision(true);

	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "PickupMagazine"));
	InteractableComponent->ShowPromptAtDistance = 200.0f;
	InteractableComponent->AnimatedIconName = "Pickup Magazine";
	InteractableComponent->SetupAttachment(GetRootComponent());

	ObjectiveMarkerComponent->bEnabled = false;
}

void APickupMagazineActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	InteractableComponent->SetAnimatedIconName(DetermineAnimatedIcon_Implementation());
	InteractableComponent->ActionSlot1.bCondition = CameFromWeapon && !CameFromWeapon->bHasBeenCleared && InteractableComponent->AnimatedIconName != NAME_None;
}

void APickupMagazineActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APickupMagazineActor, MagazineLabel);
	DOREPLIFETIME(APickupMagazineActor, CameFromWeapon);
	DOREPLIFETIME(APickupMagazineActor, MagazineData);
}

void APickupMagazineActor::ActorPickedUp(AActor* InPickupInstigator)
{
	Super::ActorPickedUp(InPickupInstigator);

	Server_Pickup(InPickupInstigator);
}

void APickupMagazineActor::Server_Pickup_Implementation(AActor* InPickupInstigator)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(InPickupInstigator);
	if (!pc)
	{
		return;
	}

	ABaseMagazineWeapon* bmw = GetValidWeaponForPickerUpper(pc);
	if (bmw == nullptr)
	{
		return;
	}

	bmw->Server_AddMagazine_Implementation(MagazineData);
	
	Destroy();
}

ABaseMagazineWeapon* APickupMagazineActor::GetValidWeaponForPickerUpper(APlayerCharacter* PlayerCharacter)
{
	if (!PlayerCharacter)
		return nullptr;
	
	ABaseMagazineWeapon* CurrentWeapon = Cast<ABaseMagazineWeapon>(PlayerCharacter->GetEquippedItem());
	if (CurrentWeapon && CurrentWeapon->MagazineLabel == MagazineLabel)
	{
		return CurrentWeapon;
	}

	// Check inventory items
	TArray<ABaseItem*> InventoryItems = PlayerCharacter->GetInventoryComponent()->GetInventoryItems();
	for (int32 i = 0; i < InventoryItems.Num(); i++)
	{
		ABaseMagazineWeapon* InventoryWeapon = Cast<ABaseMagazineWeapon>(InventoryItems[i]);
		if (InventoryWeapon && InventoryWeapon->MagazineLabel == MagazineLabel)
		{
			return InventoryWeapon;
		}
	}

	return nullptr;
}

void APickupMagazineActor::SetWeapon(ABaseMagazineWeapon* Weapon)
{
	CameFromWeapon = Weapon;
	if (Weapon)
	{
		MagazineData = Weapon->GetCurrentMagazine();
		MagazineLabel = Weapon->MagazineLabel;

		// Magazine technically also contains bullet in chamber, remove it
		MagazineData.Ammo = FMath::Max(MagazineData.Ammo - 1, 0);
	}
	else
	{
		Destroy();
	}
}

void APickupMagazineActor::Multicast_SetWeapon_Implementation(ABaseMagazineWeapon* Weapon)
{
	if (!Weapon) 
		return;

	StaticMesh->SetStaticMesh(Weapon->Mag_01_Comp->GetStaticMesh());
	StaticMesh->SetSimulatePhysics(true);
	StaticMesh->SetUseCCD(true); // Helps to stop magazines passing through the world
	StaticMesh->IgnoreActorWhenMoving(Weapon, true);
	StaticMesh->IgnoreActorWhenMoving(Weapon->GetOwner(), true);
	StaticMesh->SetPhysicsLinearVelocity(Weapon->CurrentMag01Velocity * 250.0f, true);

	DroppedMagazineHitEvent = Weapon->DroppedMagazineHitEvent;
}

bool APickupMagazineActor::CanPickUpNow(APlayerCharacter* PickerUpper)
{
	if (MagazineData.Ammo <= 0)
		return false;
	
	if (GetValidWeaponForPickerUpper(PickerUpper) != nullptr)
	{
		if (Super::CanPickUpNow(PickerUpper))
		{
			return true;
		}
	}
	
	return false;
}

void APickupMagazineActor::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ImpactSoundsPlayed > 2)
		return;
	
	ImpactSoundsPlayed++;
	float NormalImpulseSize = NormalImpulse.Size();
	if (NormalImpulseSize > MinimumHitThreshold)
	{
		auto sound = UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), DroppedMagazineHitEvent, GetActorTransform(), true);
		if (sound.Instance)
		{
			sound.Instance->setParameterByName("Material", UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get()));
			sound.Instance->setParameterByName("Velocity", NormalImpulseSize);
		}
	}
}

void APickupMagazineActor::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent)
{
	APlayerCharacter* InteractCharacter = Cast<APlayerCharacter>(InteractInstigator);
	if (InteractCharacter)
	{
		if (CanPickUpNow(InteractCharacter))
			ActorPickedUp(InteractCharacter);
	}
}

FName APickupMagazineActor::DetermineAnimatedIcon_Implementation() const
{
	if (MagazineData.Ammo <= 0)
		return NAME_None;
	
	if (CameFromWeapon && !CameFromWeapon->bHasBeenCleared)
		return "Pickup Magazine";

	return NAME_None;
}

// Void Interactive, 2020


#include "AnimNotify_SpawnWeapon.h"

#include "Components/InventoryComponent.h"

void UAnimNotify_SpawnWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (PotentialWeapons.Num() == 0)
		return;

	AReadyOrNotCharacter* ReadyOrNotCharacter = Cast<AReadyOrNotCharacter>(MeshComp->GetOwner());
	if (ReadyOrNotCharacter && !ReadyOrNotCharacter->IsCivilian())
	{
		if (ReadyOrNotCharacter->GetLocalRole() < ROLE_Authority)
			return;
		
		TSubclassOf<ABaseWeapon> WeaponToSpawn = PotentialWeapons[FMath::RandRange(0, PotentialWeapons.Num() - 1)];
		ABaseWeapon* Weapon = ReadyOrNotCharacter->GetWorld()->SpawnActor<ABaseWeapon>(WeaponToSpawn);
		if (Weapon)
		{
			ReadyOrNotCharacter->GetInventoryComponent()->AddInventoryItem(Weapon);
			ReadyOrNotCharacter->GetInventoryComponent()->PutItemInHands(Weapon, true, true);
		}
	}
}

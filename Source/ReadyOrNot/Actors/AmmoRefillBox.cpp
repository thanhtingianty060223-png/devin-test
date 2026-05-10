// Void Interactive, 2020


#include "Actors/AmmoRefillBox.h"

#include "Components/InteractableComponent.h"

// Sets default values
AAmmoRefillBox::AAmmoRefillBox()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("Interactable Component"));
	InteractableComponent->RequiredLookAtPercentage = 0.95f;
	InteractableComponent->bDistanceFadeIcon = false;
	InteractableComponent->bImprintIconOnHUDUponInteraction = true;
	InteractableComponent->ActionSlot1.Init("Use", IE_Pressed, FText::FromStringTable("ActionPromptTable", "RefillAmmo"));
	InteractableComponent->ActionSlot1.bCondition = true;
	InteractableComponent->SetupAttachment(GetRootComponent());

	

}

// Called when the game starts or when spawned
void AAmmoRefillBox::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAmmoRefillBox::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bWantsToUseAmmoRefillBox && RefillCharacter)
	{
		TimeUntilRefill -= DeltaTime;
		if (TimeUntilRefill <= 0.0f)
		{
			RefillCharacter->GetInventoryComponent()->DestroyAllEquippedItems();
			if (RefillCharacter->GetRONPlayerState())
			{
				UBpGameplayHelperLib::EquipLoadoutOnPlayer(RefillCharacter->GetRONPlayerState()->GetLoadout(), RefillCharacter, FLoadoutEquipOptions());
				for (ABaseItem* i : RefillCharacter->GetInventoryComponent()->GetInventoryItems())
				{
					if (i && i->ItemCategories == EquippedItemCategories)
					{
						RefillCharacter->GetInventoryComponent()->PutItemInHands(i, true);
						break;
					}
				}
				bWantsToUseAmmoRefillBox = false;
				RefillCharacter = nullptr;
			}			
		}
	}
}

void AAmmoRefillBox::Interact_Implementation(AReadyOrNotCharacter* InteractInstigator,
	UInteractableComponent* InInteractableComponent)
{

	if (InteractInstigator->GetEquippedItem())
	{
		FItemChangeRequest ItemChangeRequest;
		ItemChangeRequest.FromItem = InteractInstigator->GetEquippedItem();
		ItemChangeRequest.bNoDraw = true;
		InteractInstigator->GetInventoryComponent()->PlayLocalHolster(ItemChangeRequest);
		if (InteractInstigator->GetEquippedItem()->AnimationData && InteractInstigator->GetEquippedItem()->AnimationData->Holster.Body_FP)
		{
			TimeUntilRefill = InteractInstigator->GetEquippedItem()->AnimationData->Holster.Body_FP->GetPlayLength();
			TimeUntilRefill -= 0.25f;
		}
		EquippedItemCategories = InteractInstigator->GetEquippedItem()->ItemCategories;
	}

	if (!HasAuthority())
		return;
	
	bWantsToUseAmmoRefillBox = true;
	RefillCharacter = InteractInstigator;
	
}
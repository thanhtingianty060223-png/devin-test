// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmmoRefillBox.generated.h"

UCLASS()
class READYORNOT_API AAmmoRefillBox : public AActor, public IUseabilityInterface
{
	GENERATED_BODY()

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UInteractableComponent* InteractableComponent;

public:	
	// Sets default values for this actor's properties
	AAmmoRefillBox();

	bool bWantsToUseAmmoRefillBox = false;
	UPROPERTY()
	AReadyOrNotCharacter* RefillCharacter;
	float TimeUntilRefill = 0.0f;
	// used as a quick hack to equip the same item
	TArray<EItemCategory> EquippedItemCategories;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	

	virtual void Interact_Implementation(AReadyOrNotCharacter* InteractInstigator, UInteractableComponent* InInteractableComponent) override;
};

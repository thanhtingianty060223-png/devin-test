// Copyright Void Interactive, 2017

#pragma once
#include "PlayerCharacter.h"
#include "SniperCharacter.generated.h"

/**
 *	Snipers are controllable PlayerCharacters that spawn either as Marksman, Spotters, or Sniper personnel.
 *	@author	Nick Whitlock <eezstreet>
 */
UCLASS(BlueprintType, Blueprintable)
class READYORNOT_API ASniperCharacter : public APlayerCharacter
{
	GENERATED_BODY()

public:
	// The designation of this sniper.
	UPROPERTY(BlueprintReadOnly, Category = Sniper)
	int32 Designation;

	// Whether the ADS of this sniper is locked.
	UPROPERTY(BlueprintReadOnly, Category = Sniper)
	bool bADSLocked = false;


	// Overrides.
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	virtual void BeginPlay() override;
	virtual void FireSelect() override;
	virtual void PrimaryUse() override;
	virtual void SecondaryUse() override;
	virtual void EndSecondaryUse() override;
	virtual void PossessedBy(AController* NewController) override;

	// The timer stuff for fading to black.
	UPROPERTY(BlueprintReadOnly)
	FTimerHandle ExitControlHandle;

	// Called when we are to stop controlling this character.
	UFUNCTION()
	void ExitControl();

	// The attachments to use on our PRIMARY weapon.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawned Attachments", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UScopedWeaponAttachment> PrimaryScopeAttachment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawned Attachments", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UWeaponAttachment> PrimaryMuzzleAttachment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawned Attachments", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UWeaponAttachment> PrimaryUnderbarrelAttachment;

	// The attachments to use on our SECONDARY weapon.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawned Attachments", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UScopedWeaponAttachment> SecondaryScopeAttachment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawned Attachments", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UWeaponAttachment> SecondaryMuzzleAttachment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawned Attachments", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UWeaponAttachment> SecondaryUnderbarrelAttachment;

	// IControllableByTablet implementation
	//virtual bool CanControlWithTablet_Implementation(class APlayerCharacter* TabletOwner) override;
	//virtual bool CanTabletViewMe_Implementation(class APlayerCharacter* TabletOwner, class AReadyOrNotGameState* GameState) override;
};

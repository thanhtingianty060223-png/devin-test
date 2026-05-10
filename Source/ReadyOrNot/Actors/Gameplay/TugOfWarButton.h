// � Void Interactive, 2017

#pragma once

#include "Actors/InteractionActor.h"
#include "TugOfWarMover.h"
#include "ReadyOrNotGameMode.h"
#include "TugOfWarButton.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API ATugOfWarButton : public AInteractionActor
{
	GENERATED_BODY()

public:
	ATugOfWarButton();

	// Only the stated team can use this button
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tug of War")
	ETeamType OnlyTeamUse = ETeamType::TT_NONE;

	// The mover that this button influences
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tug of War")
		ATugOfWarMover* Mover = nullptr;

	virtual void BeginPlay() override;
	virtual bool CanBeUsedNow_Implementation(AActor* PotentialUser) override;
	virtual bool CanUse_Implementation(class APlayerCharacter* User) override;
	virtual void Server_TryUse_Implementation(AActor* User) override;
	virtual void Server_EndUse_Implementation(AActor* User) override;

	UPROPERTY()
	APlayerCharacter* CurrentUser;

	// This function is called when an influencer is killed
	UFUNCTION()
	void OnInfluencerKilled(AActor* Causer, ACharacter* InstigatorCharacter, ACharacter* KilledCharacter, struct FDamageEvent const& DamageEvent, APlayerState* LastPlayerState);

	// This function is called when an influencer is arrested
	UFUNCTION()
	void OnInfluencerArrested(APlayerCharacter* ArrestedCharacter, APlayerCharacter* InstigatorCharacter);

	// This function is called when an influencer is stunned
	UFUNCTION()
	void OnInfluencerStunned(APlayerCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser);

	void RemoveInfluencer(APlayerCharacter* Influencer);
private:
	FScriptDelegate InfluencerKilledDelegate;
	FScriptDelegate InfluencerArrestedDelegate;
	FScriptDelegate InfluencerStunnedDelegate;
};

// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Gameplay/TugOfWarMover.h"
#include "Components/BoxComponent.h"
#include "TugOfWarZone.generated.h"

UCLASS()
class READYORNOT_API ATugOfWarZone : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATugOfWarZone();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
private:
	UFUNCTION()
		void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex);

public:

	// is this zone disabled?? can't be used for capturing and shouldn't show on the HUD
	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bZoneDisabled = false;
	// The mover that this button influences
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tug of War")
		ATugOfWarMover* Mover = nullptr;

	// The thing that determines the bounds of this volume.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Environmental)
		UBoxComponent* Bounds;

	// This function is called when an influencer is killed
	UFUNCTION()
		void OnInfluencerKilled(AActor* Causer, ACharacter* InstigatorCharacter, ACharacter* KilledCharacter, struct FDamageEvent const& DamageEvent, APlayerState* LastPlayerState);

	// This function is called when an influencer is arrested
	UFUNCTION()
		void OnInfluencerArrested(APlayerCharacter* ArrestedCharacter, APlayerCharacter* InstigatorCharacter);

	void RemoveInfluencer(APlayerCharacter* Influencer);
private:
	FScriptDelegate InfluencerKilledDelegate;
	FScriptDelegate InfluencerArrestedDelegate;
	FScriptDelegate InfluencerStunnedDelegate;

};

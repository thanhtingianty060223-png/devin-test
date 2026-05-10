// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "CollectedEvidenceActor.generated.h"

UCLASS()
class READYORNOT_API ACollectedEvidenceActor final : public AActor, public ISecurable
{
	GENERATED_BODY()
	
public:	
	ACollectedEvidenceActor();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CollectionBagMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	UFMODEvent* Bag_Spawn_Sound;

	virtual bool IsSecured_Implementation() const override;
	virtual bool CanBeSecured_Implementation() const override;
	virtual bool CanBeSecuredByTrailers_Implementation() const override;
	virtual FVector GetLocation_Implementation() const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Collection")
	void PlaySpawnSound();
	
	UFUNCTION(Server, Reliable, Category = "Sounds")
	void Server_PlaySpawnSound();
	void Server_PlaySpawnSound_Implementation();
	bool Server_PlaySound_Validate() { return true; }

	UFUNCTION(NetMulticast, Reliable, Category = "Sounds")
	void Multicast_PlaySpawnSound();
	void Multicast_PlaySpawnSound_Implementation();
};




// Void Interactive, 2020

#pragma once

#include "GameFramework/Actor.h"
#include "ThrownEvidenceActor.generated.h"

UCLASS()
class READYORNOT_API AThrownEvidenceActor : public AActor, public ICanIssueCommandOn
{
	GENERATED_BODY()
	
public:	
	AThrownEvidenceActor();

	UPROPERTY()
	UBoxComponent* BoxComponent;

	UPROPERTY()
	ABaseItem* OwningItem = nullptr;

	UPROPERTY(Replicated)
	FVector Rep_Location;

	UPROPERTY(Replicated)
	FRotator Rep_Rotation;

protected:
	virtual void Tick(float DeltaTime) override;

	virtual bool CanIssueCommand_Implementation() const override;
	virtual AActor* GetCommandActor_Implementation() const override;

	UFUNCTION()
	void OnBoxHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	FFMODEventInstance HitEventInstance;
};

// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExfilActor.generated.h"

UCLASS()
class READYORNOT_API AExfilActor : public AActor, public ICanIssueCommandOn, public IUseabilityInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AExfilActor();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual bool CanIssueCommand_Implementation() const override;
	virtual AActor* GetCommandActor_Implementation() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* BaseMesh = nullptr;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;	

private:
	bool bIsExfilEnabled;
	

};

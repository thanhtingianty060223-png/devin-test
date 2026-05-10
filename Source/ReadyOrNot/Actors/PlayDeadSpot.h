// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayDeadSpot.generated.h"

UCLASS()
class READYORNOT_API APlayDeadSpot : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APlayDeadSpot();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};

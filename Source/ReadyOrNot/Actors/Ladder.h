// Copyright Void Interactive, 2017

#pragma once

#include "GameFramework/Actor.h"
#include "Ladder.generated.h"

UCLASS()
class READYORNOT_API ALadder : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Collision, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* BoxCollision;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* LadderMesh;
	
public:	
	// Sets default values for this actor's properties
	ALadder();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	void SetLadderLength(float NewLength);

	FORCEINLINE class UBoxComponent* GetBoxCollision() const { return BoxCollision; }
	FORCEINLINE class UStaticMeshComponent* GetLadderMesh() const { return LadderMesh; }
	
};

#pragma once

#include "GameFramework/Actor.h"
#include "PlacedZipcuffs.generated.h"

UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API APlacedZipcuffs : public AActor
{
	GENERATED_BODY()

public:
	APlacedZipcuffs();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Zipcuffs)
	USkeletalMeshComponent* ZipcuffMesh;

	void Reset() override;
};

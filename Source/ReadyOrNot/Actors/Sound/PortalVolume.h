// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Actors/Environment/BreakableGlass.h"
#include "PortalVolume.generated.h"


UENUM(BlueprintType)
enum EPortalType
{
	Vertical,
	Horizontal
};

UCLASS()
class READYORNOT_API APortalVolume : public AVolume
{
	GENERATED_BODY()

	public:
		APortalVolume();

		UPROPERTY(BlueprintReadWrite, EditAnywhere)
		bool bIsOutside = false;

		UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TEnumAsByte<EPortalType> PortalType = Horizontal;

		UPROPERTY()
		TArray<AActor*> OverlappingActors;

		UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<TSoftObjectPtr<AActor>> AttachedObjects;

		UPROPERTY(BlueprintReadOnly)
		TArray<ADoor*> Doors;

		UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TSoftObjectPtr<AActor> BreakableGlass_SoftPointer;
	

		UPROPERTY(BlueprintReadOnly)
		TArray<ABreakableGlass*> BreakableGlasses;


		void BeginPlay() override;
};

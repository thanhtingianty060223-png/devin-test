// Void Interactive, 2020

#pragma once

#include "CoreMinimal.h"
#include "Actors/Attachments/WeaponAttachment.h"
#include "MagazineAttachment.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UMagazineAttachment : public UWeaponAttachment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	UStaticMesh* MagazineStaticMesh;

	UPROPERTY(EditAnywhere)
	FName Socket_01;
	
	UPROPERTY(EditAnywhere)
	FName Socket_02;
};

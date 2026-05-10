// Copyright Void Interactive, 2017

#pragma once

#include "Actors/BaseItem.h"
#include "BaseMagPouch.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ABaseMagPouch : public ABaseItem
{
	GENERATED_BODY()

	ABaseMagPouch();

	virtual void BeginPlay() override;
	
		/** Mesh */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* MagazineMesh;

	UPROPERTY(EditAnywhere, Category = Pouches)
	UAnimationAsset* OpenPouchAnim;
	UPROPERTY(EditAnywhere, Category = Pouches)
	UAnimationAsset* ClosedPouchAnim;

	UFUNCTION(BlueprintCallable, Category = Pouches)
	void OpenPouch();
	UFUNCTION(BlueprintCallable, Category = Pouches)
	void ClosePouch();

	UFUNCTION(BlueprintCallable, Category = Pouches)
		void ShowMagazine();

	UFUNCTION(BlueprintCallable, Category = Pouches)
		void HideMagazine();

	UPROPERTY(EditAnywhere, Category = Pouches)
		FName MagSocket;

	FORCEINLINE USkeletalMeshComponent* GetMagazineMesh() const { return MagazineMesh; }
	
	
};
